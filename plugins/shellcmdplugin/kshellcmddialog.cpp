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

// Own
#include "kshellcmddialog.h"

// Qt
#include <QLayout>
#include <QLabel>
#include <QPushButton>

// KDE
#include <KLocalizedString>
#include <KStandardGuiItem>

// Local
#include "kshellcmdexecutor.h"

KShellCommandDialog::KShellCommandDialog(const QString &title, const QString &command, QWidget *parent, bool modal)
    : KDialog(parent)
{
    setModal(modal);
    setButtons(KDialog::None);
    QWidget *w = new QWidget(this);
    QVBoxLayout *box = new QVBoxLayout;
    w->setLayout(box);
    setMainWidget(w);

    QLabel *label = new QLabel(title, this);
    m_shell = new KShellCommandExecutor(command, this);

    cancelButton = new QPushButton(this);
    KGuiItem::assign(cancelButton, KStandardGuiItem::cancel());
    closeButton = new QPushButton(this);
    KGuiItem::assign(closeButton, KStandardGuiItem::close());
    closeButton->setDefault(true);

    label->resize(label->sizeHint());
    m_shell->resize(m_shell->sizeHint());
    closeButton->setFixedSize(closeButton->sizeHint());
    cancelButton->setFixedSize(cancelButton->sizeHint());

    box->addWidget(label, 0);
    box->addWidget(m_shell, 1);

    QHBoxLayout *hlayout = new QHBoxLayout();
    box->addLayout(hlayout);
    hlayout->addWidget(cancelButton);
    hlayout->addWidget(closeButton);

    m_shell->setFocus();

    connect(cancelButton, &QAbstractButton::clicked, m_shell, &KShellCommandExecutor::slotFinished);
    connect(m_shell, &KShellCommandExecutor::finished, this, &KShellCommandDialog::disableStopButton);
    connect(closeButton, &QAbstractButton::clicked, this, &KShellCommandDialog::slotClose);
}

KShellCommandDialog::~KShellCommandDialog()
{
    delete m_shell;
    m_shell = nullptr;
}

void KShellCommandDialog::disableStopButton()
{
    cancelButton->setEnabled(false);
}

void KShellCommandDialog::slotClose()
{
    delete m_shell;
    m_shell = nullptr;
    accept();
}

//blocking
int KShellCommandDialog::executeCommand()
{
    if (m_shell == nullptr) {
        return 0;
    }
    //kDebug()<<"---------- KShellCommandDialog::executeCommand()";
    m_shell->exec();
    return exec();
}

