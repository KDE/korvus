// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#include "qhauksession.h"

#include "client/haukapi.h"
#include "haukreply.h"
#include "qhauk.h"
#include "utility/qhauknam.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QRestAccessManager>
#include <QtPositioning>

#include <algorithm>
#include <memory>

#include "utility/chronoelapsedtimer.h"

using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;

namespace {
Q_LOGGING_CATEGORY(lcSession, "qhauk.session", QtDebugMsg)
Q_LOGGING_CATEGORY(lcPosition, "qhauk.position", QtDebugMsg)
}

#if defined(Q_OS_IOS)
#import <UIKit/UIKit.h>

namespace {
Q_LOGGING_CATEGORY(lcBackgroundTask, "qhauk.backgroundtask", QtDebugMsg)
}

class ScopedBackgroundTask
{
public:
    explicit ScopedBackgroundTask(NSString *name)
        : m_name(QString::fromNSString(name))
    {
        m_taskId = [[UIApplication sharedApplication] beginBackgroundTaskWithName:name
                                                                expirationHandler:^{
                                                                    qCWarning(lcBackgroundTask) << u"Background task" << m_name << m_taskId
                                                                                                << u"expired before it completed after" << m_timer;
                                                                    end();
                                                                }];
        if (m_taskId == UIBackgroundTaskInvalid) {
            qCWarning(lcBackgroundTask) << u"Failed to begin background task" << m_name;
        } else {
            qCDebug(lcBackgroundTask) << u"Began background task" << m_name << m_taskId << u"remaining background time:"
                                      << [UIApplication sharedApplication].backgroundTimeRemaining << u"s";
        }
    }

    ~ScopedBackgroundTask() { end(); }

    void end()
    {
        if (m_taskId != UIBackgroundTaskInvalid) {
            UIBackgroundTaskIdentifier id = m_taskId;
            m_taskId = UIBackgroundTaskInvalid;
            qCDebug(lcBackgroundTask) << u"Ending background task" << m_name << id << u"after" << m_timer;
            [[UIApplication sharedApplication] endBackgroundTask:id];
        }
    }

private:
    QString m_name;
    OCC::Utility::ChronoElapsedTimer m_timer;
    UIBackgroundTaskIdentifier m_taskId = UIBackgroundTaskInvalid;
};

constexpr auto makeBackgroundTaskScopePost()
{
    return std::make_unique<ScopedBackgroundTask>(@"QHaukPostLocation");
}
#else
constexpr auto makeBackgroundTaskScopePost()
{
    return nullptr;
}
#endif

class QHaukSessionPrivate
{
    Q_DECLARE_PUBLIC(QHaukSession)
public:
    explicit QHaukSessionPrivate(QHaukSession *q)
        : q_ptr(q)
        , m_haukApi(std::make_unique<Hauk::HaukApi>())
    {
    }

    QHauk *hauk() const;
    void onPositionUpdated(const QGeoPositionInfo &info);
    void abortSession(const QString &error)
    {
        Q_Q(QHaukSession);
        resetSession();
        m_error = error;
        Q_EMIT q->errorChanged();
        Q_EMIT q->isSharingChanged();
    }
    void resetSession()
    {
        m_isSharing = false;
        m_sessionId.clear();
        m_shareId.clear();
        m_shareUrl.clear();
    }
    QHaukPositionHistory *groupEntry(const QString &name);

    QHaukSession *q_ptr;

    QString m_shareId;
    QString m_sessionId;
    QUrl m_shareUrl;
    bool m_isSharing = false;
    std::unique_ptr<Hauk::HaukApi> m_haukApi;
    QString m_nickName;
    QHaukPositionHistory *m_userPositions = nullptr;
    QList<QHaukPositionHistory *> m_groupPositions;
    qreal m_lastPositionFetchTime = 0;
    OCC::Utility::ChronoElapsedTimer m_lastPositionPost;
    std::chrono::minutes m_duration = 1h;
    QHaukSession::ShareType m_shareType = QHaukSession::ShareType::Solo;
    QString m_pin;

    QString m_error;
};

QHauk *QHaukSessionPrivate::hauk() const
{
    Q_Q(const QHaukSession);
    return qobject_cast<QHauk *>(q->parent());
}

