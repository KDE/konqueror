/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000, 2007 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2003 Waldo Bastian <bastian@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/
#include "filegroupdetails.h"
#include "mimetypedata.h"

#include <QButtonGroup>
#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>

#include <KLocalizedString>

FileGroupDetails::FileGroupDetails(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *secondLayout = new QVBoxLayout(this);

    QGroupBox *autoEmbedBox = new QGroupBox(i18n("Left Click Action"));
    m_autoEmbed = new QButtonGroup(autoEmbedBox);
    secondLayout->addWidget(autoEmbedBox);
    // The order of those two items is very important. If you change it, fix typeslistitem.cpp !
    QRadioButton *r1 = new QRadioButton(i18n("Show file in embedded viewer"));
    QRadioButton *r2 = new QRadioButton(i18n("Show file in separate viewer"));
    QVBoxLayout *autoEmbedBoxLayout = new QVBoxLayout(autoEmbedBox);
    autoEmbedBoxLayout->addWidget(r1);
    autoEmbedBoxLayout->addWidget(r2);
    m_autoEmbed->addButton(r1, 0);
    m_autoEmbed->addButton(r2, 1);
    connect(m_autoEmbed, &QButtonGroup::idClicked, this, &FileGroupDetails::slotAutoEmbedClicked);

    autoEmbedBox->setWhatsThis(
        i18n("<p>Here you can configure what Konqueror should do when you click on a file belonging"
             " to this group in a file manager view.</p>"
             "<p>The file can either be displayed in Konqueror"
             " itself or in a separate application. You can change this setting for a"
             " specific file type in the 'Embedding' tab of the file type configuration.</p>"));

    secondLayout->addStretch();
}

void FileGroupDetails::setMimeTypeData(MimeTypeData *mimeTypeData)
{
    Q_ASSERT(mimeTypeData->isMeta());
    m_mimeTypeData = mimeTypeData;
    m_autoEmbed->button(m_mimeTypeData->autoEmbed())->setChecked(true);
}

void FileGroupDetails::slotAutoEmbedClicked(int button)
{
    if (!m_mimeTypeData) {
        return;
    }
    m_mimeTypeData->setAutoEmbed((MimeTypeData::AutoEmbed)button);
    Q_EMIT changed(true);
}

#include "moc_filegroupdetails.cpp"
