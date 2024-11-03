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
    m_screensModel(screensModel),
    m_defaultLineModel(new QStandardItemModel(this)),
    m_model(new QConcatenateTablesProxyModel(this))
{
    m_ui->setupUi(this);

    QString urlString = url.toDisplayString(QUrl::RemoveUserInfo|QUrl::RemoveQuery|QUrl::RemoveFragment);
    m_ui->label->setText(i18n("Do you want to allow <tt>%1</tt> to capture the contents of your screen?", urlString));

    m_defaultLineModel->appendRow(new QStandardItem(i18n("Choose window or screen to capture")));
    m_model->addSourceModel(m_defaultLineModel);
    m_model->addSourceModel(m_windowsModel);
    m_model->addSourceModel(m_screensModel);
    m_ui->choicesCombo->setModel(m_model);
    updateOkStatus();

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(i18nc("Allow a web page to capture the screen", "Allow"));
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(i18nc("Block a web page from capturing the screen", "Block"));

    connect(m_ui->choicesCombo, &QComboBox::currentIndexChanged, this, &CaptureSourceChooserDlg::updateOkStatus);
}

CaptureSourceChooserDlg::~CaptureSourceChooserDlg()
{
}

QModelIndex CaptureSourceChooserDlg::currentSourceIndex() const
{
    return m_model->mapToSource(m_ui->choicesCombo->view()->currentIndex());
}

QModelIndex WebEngine::CaptureSourceChooserDlg::choice() const
{
    QModelIndex sourceIdx = currentSourceIndex();
    return sourceIdx.model() == m_defaultLineModel ? QModelIndex() : sourceIdx;
}

void CaptureSourceChooserDlg::updateOkStatus()
{
    QModelIndex idx = currentSourceIndex();
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(idx.model() != m_defaultLineModel);
}
