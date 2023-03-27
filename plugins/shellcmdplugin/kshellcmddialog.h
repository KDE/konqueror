/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSHELLCMDDIALOG_H
#define KSHELLCMDDIALOG_H

#include <QDialog>

class KShellCommandExecutor;
class QPushButton;

class KShellCommandDialog: public QDialog
{
    Q_OBJECT
public:
    KShellCommandDialog(const QString &title, const QString &command, QWidget *parent = nullptr, bool modal = false);
    ~KShellCommandDialog() override;
    //blocking
    int executeCommand();
protected:

    KShellCommandExecutor *m_shell;
    QPushButton *cancelButton;
    QPushButton *closeButton;
protected Q_SLOTS:
    void disableStopButton();
    void slotClose();
};

#endif // KSHELLCMDDIALOG_H
