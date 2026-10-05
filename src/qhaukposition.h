// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#pragma once

#include <QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>
#include <QtQmlIntegration/QtQmlIntegration>

class QHaukPosition
{
    Q_GADGET
    Q_PROPERTY(QGeoCoordinate position MEMBER position FINAL)
    Q_PROPERTY(qreal accuracy MEMBER accuracy FINAL)
    Q_PROPERTY(qreal speed MEMBER speed FINAL)
    Q_PROPERTY(qreal direction MEMBER direction FINAL)
    QML_VALUE_TYPE(qHaukPosition)
public:
    QHaukPosition() = default;
    QHaukPosition(const QGeoPositionInfo &position);
    QHaukPosition(const QGeoCoordinate &position, qreal accuracy, qreal speed);

    friend bool operator==(const QHaukPosition &, const QHaukPosition &) = default;

    QGeoCoordinate position = {};
    qreal accuracy = {};
    qreal speed = {};
    // direction of travel in degrees from north, -1 if unknown
    qreal direction = -1;
};
