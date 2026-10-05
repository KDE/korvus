// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#include "qhaukpositionhistory.h"

#include <QGeoPositionInfo>
#include <algorithm>

QHaukPositionHistory::QHaukPositionHistory(QObject *parent)
    : QObject(parent)
{
}

QHaukPositionHistory::QHaukPositionHistory(const QString &name, const QList<QGeoCoordinate> &positions, QObject *parent)
    : QObject(parent)
    , name(name)
    // Fibonacci hashing spreads similar qHash values across the hue range, take the top 24 bits as hue
    , color(QColor::fromHslF(
          static_cast<float>((static_cast<quint64>(qHash(name)) * 0x9E3779B97F4A7C15ull) >> 40) / static_cast<float>(1u << 24), 0.55f, 0.65f, 1.0f))
    , positions(positions)
{
}

QHaukPositionHistory::AppendResult QHaukPositionHistory::append(const QGeoCoordinate &position, qreal accuracy)
{
    if (!position.isValid()) {
        return AppendResult::Rejected;
    }

    if (positions.size() >= 2) {
        // the last point floats with the current position, the one before is the last committed point.
        // Comparing against the committed point ensures a slow drift is still recorded.
        const QGeoCoordinate &anchor = positions.at(positions.size() - 2);
        if (anchor.isValid() && anchor.distanceTo(position) < std::max(MinimumDistance, accuracy)) {
            positions.last() = position;
            Q_EMIT positionsChanged();
            return AppendResult::Replaced;
        }
    }
    positions.append(position);
    Q_EMIT positionsChanged();
    return AppendResult::Appended;
}

QHaukPositionHistory::AppendResult QHaukPositionHistory::append(const QGeoPositionInfo &info)
{
    auto optionalAttr = [&info](QGeoPositionInfo::Attribute attr, qreal fallback = {}) -> qreal {
        return info.hasAttribute(attr) ? info.attribute(attr) : fallback;
    };

    setAccuracy(optionalAttr(QGeoPositionInfo::HorizontalAccuracy));
    setSpeed(optionalAttr(QGeoPositionInfo::GroundSpeed));
    auto direction = optionalAttr(QGeoPositionInfo::Direction, -1);
    if (direction < 0 && positions.size() >= 2) {
        // from the last committed point, the last point floats with the current position
        direction = positions.at(positions.size() - 2).azimuthTo(info.coordinate());
    }
    if (direction >= 0) {
        setDirection(direction);
    }
    return append(info.coordinate(), m_accuracy);
}

const QGeoCoordinate &QHaukPositionHistory::lastKnownPosition() const
{
    static const QGeoCoordinate InvalidCoordinate;
    return positions.isEmpty() ? InvalidCoordinate : positions.last();
}

void QHaukPositionHistory::setAccuracy(qreal accuracy)
{
    if (accuracy != m_accuracy) {
        m_accuracy = accuracy;
        Q_EMIT accuracyChanged();
    }
}

void QHaukPositionHistory::setSpeed(qreal speed)
{
    if (speed != m_speed) {
        m_speed = speed;
        Q_EMIT speedChanged();
    }
}

void QHaukPositionHistory::setDirection(qreal direction)
{
    if (direction != m_direction) {
        m_direction = direction;
        Q_EMIT directionChanged();
    }
}
qreal QHaukPositionHistory::accuracy() const
{
    return m_accuracy;
}

qreal QHaukPositionHistory::speed() const
{
    return m_speed;
}

qreal QHaukPositionHistory::direction() const
{
    return m_direction;
}
