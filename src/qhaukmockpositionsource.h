// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#pragma once

#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QTimer>

/**
 * Position source for testing, moves in a circle around center
 */
class QHaukMockPositionSource : public QGeoPositionInfoSource
{
    Q_OBJECT
public:
    /**
     * Returns a mock source if QHAUK_MOCK_POSITION is set, nullptr otherwise.
     * The variable is either "1" or the center of the circle as "latitude,longitude"
     */
    static QHaukMockPositionSource *createFromEnvironment(QObject *parent);

    explicit QHaukMockPositionSource(const QGeoCoordinate &center, QObject *parent = nullptr);

    void setUpdateInterval(int msec) override;
    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const override;
    PositioningMethods supportedPositioningMethods() const override;
    int minimumUpdateInterval() const override;
    Error error() const override;

public Q_SLOTS:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

private:
    void step();

    QGeoCoordinate m_center;
    QGeoPositionInfo m_lastPosition;
    QTimer m_timer;
    // angle on the circle in degrees from north
    qreal m_angle = 0;
};
