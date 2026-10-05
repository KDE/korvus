// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#pragma once

#include <QStringList>

/**
 * A simple QtMessageHandler.
 *
 * All messages are forwarded to the previously installed handler (so they still end up on
 * the console / os log) and the most recent ones are kept in memory.
 * Optionally the messages are persisted to a log file in the app data location, the log of
 * the previous run is kept so it can be recalled after a crash.
 */
namespace Utility::LogHandler {

/**
 * Installs the message handler.
 * Must be called after the application name was set, the persistence setting is read from QSettings.
 */
void install();

/**
 * Enables or disables persisting the log to logFilePath().
 * The setting is applied immediately and stored in QSettings so it is restored by install().
 */
void setPersistent(bool persistent);
bool isPersistent();

/**
 * The path of the log file of the current run.
 */
QString logFilePath();

/**
 * The path of the log file of the previous run.
 */
QString previousLogFilePath();

/**
 * The most recent messages of the current run, oldest first.
 */
QStringList recentMessages();

/**
 * Reads the persisted log.
 * @param previousRun Read the log of the previous run instead of the current one.
 */
QString readPersistedLog(bool previousRun = false);

}
