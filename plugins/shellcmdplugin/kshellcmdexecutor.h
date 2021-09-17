/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSHELLCMDEXECUTOR_H
#define KSHELLCMDEXECUTOR_H

#include <QTextEdit>

namespace KDESu
{
class PtyProcess;
}
class QSocketNotifier;

class KShellCommandExecutor: public QTextEdit
{
    Q_OBJECT
public:
    explicit KShellCommandExecutor(const QString &command, QWidget *parent = nullptr);
    ~KShellCommandExecutor() override;
    int exec();
Q_SIGNALS:
    void finished();
public Q_SLOTS:
    void slotFinished();
protected:
    KDESu::PtyProcess *m_shellProcess;
    QString m_command;
    QSocketNotifier *m_readNotifier;
    QSocketNotifier *m_writeNotifier;
protected Q_SLOTS:
    void readDataFromShell();
    void writeDataToShell();
};

#endif // KSHELLCMDEXECUTOR_H
