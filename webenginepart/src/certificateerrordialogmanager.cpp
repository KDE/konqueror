/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "certificateerrordialogmanager.h"
#include "webenginepage.h"
#include "webenginepartcertificateerrordlg.h"
#include "webengineview.h"

#include <algorithm>


#include <KSharedConfig>
#include <KConfigGroup>

using namespace KonqWebEnginePart;

CertificateErrorDialogManager::CertificateErrorDialogManager(QObject *parent) : QObject(parent)
{
}

CertificateErrorDialogManager::~CertificateErrorDialogManager()
{
}

bool CertificateErrorDialogManager::handleCertificateError(const QWebEngineCertificateError& _ce, WebEnginePage* page)
{
    QWebEngineCertificateError ce(_ce);
    if (!ce.isOverridable()) {
        ce.rejectCertificate();
        return false;
    }
    bool ignore = userAlreadyChoseToIgnoreError(ce);
    if (ignore) {
        ce.acceptCertificate();
    } else {
        ce.defer();
        QPointer<WebEnginePage> ptr(page);
        CertificateErrorData data{ce, ptr};
        if (!displayDialogIfPossible(data)) {
            m_certificates.append(data);
        }
    }
    return true;
}

bool CertificateErrorDialogManager::userAlreadyChoseToIgnoreError(const QWebEngineCertificateError &ce)
{
    int error = static_cast<int>(ce.type());
    QString url = ce.url().url();
    KConfigGroup grp(KSharedConfig::openConfig(), "CertificateExceptions");
    QList<int> exceptionsForUrl = grp.readEntry(url, QList<int>{});
    return (exceptionsForUrl.contains(error));
}

QWidget* CertificateErrorDialogManager::windowForPage(WebEnginePage* page)
{
    if (page) {
        QWidget *view = page->view();
        if (view) {
            return view->window();
        }
    }
    return nullptr;
}

bool CertificateErrorDialogManager::displayDialogIfPossible(const CertificateErrorDialogManager::CertificateErrorData& data)
{
    QWidget *window = windowForPage(data.page);
    if (m_dialogs.contains(window)) {
        return false;
    } else {
        displayDialog(data, window);
        return true;
    }
}

void CertificateErrorDialogManager::displayDialog(const CertificateErrorDialogManager::CertificateErrorData& data, QWidget *window)
{
    if (!window) {
        window = windowForPage(data.page);
    }
    Q_ASSERT(!m_dialogs.contains(window));

    WebEnginePartCertificateErrorDlg *dlg = new WebEnginePartCertificateErrorDlg(data.error, data.page, window);
    connect(dlg, &WebEnginePartCertificateErrorDlg::finished, this, [this, dlg](int){
        applyUserChoice(dlg);});
    connect(dlg, &WebEnginePartCertificateErrorDlg::destroyed, this, &CertificateErrorDialogManager::removeDestroyedDialog);
    connect(window, &QWidget::destroyed, this, &CertificateErrorDialogManager::removeDestroyedWindow);
    m_dialogs.insert(window, dlg);
    dlg->open();
}

void CertificateErrorDialogManager::displayNextDialog(QWidget *window)
{
    if (!window) {
        return;
    }
    auto findNext = [window](const CertificateErrorData &data) {
        return windowForPage(data.page) == window;
    };
    auto it = std::find_if(m_certificates.begin(), m_certificates.end(), findNext);
    if (it == m_certificates.end()) {
        return;
    }
    displayDialog(*it, window);
    m_certificates.erase(it);
}

void CertificateErrorDialogManager::applyUserChoice(WebEnginePartCertificateErrorDlg *dlg)
{
    QWebEngineCertificateError error = dlg->certificateError();
    WebEnginePartCertificateErrorDlg::UserChoice choice = dlg->userChoice();
    if (choice == WebEnginePartCertificateErrorDlg::UserChoice::DontIgnoreError) {
        error.rejectCertificate();
    } else {
        error.acceptCertificate();
        if (choice == WebEnginePartCertificateErrorDlg::UserChoice::IgnoreErrorForever) {
            recordIgnoreForeverChoice(error);
        }
    }
    dlg->deleteLater();
}

void CertificateErrorDialogManager::removeDestroyedDialog(QObject *dlg)
{
    auto findItemForDialog = [dlg](const std::pair<QObject*, QObject*>& pair){return pair.second == dlg;};
    auto it = std::find_if(m_dialogs.constKeyValueBegin(), m_dialogs.constKeyValueEnd(), findItemForDialog);
    if (it == m_dialogs.constKeyValueEnd()) {
        return;
    }
    QWidget *window = qobject_cast<QWidget*>(it->first);
    m_dialogs.remove(it->first);
    if (window) {
        disconnect(window, nullptr, this, nullptr);
        displayNextDialog(window);
    }
}

void CertificateErrorDialogManager::removeDestroyedWindow(QObject *window)
{
    if (!window) {
        return;
    }
    m_dialogs.remove(window);
}

void CertificateErrorDialogManager::recordIgnoreForeverChoice(const QWebEngineCertificateError& ce)
{
    KConfigGroup grp(KSharedConfig::openConfig(), "CertificateExceptions");
    QString url = ce.url().url();
    int error = ce.type();
    QList<int> exceptionsForUrl = grp.readEntry(url, QList<int>{});
    exceptionsForUrl.append(error);
    grp.writeEntry(url, exceptionsForUrl);
    grp.sync();
}



