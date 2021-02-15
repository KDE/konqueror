/*  This file is part of the KDE project
    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include "kshellcmdexecutor.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include <QSocketNotifier>
#include <QInputDialog>
#include <QFontDatabase>

#include <kdesu/process.h>
#include <KLocalizedString>


KShellCommandExecutor::KShellCommandExecutor(const QString &command, QWidget *parent)
    : QTextEdit(parent)
    , m_shellProcess(nullptr)
    , m_command(command)
    , m_readNotifier(nullptr)
    , m_writeNotifier(nullptr)
{
    setAcceptRichText(false);
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setReadOnly(true);
}

KShellCommandExecutor::~KShellCommandExecutor()
{
    if (m_shellProcess != nullptr) {
        ::kill(m_shellProcess->pid() + 1, SIGTERM);
        delete m_shellProcess;
    };
}

int KShellCommandExecutor::exec()
{
    //qCDebug(KONQUEROR_LOG)<<"---------- KShellCommandExecutor::exec()";
    setText(QLatin1String(""));
    if (m_shellProcess != nullptr) {
        ::kill(m_shellProcess->pid(), SIGTERM);
        delete m_shellProcess;
    };
    delete m_readNotifier;
    delete m_writeNotifier;

    m_shellProcess = new KDESu::PtyProcess();
    m_shellProcess->setTerminal(true);

    QList<QByteArray> args;
    args += "-c";
    args += m_command.toLocal8Bit();
    //qCDebug(KONQUEROR_LOG)<<"------- executing: "<<m_command.toLocal8Bit();

    QByteArray shell(getenv("SHELL"));
    if (shell.isEmpty()) {
        shell = "sh";
    }

    int ret = m_shellProcess->exec(shell, args);
    if (ret < 0) {
        //qCDebug(KONQUEROR_LOG)<<"could not execute";
        delete m_shellProcess;
        m_shellProcess = nullptr;
        return 0;
    }

    m_readNotifier = new QSocketNotifier(m_shellProcess->fd(), QSocketNotifier::Read, this);
    m_writeNotifier = new QSocketNotifier(m_shellProcess->fd(), QSocketNotifier::Write, this);
    m_writeNotifier->setEnabled(false);
    connect(m_readNotifier, &QSocketNotifier::activated, this, &KShellCommandExecutor::readDataFromShell);
    connect(m_writeNotifier, &QSocketNotifier::activated, this, &KShellCommandExecutor::writeDataToShell);

    return 1;
}

void KShellCommandExecutor::readDataFromShell()
{
    //qCDebug(KONQUEROR_LOG)<<"--------- reading ------------";
    char buffer[16 * 1024];
    int bytesRead =::read(m_shellProcess->fd(), buffer, 16 * 1024 - 1);
    //0-terminate the buffer
    //process exited
    if (bytesRead <= 0) {
        slotFinished();
    } else if (bytesRead > 0) {
        //qCDebug(KONQUEROR_LOG)<<"***********************\n"<<buffer<<"###################\n";
        buffer[bytesRead] = '\0';
        this->append(QString::fromLocal8Bit(buffer));
        setAcceptRichText(false);
    };
}

void KShellCommandExecutor::writeDataToShell()
{
    //qCDebug(KONQUEROR_LOG)<<"--------- writing ------------";
    bool ok;
    QString str = QInputDialog::getText(this,
                                        QString(),
                                        i18n("Input Required:"),
                                        QLineEdit::Normal,
                                        QString(),
                                        &ok);
    if (ok) {
        QByteArray input = str.toLocal8Bit();
        ::write(m_shellProcess->fd(), input, input.length());
        ::write(m_shellProcess->fd(), "\n", 1);
    } else {
        slotFinished();
    }

    if (m_writeNotifier) {
        m_writeNotifier->setEnabled(false);
    }
}

void KShellCommandExecutor::slotFinished()
{
    setAcceptRichText(false);
    if (m_shellProcess != nullptr) {
        delete m_readNotifier;
        m_readNotifier = nullptr;
        delete m_writeNotifier;
        m_writeNotifier = nullptr;

        //qCDebug(KONQUEROR_LOG)<<"slotFinished: pid: "<<m_shellProcess->pid();
        ::kill(m_shellProcess->pid() + 1, SIGTERM);
        ::kill(m_shellProcess->pid(), SIGTERM);
    };
    delete m_shellProcess;
    m_shellProcess = nullptr;
    Q_EMIT finished();
}

