// This file is part of the KDE project
// SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "editspeeddialentrydlg.h"
#include "ui_editspeeddialentrydlg.h"

#include "speeddialconfig.h"

#include <QRadioButton>

#include <KIconButton>
#include <KUrlRequester>
#include <KUriFilter>

using namespace Qt::Literals::StringLiterals;

EditSpeedDialEntryDlg::EditSpeedDialEntryDlg(QWidget* parent) : QDialog(parent),
    m_ui(new Ui::EditSpeedDialEntryDlg)
{
    m_ui->setupUi(this);
    connect(m_ui->buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_ui->nameWidget, &QLineEdit::textChanged, this, &EditSpeedDialEntryDlg::updateOkBtn);
    connect(m_ui->urlWidget, &KUrlRequester::textChanged, this, &EditSpeedDialEntryDlg::updateOkBtn);
    connect(m_ui->remoteIconWidget, &QLineEdit::textChanged, this, &EditSpeedDialEntryDlg::updateOkBtn);
    connect(m_ui->localIconBtn, &KIconButton::iconChanged, this, &EditSpeedDialEntryDlg::updateOkBtn);
    connect(m_ui->useFaviconBtn, &QRadioButton::toggled, this, &EditSpeedDialEntryDlg::updateOkBtn);
    connect(m_ui->useLocalIconBtn, &QRadioButton::toggled, this, &EditSpeedDialEntryDlg::updateOkBtn);
    connect(m_ui->useRemoteIconBtn, &QRadioButton::toggled, this, &EditSpeedDialEntryDlg::updateOkBtn);

    connect(m_ui->useLocalIconBtn, &QRadioButton::toggled, this, [this](bool on){m_ui->localIconBtn->setEnabled(on);});
    connect(m_ui->useRemoteIconBtn, &QRadioButton::toggled, this, [this](bool on){m_ui->remoteIconWidget->setEnabled(on);});

    m_ui->localIconBtn->setIconType(KIconLoader::Desktop, KIconLoader::Any, true);

    updateOkBtn();
}

EditSpeedDialEntryDlg::EditSpeedDialEntryDlg(const Entry &entry, QWidget *parent) : EditSpeedDialEntryDlg(parent)
{
    m_ui->nameWidget->setText(entry.name);
    m_ui->urlWidget->setText(entry.url.toString());
    if (entry.iconUrl.isEmpty()) {
        m_ui->useFaviconBtn->setChecked(true);
    } else if (entry.iconUrl.isLocalFile() || entry.iconUrl.scheme().isEmpty()) {
        m_ui->useLocalIconBtn->setChecked(true);
        m_ui->localIconBtn->setIcon(entry.iconUrl.path());
    } else {
        m_ui->useRemoteIconBtn->setChecked(true);
        m_ui->remoteIconWidget->setText(entry.iconUrl.toString());
    }
}

EditSpeedDialEntryDlg::~EditSpeedDialEntryDlg()
{
}

EditSpeedDialEntryDlg::MaybeEntry EditSpeedDialEntryDlg::entry() const
{
    if (!m_ui->buttons->button(QDialogButtonBox::Ok)->isEnabled()) {
        return std::nullopt;
    }
    QString name = m_ui->nameWidget->text();
    QUrl url = m_ui->urlWidget->url();
    QUrl iconUrl;
    if (m_ui->useRemoteIconBtn->isChecked()) {
        iconUrl = QUrl(m_ui->remoteIconWidget->text());
    } else if (m_ui->useLocalIconBtn->isChecked()) {
        QString icon = m_ui->localIconBtn->icon();
        iconUrl = QFileInfo(icon).isAbsolute() ? QUrl::fromLocalFile(icon) : QUrl(icon);
    } //There's no need for an "else" clause since for favicons iconUrl must be empty
    return {{name, url, iconUrl}};
}

EditSpeedDialEntryDlg::MaybeEntry EditSpeedDialEntryDlg::newEntry(QWidget* parent)
{
    EditSpeedDialEntryDlg dlg(parent);
    return dlg.exec() == QDialog::Accepted ? dlg.entry() : std::nullopt;
}

void EditSpeedDialEntryDlg::updateOkBtn()
{
    bool enabled = !m_ui->nameWidget->text().isEmpty() && !m_ui->urlWidget->text().isEmpty();
    if (m_ui->useLocalIconBtn->isChecked() && m_ui->localIconBtn->icon().isEmpty()) {
        enabled = false;
    } else if (m_ui->useRemoteIconBtn->isChecked() && !QUrl(m_ui->remoteIconWidget->text()).isValid()) {
        enabled = false;
    }
    m_ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

void EditSpeedDialEntryDlg::accept()
{
    QString chosenUrl = m_ui->urlWidget->text();
    bool changed = KUriFilter::self()->filterUri(chosenUrl, {u"kshorturifilter"_s, u"fixuphosturifilter"_s});
    if (changed) {
        m_ui->urlWidget->setText(chosenUrl);
    }
    QDialog::accept();
}
