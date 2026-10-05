// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#pragma once


#include <QColor>
#include <QGeoCoordinate>
#include <QList>
#include <QString>
#include <QtQmlIntegration/QtQmlIntegration>


class QGeoPositionInfo;
class QHaukPositionHistory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(QColor color MEMBER color CONSTANT FINAL)
    Q_PROPERTY(QList<QGeoCoordinate> positions MEMBER positions NOTIFY positionsChanged FINAL)
    Q_PROPERTY(QGeoCoordinate lastKnownPosition READ lastKnownPosition NOTIFY positionsChanged FINAL)
    Q_PROPERTY(qreal accuracy READ accuracy WRITE setAccuracy NOTIFY accuracyChanged FINAL)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged FINAL)
    Q_PROPERTY(qreal direction READ direction WRITE setDirection NOTIFY directionChanged FINAL)
    QML_ELEMENT
public:
    QHaukPositionHistory(QObject *parent = nullptr);
    QHaukPositionHistory(const QString &name, const QList<QGeoCoordinate> &positions, QObject *parent = nullptr);

    /**
     * Minimal distance in meters a position has to move to be recorded as a new point
     */
    static constexpr qreal MinimumDistance = 10;

    enum class AppendResult {
        Rejected, // invalid position, nothing changed
        Replaced, // the last point was updated in place
        Appended, // a new point was recorded
    };

    /**
     * Appends position, if the movement since the previous point is below MinimumDistance
     * or the reported accuracy the last point is replaced instead.
     * This keeps the latest position exact while stationary periods collapse into two points.
     */
    AppendResult append(const QGeoCoordinate &position, qreal accuracy);
    AppendResult append(const QGeoPositionInfo &info);

    /**
     * The latest position, or an invalid position if none is known yet
     */
    const QGeoCoordinate &lastKnownPosition() const;

    QString name = {};
    QColor color = {};
    QList<QGeoCoordinate> positions = {};

    void setAccuracy(qreal accuracy);
    void setSpeed(qreal speed);
    void setDirection(qreal direction);

    qreal accuracy() const;
    qreal speed() const;
    qreal direction() const;

Q_SIGNALS:
    void positionsChanged();
    void directionChanged();
    void accuracyChanged();
    void speedChanged();

private:
    qreal m_accuracy = {};
    qreal m_speed = {};
    qreal m_direction = -1;
};
