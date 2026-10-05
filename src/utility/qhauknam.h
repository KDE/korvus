// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#pragma once

#include <QNetworkAccessManager>
#include <QQmlNetworkAccessManagerFactory>

class QHaukNam : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit QHaukNam(QObject *parent = nullptr);

    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = nullptr) override;
};

class QHaukNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override;
};
