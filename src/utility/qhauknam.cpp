// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#include "qhauknam.h"

#include <QtCore/quuid.h>

#include "httplogger.h"

using namespace Qt::Literals::StringLiterals;

QHaukNam::QHaukNam(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

QNetworkReply *QHaukNam::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QNetworkRequest newRequest(request);
    newRequest.setRawHeader("User-Agent"_ba, "org.kde.qhauk"_ba);
    newRequest.setRawHeader("X-Request-ID"_ba, QUuid::createUuid().toByteArray(QUuid::WithoutBraces));

    const auto reply = QNetworkAccessManager::createRequest(op, newRequest, outgoingData);
#ifdef QT_DEBUG
    OCC::HttpLogger::logRequest(reply, op, outgoingData);
#endif
    return reply;
}

QNetworkAccessManager *QHaukNetworkAccessManagerFactory::create(QObject *parent)
{
    return new QHaukNam(parent);
}
