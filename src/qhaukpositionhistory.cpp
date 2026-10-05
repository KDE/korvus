// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#include "qhaukpositionhistory.h"

#include <algorithm>

QHaukPositionHistory::QHaukPositionHistory(const QString &name, const QList<QHaukPosition> &positions)
    : name(name)
    // Fibonacci hashing spreads similar qHash values across the hue range, take the top 24 bits as hue
    , color(QColor::fromHslF(
          static_cast<float>((static_cast<quint64>(qHash(name)) * 0x9E3779B97F4A7C15ull) >> 40) / static_cast<float>(1u << 24), 0.55f, 0.65f, 1.0f))
    , positions(positions)
{
}

QHaukPositionHistory::AppendResult QHaukPositionHistory::append(const QHaukPosition &position)
{
    if (!position.position.isValid()) {
        return AppendResult::Rejected;
    }
    if (positions.size() >= 2) {
        // the last point floats with the current position, the one before is the last committed point.
        // Comparing against the committed point ensures a slow drift is still recorded.
        const QGeoCoordinate &anchor = positions.at(positions.size() - 2).position;
        if (anchor.isValid() && anchor.distanceTo(position.position) < std::max(MinimumDistance, position.accuracy)) {
            positions.last() = position;
            return AppendResult::Replaced;
        }
    }
    positions.append(position);
    return AppendResult::Appended;
}

QHaukPosition QHaukPositionHistory::lastKnownPositions() const
{
    return positions.isEmpty() ? QHaukPosition() : positions.last();
}
