// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#include "qhaukmockpositionsource.h"

#include <QDateTime>

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Qt::Literals::StringLiterals;

namespace {
constexpr qreal Radius = 300; // meters
constexpr qreal Speed = 5; // m/s
constexpr qreal Accuracy = 5; // meters
constexpr int MinimumInterval = 1000; // ms
}

QHaukMockPositionSource *QHaukMockPositionSource::createFromEnvironment(QObject *parent)
{
    const QString value = qEnvironmentVariable("QHAUK_MOCK_POSITION");
    if (value.isEmpty() || value == "0"_L1) {
        return nullptr;
    }
    // default to Berlin
    QGeoCoordinate center(52.5200, 13.4050);
    if (const auto parts = value.split(u','); parts.size() == 2) {
        bool latOk = false;
        bool lonOk = false;
        const QGeoCoordinate coordinate(parts[0].trimmed().toDouble(&latOk), parts[1].trimmed().toDouble(&lonOk));
        if (latOk && lonOk && coordinate.isValid()) {
            center = coordinate;
        }
    }
    return new QHaukMockPositionSource(center, parent);
}

QHaukMockPositionSource::QHaukMockPositionSource(const QGeoCoordinate &center, QObject *parent)
    : QGeoPositionInfoSource(parent)
    , m_center(center)
{
    setUpdateInterval(MinimumInterval);
    connect(&m_timer, &QTimer::timeout, this, &QHaukMockPositionSource::step);
}

void QHaukMockPositionSource::setUpdateInterval(int msec)
{
    QGeoPositionInfoSource::setUpdateInterval(std::max(msec, MinimumInterval));
    m_timer.setInterval(updateInterval());
}

QGeoPositionInfo QHaukMockPositionSource::lastKnownPosition(bool) const
{
    return m_lastPosition;
}

QGeoPositionInfoSource::PositioningMethods QHaukMockPositionSource::supportedPositioningMethods() const
{
    return AllPositioningMethods;
}

int QHaukMockPositionSource::minimumUpdateInterval() const
{
    return MinimumInterval;
}

QGeoPositionInfoSource::Error QHaukMockPositionSource::error() const
{
    return NoError;
}

void QHaukMockPositionSource::startUpdates()
{
    m_timer.start();
}

void QHaukMockPositionSource::stopUpdates()
{
    m_timer.stop();
}

void QHaukMockPositionSource::requestUpdate(int)
{
    QTimer::singleShot(0, this, &QHaukMockPositionSource::step);
}

void QHaukMockPositionSource::step()
{
    // advance by the distance travelled since the last step
    const qreal distance = Speed * updateInterval() / 1000.0;
    m_angle = std::fmod(m_angle + distance / Radius * 180 / std::numbers::pi, 360);

    QGeoPositionInfo info(m_center.atDistanceAndAzimuth(Radius, m_angle), QDateTime::currentDateTimeUtc());
    info.setAttribute(QGeoPositionInfo::GroundSpeed, Speed);
    info.setAttribute(QGeoPositionInfo::HorizontalAccuracy, Accuracy);
    // moving clockwise, tangent to the circle
    info.setAttribute(QGeoPositionInfo::Direction, std::fmod(m_angle + 90, 360));
    m_lastPosition = info;
    Q_EMIT positionUpdated(info);
}
