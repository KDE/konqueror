/*  This file is part of the KDE project
    Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "konqsessiondlg.h"
#include "konqsettingsxt.h"
#include "konqviewmanager.h"
#include "konqsessionmanager.h"
#include "konqmainwindow.h"
#include "ui_konqsessiondlg_base.h"
#include "ui_konqnewsessiondlg_base.h"

#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QPushButton>

#include "konqdebug.h"
#include <kio/copyjob.h>
#include <ktempdir.h>
#include <kio/renamedialog.h>
#include <kfileitemdelegate.h>
#include <QIcon>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kstandardguiitem.h>
#include <kio/global.h>
#include <kstandarddirs.h>
#include <KLocalizedString>
#include <kconfig.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <klistwidget.h>
#include <QStandardPaths>

class KonqSessionDlg::KonqSessionDlgPrivate : public QWidget,
    public Ui::KonqSessionDlgBase
{
public:
    KonqSessionDlgPrivate(KonqViewManager *manager, QWidget *parent = nullptr)
        : QWidget(parent), m_pViewManager(manager), m_pParent(parent)
    {
        setupUi(this);
    }
    KonqViewManager *const m_pViewManager;
    KDirModel *m_pModel;
    QWidget *m_pParent;
};

#define BTN_OPEN KDialog::User1

KonqSessionDlg::KonqSessionDlg(KonqViewManager *manager, QWidget *parent)
    : KDialog(parent)
    , d(new KonqSessionDlgPrivate(manager, this))
{
    d->layout()->setContentsMargins(0, 0, 0, 0);
    setMainWidget(d);

    setObjectName(QStringLiteral("konq_session_dialog"));
    setModal(true);
    setCaption(i18nc("@title:window", "Manage Sessions"));
    setButtons(BTN_OPEN | Close);
    setDefaultButton(Close);

    setButtonGuiItem(BTN_OPEN, KGuiItem(i18n("&Open"), QStringLiteral("document-open")));
    d->m_pSaveCurrentButton->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    d->m_pRenameButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    d->m_pDeleteButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    d->m_pNewButton->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));

    QString dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + QLatin1String("sessions/");
    QDir().mkpath(dir);

    d->m_pModel = new KDirModel(d->m_pListView);
    d->m_pModel->sort(QDir::Name);
    d->m_pModel->dirLister()->setDirOnlyMode(true);
    d->m_pModel->dirLister()->openUrl(QUrl::fromLocalFile(dir));
    d->m_pListView->setModel(d->m_pModel);

    d->m_pListView->setMinimumSize(d->m_pListView->sizeHint());

    connect(d->m_pListView->selectionModel(), SIGNAL(selectionChanged(
                const QItemSelection &, const QItemSelection &)), this, SLOT(
                slotSelectionChanged()));

    enableButton(BTN_OPEN, d->m_pListView->currentIndex().isValid());
    slotSelectionChanged();

    d->m_pOpenTabsInsideCurrentWindow->setChecked(
        KonqSettings::openTabsInsideCurrentWindow());

    connect(this, &KonqSessionDlg::user1Clicked, this, &KonqSessionDlg::slotOpen);
    connect(d->m_pNewButton, &QPushButton::clicked, this, &KonqSessionDlg::slotNew);
    connect(d->m_pSaveCurrentButton, &QPushButton::clicked, this, &KonqSessionDlg::slotSave);
    connect(d->m_pRenameButton, SIGNAL(clicked()), SLOT(slotRename()));
    connect(d->m_pDeleteButton, &QPushButton::clicked, this, &KonqSessionDlg::slotDelete);

    resize(sizeHint());
}

KonqSessionDlg::~KonqSessionDlg()
{
    KonqSettings::setOpenTabsInsideCurrentWindow(
        d->m_pOpenTabsInsideCurrentWindow->isChecked());
}

void KonqSessionDlg::slotOpen()
{
    if (!d->m_pListView->currentIndex().isValid()) {
        return;
    }

    KonqSessionManager::self()->restoreSessions(d->m_pModel->itemForIndex(
                d->m_pListView->currentIndex()).url().path(),
            d->m_pOpenTabsInsideCurrentWindow->isChecked(),
            d->m_pViewManager->mainWindow());
    close();
}

void KonqSessionDlg::slotSave()
{
    if (!d->m_pListView->currentIndex().isValid()) {
        return;
    }

    QFileInfo fileInfo(
        d->m_pModel->itemForIndex(d->m_pListView->currentIndex()).url().path());

    KonqNewSessionDlg newDialog(this, d->m_pViewManager->mainWindow(),
        KIO::encodeFileName(fileInfo.fileName()), KonqNewSessionDlg::ReplaceFile);

    newDialog.exec();
}

void KonqSessionDlg::slotNew()
{
    KonqNewSessionDlg newDialog(this, d->m_pViewManager->mainWindow());
    newDialog.exec();
}

void KonqSessionDlg::slotDelete()
{
    if (!d->m_pListView->currentIndex().isValid()) {
        return;
    }

    const QString dir = d->m_pModel->itemForIndex(d->m_pListView->currentIndex()).url().toLocalFile();
    if (!KTempDir::removeDir(dir)) {
        // TODO show error msg box
    }
}

void KonqSessionDlg::slotRename(QUrl dirpathTo)
{
    if (!d->m_pListView->currentIndex().isValid()) {
        return;
    }

    QUrl dirpathFrom = d->m_pModel->itemForIndex(
                           d->m_pListView->currentIndex()).url();

    dirpathTo = (dirpathTo == QUrl()) ? dirpathFrom : dirpathTo;

    KIO::RenameDialog dlg(this, i18nc("@title:window", "Rename Session"), dirpathFrom,
                          dirpathTo, KIO::RenameDialog_Options(nullptr));

    if (dlg.exec() == KIO::R_RENAME) {
        dirpathTo = dlg.newDestUrl();
        QDir dir(dirpathTo.path());
        if (dir.exists()) {
            slotRename(dirpathTo);
        } else {
            QDir dir(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "sessions/");
            dir.rename(dirpathFrom.fileName(), dlg.newDestUrl().fileName());
        }
    }
}

void KonqSessionDlg::slotSelectionChanged()
{
    bool enable = !d->m_pListView->selectionModel()->selectedIndexes().isEmpty();
    d->m_pSaveCurrentButton->setEnabled(enable);
    d->m_pRenameButton->setEnabled(enable);
    d->m_pDeleteButton->setEnabled(enable);
    enableButton(BTN_OPEN, enable);
}

#undef BTN_OPEN

class KonqNewSessionDlg::KonqNewSessionDlgPrivate : public QWidget,
    public Ui::KonqNewSessionDlgBase
{
public:
    KonqNewSessionDlgPrivate(QWidget *parent = nullptr, KonqMainWindow *mainWindow = nullptr,
                             KonqNewSessionDlg::Mode m = KonqNewSessionDlg::NewFile)
        : QWidget(parent), m_pParent(parent), m_mainWindow(mainWindow), m_mode(m)
    {
        setupUi(this);
    }
    QWidget *m_pParent;
    KonqMainWindow *m_mainWindow;
    KonqNewSessionDlg::Mode m_mode;
};

KonqNewSessionDlg::KonqNewSessionDlg(QWidget *parent, KonqMainWindow *mainWindow, QString sessionName, Mode mode)
    : KDialog(parent)
    , d(new KonqNewSessionDlgPrivate(this, mainWindow, mode))
{
    d->layout()->setContentsMargins(0, 0, 0, 0);
    setMainWidget(d);

    setObjectName(QStringLiteral("konq_new_session_dialog"));
    setModal(true);
    setCaption(i18nc("@title:window", "Save Session"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    enableButton(Ok, false);

    if (!sessionName.isEmpty()) {
        d->m_pSessionName->setText(sessionName);
        enableButton(Ok, true);
    }

    d->m_pSessionName->setFocus();

    connect(this, &KonqNewSessionDlg::okClicked, this, &KonqNewSessionDlg::slotAddSession);
    connect(d->m_pSessionName, SIGNAL(textChanged(QString)), this,
            SLOT(slotTextChanged(QString)));

    resize(sizeHint());
}

void KonqNewSessionDlg::slotAddSession()
{
    QString dirpath = KStandardDirs::locateLocal("appdata", "sessions/" +
                      KIO::encodeFileName(d->m_pSessionName->text()));

    QDir dir(dirpath);
    if (dir.exists()) {
        if ((d->m_mode == ReplaceFile) ||
            KMessageBox::questionYesNo(this,
                                       i18n("A session with the name '%1' already exists, do you want to overwrite it?", d->m_pSessionName->text()),
                                       i18nc("@title:window", "Session exists. Overwrite?")) == KMessageBox::Yes) {
            KTempDir::removeDir(dirpath);
        } else {
            return;
        }
    }

    if (d->m_pAllWindows->isChecked()) {
        KonqSessionManager::self()->saveCurrentSessions(dirpath);
    } else {
        KonqSessionManager::self()->saveCurrentSessionToFile(dirpath + QLatin1String("/1"), d->m_mainWindow);
    }
}

void KonqNewSessionDlg::slotTextChanged(const QString &text)
{
    enableButton(Ok, !text.isEmpty());
}

KonqNewSessionDlg::~KonqNewSessionDlg()
{
}