void QHaukSessionPrivate::onPositionUpdated(const QGeoPositionInfo &info)
{
    if (!m_isSharing) {
        return;
    }
    Q_Q(QHaukSession);
    const auto result = m_userPositions->append(info);
    if (result != QHaukPositionHistory::AppendResult::Rejected) {
        Q_EMIT q->positionsChanged();
    }
    if (m_shareType == QHaukSession::ShareType::Follower) {
        return;
    }
    // only post if the change was noteworthy or once a minute to indicate we are still alive
    if (result == QHaukPositionHistory::AppendResult::Appended || m_lastPositionPost.duration() > 1min) {
        m_lastPositionPost.reset();
        qCDebug(lcPosition) << u"Position updated:" << info.coordinate().latitude() << info.coordinate().longitude() << info.timestamp() << u"speed:"
                            << m_userPositions->speed() << u"accuracy:" << m_userPositions->accuracy();
        m_haukApi->postLocation(m_sessionId, info.coordinate().latitude(), info.coordinate().longitude(), info.timestamp().toUTC().toSecsSinceEpoch(), {},
            m_userPositions->speed(), m_userPositions->accuracy(), {}, q,
            [bgTask = makeBackgroundTaskScopePost(), this](const QRestReply &reply, const QString &summary) {
                if (reply.isSuccess()) {
                    HaukPostReply haukPostReply(summary);
                    Q_ASSERT(haukPostReply.status() != HaukPostReply::Status::Invalid);
                    qCDebug(lcSession) << u"Location posted:" << haukPostReply.status();
                    if (haukPostReply.status() == HaukPostReply::Status::Error) {
                        qCDebug(lcSession) << u"Session expired";
                        abortSession(haukPostReply.error());
                    }
                } else {
                    qCWarning(lcSession) << u"Error posting location:" << reply.errorString() << summary;
                }
            });
    }
}

QHaukPositionHistory *QHaukSessionPrivate::groupEntry(const QString &name)
{
    Q_Q(QHaukSession);
    // groups are small, a linear search is cheaper than maintaining a map next to the list
    auto it = std::ranges::find(m_groupPositions, name, &QHaukPositionHistory::name);
    if (it == m_groupPositions.end()) {
        auto out = m_groupPositions.emplace_back(new QHaukPositionHistory(name, QList<QGeoCoordinate>{}, q));
        Q_EMIT q->groupNamesChanged();
        return out;
    }
    return *it;
}


QHaukSession::QHaukSession(QHauk *parent)
    : QObject(parent)
    , d_ptr(std::make_unique<QHaukSessionPrivate>(this))
{
    Q_D(QHaukSession);
    // The session object is reused across start/stop, set up everything that is not session specific once
    d->m_haukApi->setRestAccessManager(new QRestAccessManager(new QHaukNam(this), this));
    d->m_userPositions = new QHaukPositionHistory(d->m_nickName, {}, this);
    // onPositionUpdated ignores updates while not sharing
    connect(parent->locationSource(), &QGeoPositionInfoSource::positionUpdated, this, [d](const QGeoPositionInfo &info) { d->onPositionUpdated(info); });
}

QHaukSession::~QHaukSession() = default;

QUrl QHaukSession::url() const
{
    Q_D(const QHaukSession);
    return d->m_shareUrl;
}

QString QHaukSession::pin() const
{
    Q_D(const QHaukSession);
    return d->m_pin;
}

void QHaukSession::setPin(const QString &pin)
{
    Q_D(QHaukSession);
    if (pin != d->m_pin) {
        d->m_pin = pin;
        Q_EMIT pinChanged();
    }
}

QString QHaukSession::shareId() const
{
    Q_D(const QHaukSession);
    return d->m_shareId;
}

QString QHaukSession::nickName() const
{
    Q_D(const QHaukSession);
    return d->m_nickName;
}

QHaukPositionHistory *QHaukSession::positions() const
{
    Q_D(const QHaukSession);
    return d->m_userPositions;
}

const QList<QHaukPositionHistory *> &QHaukSession::groupPositions() const
{
    Q_D(const QHaukSession);
    return d->m_groupPositions;
}

QStringList QHaukSession::groupNames() const
{
    Q_D(const QHaukSession);
    auto view = d->m_groupPositions | std::views::transform([](const QHaukPositionHistory *history) { return history->name; });
    return QStringList {view.begin(), view.end()};
}

