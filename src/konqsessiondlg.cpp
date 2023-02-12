/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqsessiondlg.h"
#include "konqsettingsxt.h"
#include "konqviewmanager.h"
#include "konqsessionmanager.h"
#include "konqmainwindow.h"
#include "ui_konqsessiondlg_base.h"
#include "ui_konqnewsessiondlg_base.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QIcon>
#include <QPushButton>
#include <QStandardPaths>

#include "konqdebug.h"
#include <kio/copyjob.h>
#include <kio/renamedialog.h>
#include <kfileitemdelegate.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kstandardguiitem.h>
#include <kio/global.h>
#include <KLocalizedString>
#include <kconfig.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <KGuiItem>
#include <QVBoxLayout>

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
    QDialogButtonBox *m_buttonBox;
};

KonqSessionDlg::KonqSessionDlg(KonqViewManager *manager, QWidget *parent)
    : QDialog(parent)
    , d(new KonqSessionDlgPrivate(manager, this))
{
    setObjectName(QStringLiteral("konq_session_dialog"));
    setModal(true);
    setWindowTitle(i18nc("@title:window", "Manage Sessions"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(d);

    d->m_pSaveCurrentButton->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    d->m_pRenameButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    d->m_pDeleteButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    d->m_pNewButton->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/sessions/");
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

    d->m_pOpenTabsInsideCurrentWindow->setChecked(
        KonqSettings::openTabsInsideCurrentWindow());

    connect(d->m_pNewButton, &QPushButton::clicked, this, &KonqSessionDlg::slotNew);
    connect(d->m_pSaveCurrentButton, &QPushButton::clicked, this, &KonqSessionDlg::slotSave);
    connect(d->m_pRenameButton, SIGNAL(clicked()), SLOT(slotRename()));
    connect(d->m_pDeleteButton, &QPushButton::clicked, this, &KonqSessionDlg::slotDelete);

    d->m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Close);
    connect(d->m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(d->m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(d->m_buttonBox);
    d->m_buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
    QPushButton *openButton = d->m_buttonBox->button(QDialogButtonBox::Open);
    connect(openButton, &QPushButton::clicked, this, &KonqSessionDlg::slotOpen);

    slotSelectionChanged();

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
    if (!QDir(dir).removeRecursively()) {
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
                          dirpathTo, KIO::RenameDialog_Options());

    if (dlg.exec() == KIO::Result_Rename) {
        dirpathTo = dlg.newDestUrl();
        QDir dir(dirpathTo.path());
        if (dir.exists()) {
            slotRename(dirpathTo);
        } else {
            QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/sessions/"));
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
    QPushButton *openButton = d->m_buttonBox->button(QDialogButtonBox::Open);
    openButton->setEnabled(enable);
}

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
    QDialogButtonBox *m_buttonBox;
};

KonqNewSessionDlg::KonqNewSessionDlg(QWidget *parent, KonqMainWindow *mainWindow, QString sessionName, Mode mode)
    : QDialog(parent)
    , d(new KonqNewSessionDlgPrivate(this, mainWindow, mode))
{
    setObjectName(QStringLiteral("konq_new_session_dialog"));
    setModal(true);
    setWindowTitle(i18nc("@title:window", "Save Session"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(d);

    d->m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    mainLayout->addWidget(d->m_buttonBox);
    QPushButton *okButton = d->m_buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    okButton->setEnabled(false);

    if (!sessionName.isEmpty()) {
        d->m_pSessionName->setText(sessionName);
        okButton->setEnabled(true);
    }

    d->m_pSessionName->setFocus();

    connect(okButton, &QPushButton::clicked, this, &KonqNewSessionDlg::slotAddSession);
    connect(d->m_pSessionName, SIGNAL(textChanged(QString)), this,
            SLOT(slotTextChanged(QString)));

    resize(sizeHint());
    connect(d->m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(d->m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void KonqNewSessionDlg::slotAddSession()
{
    QString dirpath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/sessions/") + KIO::encodeFileName(d->m_pSessionName->text());

    QDir dir(dirpath);
    if (dir.exists()) {
        if ((d->m_mode == ReplaceFile) ||
            KMessageBox::questionTwoActions(this,
                                       i18n("A session with the name '%1' already exists, do you want to overwrite it?", d->m_pSessionName->text()),
                                       i18nc("@title:window", "Session exists. Overwrite?"),
                                       KStandardGuiItem::overwrite(),
                                       KStandardGuiItem::cancel()) == KMessageBox::PrimaryAction) {
            QDir(dirpath).removeRecursively();
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
    QPushButton *okButton = d->m_buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(!text.isEmpty());
}

KonqNewSessionDlg::~KonqNewSessionDlg()
{
}

