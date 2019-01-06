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

#include <QFontDatabase>
#include <QTimer>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QDialogButtonBox>

#include <KGuiItem>
#include <KLocalizedString>
#include <KStandardGuiItem>
#include <QFontDatabase>

WebShortcutWidget::WebShortcutWidget(QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *titleLayout = new QHBoxLayout();
    mainLayout->addLayout(titleLayout);
    QLabel *iconLabel = new QLabel(this);
    QIcon wsIcon = QIcon::fromTheme(QStringLiteral("preferences-web-browser-shortcuts"));
    iconLabel->setPixmap(wsIcon.pixmap(22, 22));
    titleLayout->addWidget(iconLabel);
    m_searchTitleLabel = new QLabel(i18n("Set Uri Shortcuts"), this);
    QFont boldFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    boldFont.setBold(true);
    m_searchTitleLabel->setFont(boldFont);
    titleLayout->addWidget(m_searchTitleLabel);
    titleLayout->addStretch();

    QFormLayout *formLayout = new QFormLayout();
    mainLayout->addLayout(formLayout);

    QFont smallFont = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
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

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &WebShortcutWidget::okClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &WebShortcutWidget::cancelClicked);

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

