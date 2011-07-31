/* This file is part of the KDE project
 *
 * Copyright (C) 2005 Leo Savernik <l.savernik@aon.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
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

#include "domtreewindow.h"
#include "domtreeview.h"
#include "domtreecommands.h"
#include "plugin_domtreeviewer.h"
#include "ui_messagedialog.h"


#include <kundostack.h>
#include <kconfig.h>
#include <khtml_part.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kstatusbar.h>
#include <kstandarddirs.h>
#include <kstandardguiitem.h>
#include <ktextedit.h>
#include <kurl.h>
#include <kurlrequesterdialog.h>
#include <kxmlguifactory.h>
#include <kparts/partmanager.h>

#include <kedittoolbar.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <kstandardaction.h>

#include <qdatetime.h>
#include <qtimer.h>
#include <QDragEnterEvent>
#include <QMenu>
#include <QDropEvent>


class MessageDialog : public KDialog, public Ui::MessageDialog
{
public:
    MessageDialog(QWidget *parent)
        : KDialog(parent)
    {
        setupUi(mainWidget());

        setWindowTitle(i18nc("@title:window", "Message Log"));
        setButtons(Close | User1);
        setButtonGuiItem(User1, KStandardGuiItem::clear());

        QPalette pal = messagePane->palette();
        pal.setColor(messagePane->backgroundRole(), palette().color(QPalette::Active, QPalette::Base));
        messagePane->setPalette(pal);

        connect(this, SIGNAL(closeClicked()), this, SLOT(close()));
        connect(this, SIGNAL(user1Clicked()), messagePane, SLOT(clear()));
    }

    void addMessage(const QString &msg)
    {
        messagePane->append(msg);
    }
};


using domtreeviewer::ManipulationCommand;

DOMTreeWindow::DOMTreeWindow(PluginDomtreeviewer *plugin)
    : KXmlGuiWindow( 0 ),
      m_plugin(plugin), m_view(new DOMTreeView(this))
{
    setObjectName("DOMTreeWindow" );
    part_manager = 0;

    // set configuration object
    _config = new KConfig("domtreeviewerrc");

    // accept dnd
    setAcceptDrops(true);

    // tell the KXmlGuiWindow that this is indeed the main widget
    setCentralWidget(m_view);

    // message window dialog
    msgdlg = new MessageDialog(0);
// msgdlg->show();

    // then, setup our actions
    setupActions();

    // Add typical actions and save size/toolbars/statusbar
    setupGUI(ToolBar | Keys | StatusBar | Save | Create,
             KStandardDirs::locate( "data", "domtreeviewer/domtreeviewerui.rc", componentData()));

    // allow the view to change the statusbar and caption
#if 0
    connect(m_view, SIGNAL(signalChangeStatusbar(QString)),
            this,   SLOT(changeStatusbar(QString)));
    connect(m_view, SIGNAL(signalChangeCaption(QString)),
            this,   SLOT(changeCaption(QString)));
#endif
    connect(view(), SIGNAL(htmlPartChanged(KHTMLPart*)),
    		SLOT(slotHtmlPartChanged(KHTMLPart*)));

    ManipulationCommand::connect(SIGNAL(error(int,QString)),
    				this, SLOT(addMessage(int,QString)));

    infopanel_ctx = createInfoPanelAttrContextMenu();
    domtree_ctx = createDOMTreeViewContextMenu();

}

DOMTreeWindow::~DOMTreeWindow()
{
    kDebug(90180) << this;
    delete m_commandHistory;
    delete msgdlg;
    delete _config;
}

void DOMTreeWindow::executeAndAddCommand(ManipulationCommand *cmd)
{
    m_commandHistory->push(cmd); // calls cmd->redo()
    if (!cmd->isValid()) {
        cmd->undo();
        // TODO: ideally, remove the command from m_commandHistory, but I don't see how to do that.
    } else {
        view()->hideMessageLine();
    }
}

void DOMTreeWindow::setupActions()
{
    KStandardAction::close(this, SLOT(close()), actionCollection());

    KStandardAction::cut(this, SLOT(slotCut()), actionCollection())->setEnabled(false);
    KStandardAction::copy(this, SLOT(slotCopy()), actionCollection())->setEnabled(false);
    KStandardAction::paste(this, SLOT(slotPaste()), actionCollection())->setEnabled(false);

    m_commandHistory = new KUndoStack;

    QAction* undoAction = m_commandHistory->createUndoAction(actionCollection());
    connect(undoAction, SIGNAL(triggered()), m_commandHistory, SLOT(undo()));
    QAction* redoAction = m_commandHistory->createRedoAction(actionCollection());
    connect(redoAction, SIGNAL(triggered()), m_commandHistory, SLOT(redo()));


    KStandardAction::find(this, SLOT(slotFind()), actionCollection());

    KStandardAction::redisplay(m_view, SLOT(refresh()), actionCollection());

    // Show/hide options
    QAction *pure = actionCollection()->addAction("show_dom_pure");
    pure->setText(i18n("Pure DOM Tree"));
    pure->setCheckable(true);
    pure->setChecked(true);
    connect(pure, SIGNAL(toggled(bool)), m_view, SLOT(slotPureToggled(bool)));

    QAction *attr = actionCollection()->addAction("show_dom_attributes");
    attr->setText(i18n("Show DOM Attributes"));
    attr->setCheckable(true);
    attr->setChecked(true);
    connect(attr, SIGNAL(toggled(bool)),
            m_view, SLOT(slotShowAttributesToggled(bool)));

    QAction *highlight = actionCollection()->addAction("show_highlight_html");
    highlight->setText(i18n("Highlight HTML"));
    highlight->setCheckable(true);
    highlight->setChecked(true);
    connect(highlight, SIGNAL(toggled(bool)),
            m_view, SLOT(slotHighlightHTMLToggled(bool)));

    // toggle manipulation dialog
    QAction *a = actionCollection()->addAction("show_msg_dlg");
    a->setText(i18n("Show Message Log"));
    a->setShortcut(Qt::CTRL+Qt::Key_E);
    connect(a, SIGNAL(triggered()), this, SLOT(showMessageLog()));

//     KAction *custom = new KAction(i18n("Cus&tom Menuitem"), 0,
//                                   this, SLOT(optionsPreferences()),
//                                   actionCollection(), "custom_action");

    // actions for the dom tree list view toolbar
    QAction *tree = KStandardAction::up(view(), SLOT(moveToParent()), actionCollection());
    actionCollection()->addAction("tree_up",tree);
    QAction *tree_inc_level = actionCollection()->addAction("tree_inc_level");
    tree_inc_level->setText(i18n("Expand"));
    tree_inc_level->setIcon(KIcon("arrow-right"));
    tree_inc_level->setShortcut(Qt::CTRL+Qt::Key_Greater);
    tree_inc_level->setToolTip(i18n("Increase expansion level"));
    connect(tree_inc_level, SIGNAL(triggered()), view(), SLOT(increaseExpansionDepth()));
    QAction *tree_dec_level = actionCollection()->addAction( "tree_dec_level");
    tree_dec_level->setText(i18n("Collapse"));
    tree_dec_level->setIcon(KIcon("arrow-left"));
    tree_dec_level->setShortcut(Qt::CTRL+Qt::Key_Less);
    tree_dec_level->setToolTip(i18n("Decrease expansion level"));
    connect(tree_dec_level, SIGNAL(triggered()), view(), SLOT(decreaseExpansionDepth()));

    // actions for the dom tree list view context menu

    del_tree = actionCollection()->addAction( "tree_delete");
    del_tree->setText(i18n("&Delete"));
    del_tree->setIcon(KIcon("edit-delete"));
    del_tree->setShortcut(Qt::Key_Delete);
    del_tree->setToolTip(i18n("Delete nodes"));
    del_tree->setShortcutContext(Qt::WidgetShortcut);
    view()->m_listView->addAction(del_tree);
    connect(del_tree, SIGNAL(triggered()), view(), SLOT(deleteNodes()));
    QAction *new_elem = actionCollection()->addAction( "tree_add_element");
    new_elem->setText(i18n("New &Element..."));
    new_elem->setIcon(KIcon("document-new"));
    connect(new_elem, SIGNAL(triggered()), view(), SLOT(slotAddElementDlg()));
    QAction *new_text = actionCollection()->addAction( "tree_add_text");
    new_text->setText(i18n("New &Text Node..."));
    new_text->setIcon(KIcon("draw-text"));
    connect(new_text, SIGNAL(triggered()), view(), SLOT(slotAddTextDlg()));

    // actions for the info panel attribute list context menu
    del_attr = actionCollection()->addAction( "attr_delete");
    del_attr->setText(i18n("&Delete"));
    del_attr->setIcon(KIcon("edit-delete"));
    del_attr->setShortcut(Qt::Key_Delete);
    del_attr->setToolTip(i18n("Delete attributes"));
    del_attr->setShortcutContext(Qt::WidgetShortcut);
    view()->nodeAttributes->addAction(del_attr);
    connect(del_attr, SIGNAL(triggered()), view(), SLOT(deleteAttributes()));

}

QMenu *DOMTreeWindow::createInfoPanelAttrContextMenu()
{
  QWidget *w = factory()->container("infopanelattr_context", this);
  Q_ASSERT(w);
  return static_cast<QMenu *>(w);
}

QMenu *DOMTreeWindow::createDOMTreeViewContextMenu()
{
  QWidget *w = factory()->container("domtree_context", this);
  Q_ASSERT(w);
  return static_cast<QMenu *>(w);
}

void DOMTreeWindow::saveProperties(KConfigGroup &config)
{
    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored

#if 0
    if (!m_view->currentURL().isNull()) {
        config.writePathEntry("lastURL", m_view->currentURL());
#endif
}

void DOMTreeWindow::readProperties(const KConfigGroup &config)
{
    // the 'config' object points to the session managed
    // config file.  this function is automatically called whenever
    // the app is being restored.  read in here whatever you wrote
    // in 'saveProperties'

#if 0
    QString url = config.readPathEntry("lastURL", QString());

    if (!url.isEmpty())
        m_view->openUrl(KUrl::fromPathOrUrl(url));
#endif
}

void DOMTreeWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // accept uri drops only
    event->setAccepted(KUrl::List::canDecode(event->mimeData()));
}

void DOMTreeWindow::dropEvent(QDropEvent *event)
{
    // this is a very simplistic implementation of a drop event.  we
    // will only accept a dropped URL.  the Qt dnd code can do *much*
    // much more, so please read the docs there

    // see if we can decode a URI.. if not, just ignore it
    KUrl::List urls = KUrl::List::fromMimeData( event->mimeData() );
    if (!urls.isEmpty())
    {
        // okay, we have a URI.. process it
        const KUrl &url = urls.first();
#if 0
        // load in the file
        load(url);
#endif
    }
}

void DOMTreeWindow::addMessage(int msg_id, const QString &msg)
{
  QDateTime t(QDateTime::currentDateTime());
  QString fullmsg = t.toString();
  fullmsg += ':';

  if (msg_id != 0)
    fullmsg += " (" + QString::number(msg_id) + ") ";
  fullmsg += msg;

  if (msgdlg) msgdlg->addMessage(fullmsg);
  view()->setMessage(msg);
  kWarning() << fullmsg ;
}
void DOMTreeWindow::slotCut()
{
  // TODO implement
}

void DOMTreeWindow::slotCopy()
{
  // TODO implement
}

void DOMTreeWindow::slotPaste()
{
  // TODO implement
}

void DOMTreeWindow::slotFind()
{
  view()->slotFindClicked();
}

void DOMTreeWindow::showMessageLog()
{
  msgdlg->show();
  msgdlg->raise();
  msgdlg->activateWindow();
}

void DOMTreeWindow::optionsConfigureToolbars()
{
    // use the standard toolbar editor
    saveMainWindowSettings( config()->group( autoSaveGroup() ) );
    KEditToolBar dlg(actionCollection());
    connect(&dlg, SIGNAL(newToolbarConfig()), this, SLOT(newToolbarConfig()));
    dlg.exec();
}

void DOMTreeWindow::newToolbarConfig()
{
    // this slot is called when user clicks "Ok" or "Apply" in the toolbar editor.
    // recreate our GUI, and re-apply the settings (e.g. "text under icons", etc.)
    createGUI(KStandardDirs::locate( "data", "domtreeviewer/domtreeviewerui.rc", componentData()));
    applyMainWindowSettings( config()->group( autoSaveGroup() ) );
}

void DOMTreeWindow::optionsPreferences()
{
#if 0
    // popup some sort of preference dialog, here
    DOMTreeWindowPreferences dlg;
    if (dlg.exec())
    {
        // redo your settings
    }
#endif
}

void DOMTreeWindow::changeStatusbar(const QString& text)
{
    // display the text on the statusbar
    statusBar()->showMessage(text);
}

void DOMTreeWindow::changeCaption(const QString& text)
{
    // display the text on the caption
    setCaption(text);
}

void DOMTreeWindow::slotHtmlPartChanged(KHTMLPart *p)
{
  kDebug(90180) << p;

  if (p) {
    // set up manager connections
    if ( part_manager )
        disconnect(part_manager);

    part_manager = p->manager();

    connect(part_manager, SIGNAL(activePartChanged(KParts::Part*)),
    	SLOT(slotActivePartChanged(KParts::Part*)));
    connect(part_manager, SIGNAL(partRemoved(KParts::Part*)),
    	SLOT(slotPartRemoved(KParts::Part*)));

    // set up browser extension connections
    connect(p, SIGNAL(docCreated()), SLOT(slotClosePart()));
  }
}

void DOMTreeWindow::slotActivePartChanged(KParts::Part *p)
{
  kDebug(90180) << p;
  if (p == view()->htmlPart())
    return;

  m_commandHistory->clear();
  view()->disconnectFromTornDownPart();
  view()->setHtmlPart(qobject_cast<KHTMLPart *>(p));
}

void DOMTreeWindow::slotPartRemoved(KParts::Part *p)
{
  kDebug(90180) << p;
  if (p != view()->htmlPart()) return;

  m_commandHistory->clear();
  view()->disconnectFromTornDownPart();
  view()->setHtmlPart(0);
}

void DOMTreeWindow::slotClosePart()
{
  kDebug(90180) ;
  view()->disconnectFromTornDownPart();
  view()->connectToPart();
}

#include "domtreewindow.moc"
