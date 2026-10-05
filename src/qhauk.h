// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#pragma once

#include "qhauksession.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

class QGeoPositionInfoSource;

class QHauk : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl host READ host WRITE setHost NOTIFY hostChanged FINAL)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged FINAL)
    Q_PROPERTY(QString nickName READ nickName WRITE setNickName NOTIFY nickNameChanged FINAL)

    QML_ELEMENT
    QML_SINGLETON
public:
    QHauk(QObject *parent = nullptr);
    ~QHauk();

    /**
     *
     * @param duration in minutes
     */
    Q_INVOKABLE QHaukSession *createSession();

    void setHost(const QUrl &url);
    QUrl host() const;
    void setPassword(const QString &password);
    QString password() const;
    void setNickName(const QString &nickName);
    QString nickName() const;

    QGeoPositionInfoSource *locationSource() const;

    Q_INVOKABLE void setClipboard(const QString &text) const;
    Q_INVOKABLE void copyLogToClipboard() const;

Q_SIGNALS:
    void hostChanged();
    void passwordChanged();
    void nickNameChanged();


private:
    QUrl m_host;
    QString m_password;
    QString m_nickName;
    QGeoPositionInfoSource *m_locationSource = nullptr;

    QList<QHaukSession *> m_sessions;
};
