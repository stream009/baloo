/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2010-14 Vishesh Handa <handa.vish@gmail.com>
   Copyright (C) 2010-2011 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "app.h"
#include "../priority.h"

#include <cassert>
#include <csignal>
#include <unistd.h>

#include <KAboutData>
#include <KLocalizedString>
#include <QStandardPaths>
#include <QByteArray>

#include <QApplication>
#include <QSessionManager>
#include <QFile>

void messageHandler(const QtMsgType type, const QMessageLogContext &context,
                    const QString &message)
{
    QFile log { "/tmp/baloo_extractor.log" };
    log.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts { &log };

    ts << qFormatLogMessage(type, context, message) << endl;

    if (type == QtFatalMsg) {
        abort();
    }
}

void handleSignal(const int num)
{
    qDebug() << "Receive signal:" << num;
    abort();
}

int main(int argc, char* argv[])
{
#if 0
    lowerIOPriority();
    setIdleSchedulingPriority();
    lowerPriority();
#endif

    KAboutData aboutData(QStringLiteral("baloo"), i18n("Baloo File Extractor"), QLatin1String(PROJECT_VERSION));
    aboutData.addAuthor(i18n("Vishesh Handa"), i18n("Maintainer"), QStringLiteral("vhanda@kde.org"), QStringLiteral("http://vhanda.in"));

    //QApplication::setDesktopSettingsAware(false);
    QApplication app(argc, argv);

    qSetMessagePattern("%{time hh:mm:ss} %{pid} %{function} %{message}");
    qInstallMessageHandler(messageHandler);
    qDebug() << "launched";

    for (auto sig = 1; sig <= 15; ++sig) {
        if (sig == SIGABRT) continue;
        if (sig == SIGSEGV) continue;
        if (sig == SIGPIPE) {
            signal(sig, SIG_IGN);
            //signal(sig, handleSignal);
        }
        else {
            signal(sig, handleSignal);
        }
    }

    KAboutData::setApplicationData(aboutData);

    //app.setQuitOnLastWindowClosed(false);
#if 0
    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);
#endif
    Baloo::App appObject;

    //kill(getpid(), SIGHUP);
    const auto rc = app.exec();
    qDebug() << "exit normally:" << rc;

    return rc;
}
