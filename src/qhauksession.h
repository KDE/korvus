// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#pragma once

#include "qhaukpositionhistory.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

#include <chrono>
#include <memory>

class QHauk;
class QHaukSessionPrivate;

class QHaukSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url NOTIFY isSharingChanged FINAL)
    Q_PROPERTY(bool isSharing READ isSharing NOTIFY isSharingChanged FINAL)
    Q_PROPERTY(QString shareId READ shareId NOTIFY isSharingChanged FINAL)

    Q_PROPERTY(QHaukPositionHistory *positions READ positions NOTIFY positionsChanged FINAL)
    Q_PROPERTY(QList<QHaukPositionHistory *> groupPositions READ groupPositions NOTIFY groupPositionsChanged FINAL)
    Q_PROPERTY(int duration READ durationInt WRITE setDurationInt NOTIFY durationChanged FINAL)
    Q_PROPERTY(ShareType shareType READ shareType WRITE setShareType NOTIFY shareTypeChanged FINAL)
    Q_PROPERTY(QString pin READ pin WRITE setPin NOTIFY pinChanged FINAL)

    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    QML_UNCREATABLE("Must create via QHauk")
    QML_ELEMENT
public:
    enum class ShareType {
        Solo,
        Group,
        JoinGroup // TODO: can we cover that with Group?
    };
    Q_ENUM(ShareType)

    QHaukSession(QHauk *parent);
    ~QHaukSession() override;

    QUrl url() const;
    QString pin() const;
    void setPin(const QString &pin);
    QString shareId() const;

    /**
     * The positions of the user
     */
    QHaukPositionHistory *positions() const;

    /**
     *  All positions of the group including the user's position
     */
    const QList<QHaukPositionHistory *> &groupPositions() const;

    bool isSharing() const;

    std::chrono::minutes duration() const;
    void setDuration(const std::chrono::minutes &duration);

    void setShareType(ShareType shareType);
    ShareType shareType() const;

    QString error() const;

Q_SIGNALS:
    void positionsChanged();
    void groupPositionsChanged();
    void isSharingChanged();
    void durationChanged();
    void shareTypeChanged();
    void pinChanged();
    void errorChanged();


public Q_SLOTS:
    void stopSharing();
    void startSharing();
    void requestGroupPositionsUpdate();

private:
    int durationInt() const;
    void setDurationInt(int duration);

    std::unique_ptr<QHaukSessionPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QHaukSession)
};
