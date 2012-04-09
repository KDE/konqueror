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
#include "ui_konqsessiondlg_base.h"
#include "ui_konqnewsessiondlg_base.h"

#include <QtCore/QDir>
#include <QFileSystemModel>
#include <QListWidgetItem>
#include <QtCore/QFileInfo>
#include <QtCore/QDirIterator>

#include <kdebug.h>
#include <kurl.h>
#include <kio/copyjob.h>
#include <ktempdir.h>
#include <kio/renamedialog.h>
#include <kfileitemdelegate.h>
#include <kicon.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kstandardguiitem.h>
#include <kio/global.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kconfig.h>
#include <kseparator.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <klistwidget.h>
#include <kpushbutton.h>

class KonqSessionDlg::KonqSessionDlgPrivate : public QWidget,
    public Ui::KonqSessionDlgBase
{
public:
    KonqSessionDlgPrivate( KonqViewManager *manager, QWidget *parent = 0 )
        : QWidget( parent ) , m_pViewManager( manager ), m_pParent( parent )
    {
        setupUi( this );
    }
    KonqViewManager * const m_pViewManager;
    KDirModel * m_pModel;
    QWidget *m_pParent;
};

#define BTN_OPEN KDialog::User1

KonqSessionDlg::KonqSessionDlg( KonqViewManager *manager, QWidget *parent )
    : KDialog( parent )
    , d( new KonqSessionDlgPrivate( manager, this ) )
{
    d->layout()->setMargin( 0 );
    setMainWidget( d );
    
    setObjectName( QLatin1String( "konq_session_dialog" ) );
    setModal( true );
    setCaption( i18nc( "@title:window", "Manage Sessions" ) );
    setButtons( BTN_OPEN | Close );
    setDefaultButton( Close );
    
    setButtonGuiItem( BTN_OPEN, KGuiItem( i18n( "&Open" ), "document-open" ) );
    d->m_pSaveCurrentButton->setIcon(KIcon("document-save"));
    d->m_pRenameButton->setIcon(KIcon("edit-rename"));
    d->m_pDeleteButton->setIcon(KIcon("edit-delete"));
    d->m_pNewButton->setIcon(KIcon("document-new"));
    
    QString dir = KStandardDirs::locateLocal("appdata", "sessions/");
    
    d->m_pModel = new KDirModel(d->m_pListView);
    d->m_pModel->sort(QDir::Name);
    d->m_pModel->dirLister()->setDirOnlyMode(true);
    d->m_pModel->dirLister()->setAutoUpdate(true);
    d->m_pModel->dirLister()->openUrl(dir);
    d->m_pListView->setModel(d->m_pModel);
    
    d->m_pListView->setMinimumSize( d->m_pListView->sizeHint() );
    
    connect( d->m_pListView->selectionModel(), SIGNAL( selectionChanged(
        const QItemSelection  &, const QItemSelection &) ), this, SLOT(
        slotSelectionChanged() ) );
    
    enableButton( BTN_OPEN, d->m_pListView->currentIndex().isValid() );
    slotSelectionChanged();

    d->m_pOpenTabsInsideCurrentWindow->setChecked(
	KonqSettings::openTabsInsideCurrentWindow());

    connect( this,SIGNAL(user1Clicked()),SLOT(slotOpen()));
    connect( d->m_pNewButton, SIGNAL(clicked()),SLOT(slotNew()));
    connect( d->m_pSaveCurrentButton, SIGNAL(clicked()),SLOT(slotSave()));
    connect( d->m_pRenameButton, SIGNAL(clicked()),SLOT(slotRename()));
    connect( d->m_pDeleteButton, SIGNAL(clicked()),SLOT(slotDelete()));
    
    resize( sizeHint() );
}

KonqSessionDlg::~KonqSessionDlg()
{
    KonqSettings::setOpenTabsInsideCurrentWindow(
	d->m_pOpenTabsInsideCurrentWindow->isChecked());
}

void KonqSessionDlg::slotOpen()
{
    if(!d->m_pListView->currentIndex().isValid())
        return;
    
    KonqSessionManager::self()->restoreSessions(d->m_pModel->itemForIndex(
        d->m_pListView->currentIndex()).url().path(),
	d->m_pOpenTabsInsideCurrentWindow->isChecked(), 
	reinterpret_cast<KonqMainWindow*>(parent()));
    close();
}