void QHaukSession::requestGroupPositionsUpdate()
{
    Q_D(const QHaukSession);
    qCDebug(lcSession) << "Requesting group positions update";
    d->m_haukApi->fetchLocations(d->m_shareId, {}, d->m_lastPositionFetchTime, this, [this](const QRestReply &reply, const Hauk::GroupShare &summary) {
        if (reply.isSuccess()) {
            if (!summary.isServerTimeSet()) {
                // qt does not provide the raw reply, this might be a plaintext response
                // assume the session is over, but ignore the error
                // a failing post will abort if the session is over
                qCWarning(lcSession) << "Failed to fetch group positions: Invalid session?";
                return;
            }
            Q_D(QHaukSession);
            d->m_lastPositionFetchTime = summary.getServerTimeValue();
            bool updated = false;
            // nickname -> list of [lat, lon, time, provider, accuracy|null, speed|null], oldest first,
            // or null if the member has no points newer than since.
            // Values are untyped QJsonValues, see GroupShare.points in hauk.yaml.
            // Null accuracy and speed are mapped to 0 by toDouble()
            const auto points = summary.getPointsValue();
            for (const auto &[name, memberPointsValue] : points.asKeyValueRange()) {
                if (name == d->m_nickName) {
                    // Ignore our own points, we track our own position separately.
                    continue;
                }
                auto *entry = d->groupEntry(name);
                if (!memberPointsValue.isArray()) {
                    if (!memberPointsValue.isNull()) {
                        qCWarning(lcSession) << u"Ignoring malformed points for" << name << memberPointsValue;
                    }
                    continue;
                }
                const QJsonArray memberPoints = memberPointsValue.toArray();
                entry->positions.reserve(entry->positions.size() + memberPoints.size());
                bool memberUpdated = false;
                for (qsizetype index = 0; index < memberPoints.size(); ++index) {
                    const QJsonArray point = memberPoints.at(index).toArray();
                    if (point.size() < 6) {
                        qCWarning(lcSession) << u"Ignoring malformed point for" << name << point;
                        continue;
                    }
                    memberUpdated |= entry->append(QGeoCoordinate(point[0].toDouble(), point[1].toDouble()), point[4].toDouble())
                        != QHaukPositionHistory::AppendResult::Rejected;

                    if (index == memberPoints.size() - 1) {
                        entry->setAccuracy(point[4].toDouble());
                        entry->setSpeed(point[5].toDouble());
                    }
                }
                // the server doesn't share the direction, derive it from the last two recorded points
                if (memberUpdated) {
                    if (memberUpdated && entry->positions.size() > 1) {
                        entry->setDirection(entry->positions.at(entry->positions.size() - 2).azimuthTo(entry->positions.last()));
                    }
                }
                updated |= memberUpdated;
            }
            qCDebug(lcSession) << "Fetched group positions:" << d->m_groupPositions.size() << u"members";
            if (updated) {
                Q_EMIT groupPositionsChanged();
            }
        } else {
            qCWarning(lcSession) << "Failed to fetch group positions:" << reply.errorString();
        }
    });
}
void QHaukSession::start()
{
    Q_D(QHaukSession);
    // QML might still reference the old histories until it processed the change signals, delete them later
    for (auto *history : std::as_const(d->m_groupPositions)) {
        history->deleteLater();
    }
    d->m_groupPositions.clear();
    Q_EMIT groupPositionsChanged();
    d->m_lastPositionFetchTime = 0;
    auto *hauk = d->hauk();
    d->m_userPositions->deleteLater();
    d->m_userPositions = new QHaukPositionHistory(d->m_nickName, {}, this);
    Q_EMIT positionsChanged();

    for (const QString &op : d->m_haukApi->operations()) {
        d->m_haukApi->setServerVariable(op, 0, u"host"_s, hauk->host().host());
        d->m_haukApi->setServerVariable(op, 0, u"basePath"_s, hauk->host().path());
    }
}

bool QHaukSession::isSharing() const
{
    Q_D(const QHaukSession);
    return d->m_isSharing;
}

std::chrono::minutes QHaukSession::duration() const
{
    Q_D(const QHaukSession);
    return d->m_duration;
}

void QHaukSession::setDuration(const std::chrono::minutes &duration)
{
    Q_D(QHaukSession);
    if (duration != d->m_duration) {
        d->m_duration = duration;
        Q_EMIT durationChanged();
    }
}

void QHaukSession::setShareType(ShareType shareType)
{
    Q_D(QHaukSession);
    if (shareType != d->m_shareType) {
        d->m_shareType = shareType;
        Q_EMIT shareTypeChanged();
    }
}

