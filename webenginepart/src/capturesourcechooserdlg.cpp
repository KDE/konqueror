/*
  This file is part of the KDE project
  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "capturesourcechooserdlg.h"
#include "ui_capturesourcechooserdlg.h"

#include <QConcatenateTablesProxyModel>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QPushButton>

using namespace WebEngine;

CaptureSourceChooserDlg::CaptureSourceChooserDlg(const QUrl &url, QAbstractListModel *windowsModel, QAbstractListModel *screensModel, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::CaptureSourceChooserDlg),
    m_windowsModel(windowsModel),
    m_screensModel(screensModel)
{
    m_ui->setupUi(this);

    QString urlString = url.toDisplayString(QUrl::RemoveUserInfo|QUrl::RemoveQuery|QUrl::RemoveFragment);
    m_ui->label->setText(i18n("Do you want to allow <tt>%1</tt> to capture the contents of your screen?", urlString));

    m_shareScreenBtn = new QPushButton(i18nc("Choose to share a screen", "Share Screen"), m_ui->buttonBox);
    connect(m_shareScreenBtn, &QPushButton::clicked, this, [this]{m_choice = Choice::ShareScreen;});
    m_shareWindowBtn = new QPushButton(i18nc("Choose to share a window", "Share Window"), m_ui->buttonBox);
    connect(m_shareWindowBtn, &QPushButton::clicked, this, [this]{m_choice = Choice::ShareWindow;});
    m_ui->buttonBox->addButton(m_shareWindowBtn, QDialogButtonBox::AcceptRole);
    m_ui->buttonBox->addButton(m_shareScreenBtn, QDialogButtonBox::AcceptRole);
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(i18nc("Don't share any part of the screen", "Share nothing"));

    m_ui->windowCombo->setModel(m_windowsModel);
    m_ui->windowCombo->setCurrentIndex(-1);
    m_ui->screenCombo->setModel(m_screensModel);
    m_ui->screenCombo->setCurrentIndex(-1);
    updateShareBtnStatus();

    connect(m_ui->windowCombo, &QComboBox::currentIndexChanged, this, &CaptureSourceChooserDlg::updateShareBtnStatus);
    connect(m_ui->screenCombo, &QComboBox::currentIndexChanged, this, &CaptureSourceChooserDlg::updateShareBtnStatus);
}

CaptureSourceChooserDlg::~CaptureSourceChooserDlg()
{
}

QModelIndex WebEngine::CaptureSourceChooserDlg::choice() const
{
    if (m_choice == Choice::ShareNone) {
        return {};
    }
    QComboBox *currentBox = m_choice == Choice::ShareWindow ? m_ui->windowCombo : m_ui->screenCombo;
    return currentBox->view()->currentIndex();
}

void CaptureSourceChooserDlg::updateShareBtnStatus()
{
    m_shareScreenBtn->setEnabled(m_ui->screenCombo->currentIndex() >= 0);
    m_shareWindowBtn->setEnabled(m_ui->windowCombo->currentIndex() >= 0);
}
