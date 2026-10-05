// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#include "qhauk.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QtPositioning>

#include <algorithm>

#include "qhaukmockpositionsource.h"
#include "utility/loghandler.h"

using namespace Qt::Literals::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcPosition, "qhauk.position", QtDebugMsg)
}

QHauk::QHauk(QObject *parent)
    : QObject(parent)
    , m_locationSource(QHaukMockPositionSource::createFromEnvironment(this))
{
    if (!m_locationSource) {
        // allowAutoPause on ios this requires a backport of https://github.com/qt/qtpositioning/commit/aa3be9b00421e2bf7acaa6f65367b79391a1b695.patch
        // or probably Qt 6.13
        m_locationSource = QGeoPositionInfoSource::createDefaultSource({{u"allowAutoPause"_s, false}}, this);
    }
    if (!m_locationSource) {
        qCWarning(lcPosition) << u"Failed to create position info source";
    } else {
        qCDebug(lcPosition) << u"QHauk::QHauk" << m_locationSource->sourceName();
        m_locationSource->requestUpdate();
        connect(m_locationSource, &QGeoPositionInfoSource::errorOccurred, this, [this](QGeoPositionInfoSource::Error error) {
            qCWarning(lcPosition) << u"Position info source error:" << error << m_locationSource->sourceName() << m_locationSource->lastKnownPosition();
        });
    }
}

QHauk::~QHauk() = default;

QHaukSession *QHauk::createSession()
{
    if (!m_locationSource) {
        qCWarning(lcPosition) << u"Failed to create position info source";
        return nullptr;
    }
    auto *out = new QHaukSession(this);
    connect(out, &QHaukSession::destroyed, this, [out, this] {
        m_sessions.removeOne(out);
        if (m_sessions.isEmpty()) {
            m_locationSource->stopUpdates();
        }
    });
    connect(out, &QHaukSession::isSharingChanged,  this,[this] {
        if (std::ranges::find_if(m_sessions, [](const auto *session) { return session->isSharing(); }) != m_sessions.end()) {
            qCDebug(lcPosition) << u"Starting location updates";
            m_locationSource->startUpdates();
        } else {
            qCDebug(lcPosition) << u"Stopping location updates";
            m_locationSource->stopUpdates();
        }
    });
    m_sessions.append(out);
    m_locationSource->startUpdates();
    return out;
}

void QHauk::setHost(const QUrl &url)
{
    if (m_host != url) {
        m_host = url;
        Q_EMIT hostChanged();
    }
}

QUrl QHauk::host() const
{
    return m_host;
}

void QHauk::setPassword(const QString &password)
{
    if (m_password != password) {
        m_password = password;
        Q_EMIT passwordChanged();
    }
}

QString QHauk::password() const
{
    return m_password;
}

void QHauk::setNickName(const QString &nickName)
{
    if (m_nickName != nickName) {
        m_nickName = nickName;
        Q_EMIT nickNameChanged();
    }
}

QString QHauk::nickName() const
{
    return m_nickName;
}

QGeoPositionInfoSource *QHauk::locationSource() const
{
    return m_locationSource;
}

void QHauk::setClipboard(const QString &text) const
{
    qApp->clipboard()->setText(text);
}

void QHauk::copyLogToClipboard() const
{
    setClipboard(Utility::LogHandler::recentMessages().join(u"\n"));
}
