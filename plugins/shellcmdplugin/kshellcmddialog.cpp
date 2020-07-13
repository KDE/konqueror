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
#include <QDialogButtonBox>

// KDE
#include <KLocalizedString>
#include <KStandardGuiItem>

// Local
#include "kshellcmdexecutor.h"

KShellCommandDialog::KShellCommandDialog(const QString &title, const QString &command, QWidget *parent, bool modal)
    : QDialog(parent)
{
    setModal(modal);

    QLabel *label = new QLabel(title, this);
    m_shell = new KShellCommandExecutor(command, this);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close|QDialogButtonBox::Cancel, this);
    cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    closeButton = buttonBox->button(QDialogButtonBox::Close);
    closeButton->setDefault(true);

    label->resize(label->sizeHint());
    m_shell->resize(m_shell->sizeHint());

    QVBoxLayout *box = new QVBoxLayout(this);
    box->addWidget(label);
    box->addWidget(m_shell);
    box->setStretchFactor(m_shell, 1);
    box->addWidget(buttonBox);

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

    m_shell->exec();
    return exec();
}

