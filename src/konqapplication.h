/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_APPLICATION_H
#define KONQ_APPLICATION_H

#include "konqprivate_export.h"
#include <QApplication>

#include <KAboutData>

#include <QCommandLineParser>

#ifdef KActivities_FOUND
namespace KActivities {
    class Consumer;
}
#endif

class KonqMainWindow;
class QDBusMessage;
class KonqBrowser;

namespace KParts {
    class Part;
}

class KONQ_TESTS_EXPORT KonquerorApplication : public QApplication
{
    Q_OBJECT
public:
    KonquerorApplication(int &argc, char **argv);
    int start();

#ifdef KActivities_FOUND
    static QString currentActivity();
#endif

public slots:
    void slotReparseConfiguration();
signals:
    void configurationChanged();
    void aboutToConfigure();

private slots:
    void slotAddToCombo(const QString &url, const QDBusMessage &msg);
    void slotRemoveFromCombo(const QString &url, const QDBusMessage &msg);
    void slotComboCleared(const QDBusMessage &msg);

private:

    using WindowCreationResult = QPair<KonqMainWindow*, int>;

    void setupAboutData();
    void setupParser();
    int startFirstInstance();
    int performStart(const QString &workingDirectory, bool firstInstance = false);
    void restoreSession();
    void listSessions();
    int openSession(const QString &session);
    WindowCreationResult createEmptyWindow(bool firstInstance);
    void preloadWindow(const QStringList &args);
    WindowCreationResult createWindowsForUrlArguments(const QStringList &args, const QString &workingDirectory, KonqMainWindow *mainwin= nullptr);

    /**
     * @brief What to do when running Konqueror as root
     */
    enum KonquerorAsRootBehavior {
        NotRoot, //!< Konqueror is not being run as root
        PreventRunningAsRoot, //!< The user tried to run Konqueror as root but answered to exit when asked
        RunInDangerousMode //!< The user decided to run Konqueror as user enabling the `--no-sandbox` chromium flag
    };

    /**
     * @brief Checks whether Konqueror is being run as root and, if so, asks the user what to do
     * @return NotRoot if Konqueror isn't being run as root or PreventRunningAsRoot or RunInDangerousMode, according
     * to the user's choice, otherwise
     */
    static KonquerorAsRootBehavior checkRootBehavior();

private:
    KAboutData m_aboutData;
    QCommandLineParser m_parser;
    bool m_sessionRecoveryAttempted = false;
    KonquerorAsRootBehavior m_runningAsRootBehavior = NotRoot;
#ifdef KActivities_FOUND
    KActivities::Consumer* m_activityConsumer;
#endif

    KonqBrowser *m_browser;

};

#endif
