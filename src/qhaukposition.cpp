// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#include "qhaukposition.h"

QHaukPosition::QHaukPosition(const QGeoPositionInfo &position)
    : position(position.coordinate())
{
    auto optionalAttr = [&position](QGeoPositionInfo::Attribute attr) -> qreal {
        if (position.hasAttribute(attr)) {
            switch (attr) {
            case QGeoPositionInfo::GroundSpeed:
                return position.attribute(attr);
                break;
            case QGeoPositionInfo::HorizontalAccuracy:
                return position.attribute(attr);
                break;
            default:
                Q_UNREACHABLE();
            }
        }
        return {};
    };

    speed = optionalAttr(QGeoPositionInfo::GroundSpeed);
    accuracy = optionalAttr(QGeoPositionInfo::HorizontalAccuracy);
}

QHaukPosition::QHaukPosition(const QGeoCoordinate &position, qreal accuracy, qreal speed)
    : position(position)
    , accuracy(accuracy)
    , speed(speed)
{
}
