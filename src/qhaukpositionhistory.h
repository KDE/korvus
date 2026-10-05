// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>
#pragma once

#include "qhaukposition.h"

#include <QColor>
#include <QList>
#include <QString>
#include <QtQmlIntegration/QtQmlIntegration>

class QHaukPositionHistory
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name FINAL)
    Q_PROPERTY(QColor color MEMBER color FINAL)
    Q_PROPERTY(QList<QHaukPosition> positions MEMBER positions FINAL)
    Q_PROPERTY(QHaukPosition lastKnownPositions READ lastKnownPositions FINAL)
    QML_VALUE_TYPE(qHaukPositionHistory)
public:
    QHaukPositionHistory() = default;
    QHaukPositionHistory(const QString &name, const QList<QHaukPosition> &positions);

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
    AppendResult append(const QHaukPosition &position);

    /**
     * The latest position, or an invalid position if none is known yet
     */
    QHaukPosition lastKnownPositions() const;

    QString name = {};
    QColor color = {};
    QList<QHaukPosition> positions = {};
};
