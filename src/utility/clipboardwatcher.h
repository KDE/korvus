// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#pragma once

#include <QClipboard>
#include <QObject>

#include <QtQmlIntegration/QtQmlIntegration>

class ClipboardWatcher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit ClipboardWatcher(QObject *parent = nullptr);
    explicit ClipboardWatcher(QClipboard *cb, QObject *parent = nullptr);

Q_SIGNALS:
    void haukIdDetected(const QString &id);

private:
    QString m_lastHaukId;
};
