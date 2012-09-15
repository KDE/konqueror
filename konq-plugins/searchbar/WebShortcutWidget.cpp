/* This file is part of the KDE project
 * Copyright (C) 2009 Fredy Yanardi <fyanardi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "WebShortcutWidget.h"

#include <QtCore/QTimer>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>

#include <KGlobalSettings>
#include <KIcon>
#include <KLocale>

WebShortcutWidget::WebShortcutWidget(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QHBoxLayout *titleLayout = new QHBoxLayout();
    mainLayout->addLayout(titleLayout);
    QLabel *iconLabel = new QLabel(this);
    KIcon wsIcon("preferences-web-browser-shortcuts");
    iconLabel->setPixmap(wsIcon.pixmap(22, 22));
    titleLayout->addWidget(iconLabel);
    m_searchTitleLabel = new QLabel(i18n("Set Uri Shortcuts"), this);
    QFont boldFont = KGlobalSettings::generalFont();
    boldFont.setBold(true);
    m_searchTitleLabel->setFont(boldFont);
    titleLayout->addWidget(m_searchTitleLabel);
    titleLayout->addStretch();

    QFormLayout *formLayout = new QFormLayout();
    mainLayout->addLayout(formLayout);

    QFont smallFont = KGlobalSettings::smallestReadableFont();
    m_nameLineEdit = new QLineEdit(this);
    m_nameLineEdit->setEnabled(false);
    m_nameLineEdit->setFont(smallFont);
    QLabel *nameLabel = new QLabel(i18n("Name:"), this);
    nameLabel->setFont(smallFont);
    formLayout->addRow(nameLabel, m_nameLineEdit);

    QLabel *shortcutsLabel = new QLabel(i18n("Shortcuts:"), this);
    shortcutsLabel->setFont(smallFont);
    m_wsLineEdit = new QLineEdit(this);
    m_wsLineEdit->setMinimumWidth(100);
    m_wsLineEdit->setFont(smallFont);
    formLayout->addRow(shortcutsLabel, m_wsLineEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    mainLayout->addLayout(buttonLayout);
    buttonLayout->addStretch();
    QPushButton *okButton = new QPushButton(i18n("OK"), this);
    okButton->setDefault(true);
    buttonLayout->addWidget(okButton);
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));

    QPushButton *cancelButton = new QPushButton(i18n("Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));

    setLayout(mainLayout);

    resize(minimumSizeHint());

    QTimer::singleShot(0, m_wsLineEdit, SLOT(setFocus()));
}

void WebShortcutWidget::show(const QString &openSearchName, const QString &fileName)
{
    m_wsLineEdit->clear();
    m_nameLineEdit->setText(openSearchName);
    m_fileName = fileName;

    QDialog::show();
}

void WebShortcutWidget::okClicked()
{
    hide();
    emit webShortcutSet(m_nameLineEdit->text(), m_wsLineEdit->text(), m_fileName);
}

void WebShortcutWidget::cancelClicked()
{
    hide();
}

#include "WebShortcutWidget.moc"

