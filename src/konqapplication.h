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

class KonqMainWindow;
class QDBusMessage;

class KONQ_TESTS_EXPORT KonquerorApplication : public QApplication
{
    Q_OBJECT
public:
    KonquerorApplication(int &argc, char **argv);
    int start();

public slots:
    void slotReparseConfiguration();

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
    WindowCreationResult createWindowsForUrlArguments(const QStringList &args, const QString &workingDirectory);

private:
    KAboutData m_aboutData;
    QCommandLineParser m_parser;
    bool m_sessionRecoveryAttempted = false;

};

#endif
