// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#include "loghandler.h"

#include <QDir>
#include <QFile>
#include <QMutex>
#include <QSettings>
#include <QStandardPaths>

#include <cstdio>
#include <deque>

using namespace Qt::Literals::StringLiterals;

namespace {
constexpr size_t MaxRecentMessages = 1000;
const auto PersistKey = u"Logging/persist"_s;

struct State
{
    QMutex mutex;
    QtMessageHandler previousHandler = nullptr;
    std::deque<QString> recent;
    QFile file;
    bool rotated = false;
};

State &state()
{
    static State s;
    return s;
}

QString logDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs"_L1;
}

// must be called with the mutex locked
void openLogFile(State &s)
{
    if (s.file.isOpen()) {
        return;
    }
    QDir().mkpath(logDir());
    s.file.setFileName(Utility::LogHandler::logFilePath());
    // keep the log of the previous run, but only rotate once per process
    QIODevice::OpenMode mode = QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text;
    if (!s.rotated) {
        s.rotated = true;
        QFile::remove(Utility::LogHandler::previousLogFilePath());
        QFile::rename(s.file.fileName(), Utility::LogHandler::previousLogFilePath());
        mode |= QIODevice::Truncate;
        mode &= ~QIODevice::Append;
    }
    if (!s.file.open(mode)) {
        std::fprintf(stderr, "Failed to open log file %s: %s\n", qPrintable(s.file.fileName()), qPrintable(s.file.errorString()));
        return;
    }
    // persist what we already collected in this run
    for (const auto &msg : s.recent) {
        s.file.write(msg.toUtf8() + '\n');
    }
    s.file.flush();
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // prevent recursion if something in here logs
    static thread_local bool inHandler = false;
    auto &s = state();
    if (s.previousHandler) {
        s.previousHandler(type, context, msg);
    } else {
        std::fprintf(stderr, "%s\n", qPrintable(qFormatLogMessage(type, context, msg)));
    }
    if (inHandler) {
        return;
    }
    inHandler = true;
    const QString formatted = qFormatLogMessage(type, context, msg);
    {
        QMutexLocker lock(&s.mutex);
        s.recent.push_back(formatted);
        if (s.recent.size() > MaxRecentMessages) {
            s.recent.pop_front();
        }
        if (s.file.isOpen()) {
            s.file.write(formatted.toUtf8() + '\n');
            s.file.flush();
        }
    }
    inHandler = false;
}
}

namespace Utility {

void LogHandler::install()
{
    auto &s = state();
    qSetMessagePattern(u"%{time yyyy-MM-dd hh:mm:ss.zzz} [ %{type} %{category} ]%{if-debug} %{function}:%{line}%{endif}: %{message}"_s);
    {
        QMutexLocker lock(&s.mutex);
        if (QSettings().value(PersistKey, false).toBool()) {
            openLogFile(s);
        }
    }
    s.previousHandler = qInstallMessageHandler(messageHandler);
}

void LogHandler::setPersistent(bool persistent)
{
    QSettings().setValue(PersistKey, persistent);
    auto &s = state();
    QMutexLocker lock(&s.mutex);
    if (persistent) {
        openLogFile(s);
    } else {
        s.file.close();
    }
}

bool LogHandler::isPersistent()
{
    return QSettings().value(PersistKey, false).toBool();
}

QString LogHandler::logFilePath()
{
    return logDir() + "/qhauk.log"_L1;
}

QString LogHandler::previousLogFilePath()
{
    return logDir() + "/qhauk.previous.log"_L1;
}

QStringList LogHandler::recentMessages()
{
    auto &s = state();
    QMutexLocker lock(&s.mutex);
    return {s.recent.cbegin(), s.recent.cend()};
}

QString LogHandler::readPersistedLog(bool previousRun)
{
    QFile f(previousRun ? previousLogFilePath() : logFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(f.readAll());
}

}