void KonqSessionDlg::slotSave()
{
    if(!d->m_pListView->currentIndex().isValid())
        return;
    
    QFileInfo fileInfo(
        d->m_pModel->itemForIndex(d->m_pListView->currentIndex()).url().path());
    QString dirpath = "sessions/" + KIO::encodeFileName(fileInfo.fileName());
    
    slotDelete();
    KonqSessionManager::self()->saveCurrentSessions(dirpath);
}

void KonqSessionDlg::slotNew()
{
    KonqNewSessionDlg newDialog(this);
    newDialog.exec();
}

void KonqSessionDlg::slotDelete()
{
    if(!d->m_pListView->currentIndex().isValid())
        return;

    const QString dir = d->m_pModel->itemForIndex(d->m_pListView->currentIndex()).url().toLocalFile();
    if (!KTempDir::removeDir(dir)) {
        // TODO show error msg box
    }
}

void KonqSessionDlg::slotRename(KUrl dirpathTo)
{
    if ( !d->m_pListView->currentIndex().isValid() )
        return;
    
    KUrl dirpathFrom = d->m_pModel->itemForIndex(
        d->m_pListView->currentIndex()).url();
    
    dirpathTo = (dirpathTo == KUrl()) ? dirpathFrom : dirpathTo;
    
    KIO::RenameDialog dlg(this, i18nc("@title:window", "Rename Session"), dirpathFrom,
        dirpathTo, KIO::RenameDialog_Mode(0));
        
    if(dlg.exec() == KIO::R_RENAME)
    {
        dirpathTo = dlg.newDestUrl();
        QDir dir(dirpathTo.path());
        if(dir.exists())
            slotRename(dirpathTo);
        else {
            QDir dir(KStandardDirs::locateLocal("appdata", "sessions/"));
            dir.rename(dirpathFrom.fileName(), dlg.newDestUrl().fileName());
        }
    } 
}

void KonqSessionDlg::slotSelectionChanged()
{
    bool enable = !d->m_pListView->selectionModel()->selectedIndexes().isEmpty();
    d->m_pSaveCurrentButton->setEnabled( enable );
    d->m_pRenameButton->setEnabled( enable );
    d->m_pDeleteButton->setEnabled( enable );
    enableButton( BTN_OPEN, enable );
}

#undef BTN_OPEN

class KonqNewSessionDlg::KonqNewSessionDlgPrivate : public QWidget,
    public Ui::KonqNewSessionDlgBase
{
public:
    KonqNewSessionDlgPrivate( QWidget *parent = 0 )
        : QWidget( parent ), m_pParent( parent )
    {
        setupUi( this );
    }
    QWidget *m_pParent;
};

KonqNewSessionDlg::KonqNewSessionDlg( QWidget *parent, QString sessionName )
    : KDialog( parent )
    , d( new KonqNewSessionDlgPrivate( this ) )
{
    d->layout()->setMargin( 0 );
    setMainWidget( d );
    
    setObjectName( QLatin1String( "konq_new_session_dialog" ) );
    setModal( true );
    setCaption( i18nc( "@title:window", "Save Session" ) );
    setButtons( Ok | Cancel );
    setDefaultButton( Ok );
    enableButton( Ok, false );
    
    if(!sessionName.isEmpty())
        d->m_pSessionName->setText(sessionName);
    
    connect(this, SIGNAL(okClicked()), SLOT(slotAddSession()));
    connect(d->m_pSessionName, SIGNAL(textChanged(QString)), this,
        SLOT(slotTextChanged(QString)));
    
    resize( sizeHint() );
}

void KonqNewSessionDlg::slotAddSession()
{
    QString dirpath = KStandardDirs::locateLocal("appdata", "sessions/" + 
        KIO::encodeFileName(d->m_pSessionName->text()));
    
    QDir dir(dirpath);
    if(dir.exists())
    {
        if(KMessageBox::questionYesNo(this,
            i18n("A session with the name '%1' already exists, do you want to overwrite it?", d->m_pSessionName->text()),
            i18nc("@title:window", "Session exists. Overwrite?")) == KMessageBox::Yes)
        {
            KTempDir::removeDir(dirpath);
        } else {
            KonqNewSessionDlg newDialog(d->m_pParent,
                d->m_pSessionName->text());
            newDialog.exec();
        }
    }
    KonqSessionManager::self()->saveCurrentSessions(dirpath);
}

void KonqNewSessionDlg::slotTextChanged(const QString& text)
{
    enableButton( Ok, !text.isEmpty() );
}

KonqNewSessionDlg::~KonqNewSessionDlg()
{
}

#include "konqsessiondlg.moc"
