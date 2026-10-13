// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#include "clipboardwatcher.h"

#include <QGuiApplication>
#include <QRegularExpression>

using namespace Qt::Literals::StringLiterals;

ClipboardWatcher::ClipboardWatcher(QObject *parent)
    : ClipboardWatcher(QGuiApplication::clipboard(), parent)
{
}

ClipboardWatcher::ClipboardWatcher(QClipboard *cb, QObject *parent)
    : QObject(parent)
{
    connect(cb, &QClipboard::dataChanged, this, [cb, this] {
        if (cb->ownsClipboard()) {
            return;
        }
        // Either the clipboard holds nothing but a share id, or it contains a share link anywhere in the text,
        // e.g. "You can follow my location share https://hauk.example.org/?L528-TFWZ".
        // The link must have the id as its query: http(s)://host[/path]/?XXXX-XXXX, not followed by more id characters.
        static const QRegularExpression haukIdRegex(u"(?|^\\s*([A-Za-z0-9]{4}-[A-Za-z0-9]{4})\\s*$"
                                                    "|https?://[^\\s/?#]+(?:/[^\\s?#]*)?/\\?([A-Za-z0-9]{4}-[A-Za-z0-9]{4})(?![A-Za-z0-9-]))"_s);
        const auto match = haukIdRegex.match(cb->text());
        const QString id = match.captured(1).toUpper();
        if (m_lastHaukId != id) {
            m_lastHaukId = id;
            if (!id.isEmpty()) {
                Q_EMIT haukIdDetected(id);
            }
        }
    });
}
