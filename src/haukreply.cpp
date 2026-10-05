// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#include "haukreply.h"

#include "qhauksession.h"

#include <QDebug>
#include <QLoggingCategory>

using namespace Qt::Literals::StringLiterals;

namespace {
Q_LOGGING_CATEGORY(lcReply, "qhauk.reply", QtDebugMsg)
}

class HaukReplyData : public QSharedData
{
public:
    HaukReplyData() = default;
    HaukReplyData(const QString &_response)
        : m_response(_response)
        , m_data(QStringView(m_response).split('\n'_L1, Qt::SkipEmptyParts))
    {
        qCDebug(lcReply) << u"HaukReplyData" << m_data;
        if (!m_data.isEmpty() && m_data[0] == "OK"_L1) {
            m_status = HaukReply::Status::Ok;
        }
    }

    ~HaukReplyData() = default;

    const QString m_response;
    const QList<QStringView> m_data;
    HaukReply::Status m_status = HaukReply::Status::Error;
};

HaukReply::HaukReply()
    : d(new HaukReplyData(QString()))
{
}

HaukReply::HaukReply(const QString &_response)
    : d(new HaukReplyData(_response))
{
}

HaukReply::HaukReply(const HaukReply &other) = default;
HaukReply::~HaukReply() = default;

HaukReply::Status HaukReply::status() const
{
    return d->m_status;
}
QString HaukReply::error() const
{
    const auto error = data(0);
    if (error == "Session expired!"_L1) {
        return QHaukSession::tr("Session expired");
    }
    if (error == "Invalid group PIN!"_L1) {
        return QHaukSession::tr("Invalid group PIN");
    }
    return {};
}

QStringView HaukReply::data(qsizetype index) const
{
    if (index < 0) {
        index = d->m_data.size() + index;
    }
    if (index >= d->m_data.size()) {
        qCWarning(lcReply) << "Invalid index" << index;
        return {};
    }
    return d->m_data[index];
}

const QList<QStringView> &HaukReply::rawData() const
{
    return d->m_data;
}

QString HaukCreateReply::session() const
{
    if (status() == Status::Ok) {
        return data(1).toString();
    }
    return {};
}

QUrl HaukCreateReply::url() const
{
    if (status() == Status::Ok) {
        return QUrl(data(2).toString());
    }
    return {};
}

QString HaukCreateReply::groupPin() const
{
    if (status() == Status::Ok && rawData().size() == 5) {
        return data(3).toString();
    }
    return {};
}

QString HaukCreateReply::shareId() const
{
    if (status() == Status::Ok) {
        // always the last field
        return data(-1).toString();
    }
    return {};
}


QList<QUrl> HaukPostReply::urls() const
{
    if (status() == Status::Ok) {
        const auto baseUrl = data(1).toString().replace("%s"_L1, "%1");
        auto view = rawData() | std::views::drop(2) | std::views::transform([&baseUrl](const QStringView &url) { return QUrl(baseUrl.arg(url.toString())); });
        return {view.begin(), view.end()};
    }
    return {};
}
