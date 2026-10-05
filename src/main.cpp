// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Hannah von Reth <vonreth@kde.org>

#include <QGuiApplication>
#include <QLocationPermission>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QtQuickControls2/QQuickStyle>

#include "utility/loghandler.h"
#include "utility/qhauknam.h"

namespace {
Q_LOGGING_CATEGORY(lcApp, "qhauk.app", QtDebugMsg)
}

void start()
{
    auto *engine = new QQmlApplicationEngine(qApp);
    QObject::connect(
        engine, &QQmlApplicationEngine::objectCreationFailed, qApp,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine->setNetworkAccessManagerFactory(new QHaukNetworkAccessManagerFactory);
    engine->loadFromModule("org.kde.hauk.app", "Main");
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("KDE");
    app.setOrganizationDomain("org.kde");
    app.setApplicationName("QHauk");

#ifdef Q_DEBUG
    Utility::LogHandler::install();
#endif

    QQuickStyle::setStyle("iOS");

    QLocationPermission permission;
    permission.setAccuracy(QLocationPermission::Precise);
    permission.setAvailability(QLocationPermission::Always);

    switch (app.checkPermission(permission)) {
    case Qt::PermissionStatus::Granted:
        qCDebug(lcApp) << "Location permission granted";
        start();
        break;
    case Qt::PermissionStatus::Undetermined:
        app.requestPermission(permission, &app, [](const QPermission &permission) {
            if (permission.status() == Qt::PermissionStatus::Granted) {
                qCDebug(lcApp) << "Location permission granted after requesting";
                start();
            } else {
                qCWarning(lcApp) << "Location permission was not granted";
            }
        });
        break;
    case Qt::PermissionStatus::Denied:
        qCWarning(lcApp) << "Location permission denied";
        break;
    }
    return app.exec();
}