QHaukSession::ShareType QHaukSession::shareType() const
{
    Q_D(const QHaukSession);
    return d->m_shareType;
}

QString QHaukSession::error() const
{
    Q_D(const QHaukSession);
    return d->m_error;
}

void QHaukSession::stop()
{
    Q_D(QHaukSession);
    if (!d->m_isSharing) {
        qCDebug(lcSession) << u"Not sharing, nothing to stop";
        return;
    }
    // followers have no session of their own on the server
    if (shareType() != ShareType::Follower) {
        d->m_haukApi->stopSharing(d->m_sessionId, {}, {}, this, [](const QRestReply &reply, const QString &summary) {
            if (reply.isSuccess()) {
                qCDebug(lcSession) << u"Sharing stopped" << summary;
            } else {
                qCWarning(lcSession) << u"Error stopping sharing:" << reply.errorString() << summary;
            }
        });
    }
    d->resetSession();
    Q_EMIT isSharingChanged();
}

void QHaukSession::startSharing()
{
    Q_D(QHaukSession);
    if (d->m_isSharing) {
        qCDebug(lcSession) << u"Session already started";
        return;
    }
    QHauk *hauk = d->hauk();
    qCDebug(lcSession) << u"Starting session" << hauk->host() << u"for" << d->m_duration.count() << u"minutes" << shareType();
    auto adoptionFlag = [shareType = shareType()] {
        // parts of the generated code are a bit rough
        Hauk::AdoptionFlag ad;
        if (shareType == ShareType::Solo) {
            /* allow adoption*/
            ad.setValue(Hauk::AdoptionFlag::eAdoptionFlag::_1);
        } else {
            ad.setValue(Hauk::AdoptionFlag::eAdoptionFlag::_0);
        }
        return ad;
    }();
    // keep the name used for this session, the setting might change while sharing
    d->m_nickName = hauk->nickName();
    Q_EMIT nickNameChanged();
    start();
    // The interval is sent in (fractional) seconds, never send 0: the server rejects anything below its min_interval (default 1s)
    const auto *locationSource = hauk->locationSource();
    const std::chrono::duration<double> updateInterval = std::max({std::chrono::milliseconds(locationSource->updateInterval()),
        std::chrono::milliseconds(locationSource->minimumUpdateInterval()), std::chrono::milliseconds(1s)});
    d->m_haukApi->createSession(std::chrono::duration_cast<std::chrono::seconds>(d->m_duration).count(), updateInterval.count(), hauk->password(), {}, {},
        static_cast<int>(shareType()), d->m_nickName, shareType() == ShareType::JoinGroup ? pin() : QString(), adoptionFlag, {}, this,
        [hauk, this](const QRestReply &reply, const QString &summary) {
            Q_D(QHaukSession);
            if (reply.isSuccess()) {
                HaukCreateReply createReply(summary);
                if (createReply.status() == HaukReply::Status::Ok) {
                    d->m_isSharing = true;
                    d->m_sessionId = createReply.session();
                    d->m_shareId = createReply.shareId();
                    d->m_shareUrl = createReply.url();
                    qCDebug(lcSession) << u"Session created:" << d->m_sessionId << d->m_shareUrl;
                    // pin is only replied when a new session is created
                    if (!createReply.groupPin().isEmpty()) {
                        setPin(createReply.groupPin());
                    }
                    d->onPositionUpdated(hauk->locationSource()->lastKnownPosition());
                    Q_EMIT isSharingChanged();
                } else {
                    qCWarning(lcSession) << u"Error creating session:" << summary;
                    d->abortSession(createReply.error());
                }
            } else {
                qCWarning(lcSession) << u"Error creating session:" << reply.errorString() << summary;
                d->abortSession(reply.errorString());
            }
        });
}

void QHaukSession::startFollowing(const QString &sessionId)
{
    Q_D(QHaukSession);
    if (d->m_isSharing) {
        qCDebug(lcSession) << u"Session already started";
        return;
    }
    setShareType(ShareType::Follower);
    d->m_nickName = tr("Follower (%1)").arg(d->hauk()->nickName());
    Q_EMIT nickNameChanged();
    start();
    d->m_isSharing = true;
    d->m_shareId = sessionId;
    Q_EMIT isSharingChanged();
}

int QHaukSession::durationInt() const
{
    Q_D(const QHaukSession);
    return d->m_duration.count();
}

void QHaukSession::setDurationInt(int duration)
{
    setDuration(std::chrono::minutes(duration));
}
