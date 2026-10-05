// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#include "qhaukposition.h"

QHaukPosition::QHaukPosition(const QGeoPositionInfo &position)
    : position(position.coordinate())
{
    auto optionalAttr = [&position](QGeoPositionInfo::Attribute attr, qreal fallback = {}) -> qreal {
        return position.hasAttribute(attr) ? position.attribute(attr) : fallback;
    };

    speed = optionalAttr(QGeoPositionInfo::GroundSpeed);
    accuracy = optionalAttr(QGeoPositionInfo::HorizontalAccuracy);
    direction = optionalAttr(QGeoPositionInfo::Direction, -1);
}

QHaukPosition::QHaukPosition(const QGeoCoordinate &position, qreal accuracy, qreal speed)
    : position(position)
    , accuracy(accuracy)
    , speed(speed)
{
}
