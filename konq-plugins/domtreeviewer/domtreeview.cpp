/***************************************************************************
    copyright            : (C) 2001 - The Kafka Team/Andreas Schlapbach
                           (C) 2005 - Leo Savernik
                           (C) 2008 - Harri Porten
    email                : kde-kafka@master.kde.org
                           schlpbch@iam.unibe.ch
                           porten@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "domtreeview.h"
#include "domlistviewitem.h"
#include "domtreewindow.h"
#include "domtreecommands.h"

#include "signalreceiver.h"
#include "ui_attributeeditwidget.h"
#include "ui_elementeditwidget.h"
#include "ui_texteditwidget.h"

#include <assert.h>

#include <qapplication.h>
#include <qcheckbox.h>
#include <qevent.h>
#include <qfont.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmenu.h>
#include <qscrollbar.h>
#include <qtextstream.h>
#include <qtimer.h>

#include <dom/dom_core.h>
#include <dom/html_base.h>
#include <dom/dom2_views.h>

#include <kdebug.h>
#include <kcombobox.h>
#include <kdialog.h>
#include <kfinddialog.h>
#include <kglobalsettings.h>
#include <khtml_part.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kstandardguiitem.h>
#include <ktextedit.h>

using namespace domtreeviewer;

class ElementEditDialog : public KDialog, public Ui::ElementEditWidget
{
public:
    ElementEditDialog(QWidget *parent)
        : KDialog(parent)
    {
        setupUi(mainWidget());

        setWindowTitle(i18nc("@title:window", "Edit Element"));
        setButtons(User1 | User2 | Cancel);
        setButtonText(User1, i18n("&Append as Child"));
        setButtonText(User2, i18n("Insert &Before Current"));

        connect(this, SIGNAL(cancelClicked()), this, SLOT(reject()));
        connect(this, SIGNAL(user1Clicked()), this, SLOT(accept()));
        connect(this, SIGNAL(user2Clicked()), this, SLOT(accept()));
    }
};


class TextEditDialog : public KDialog, public Ui::TextEditWidget
{
public:
    TextEditDialog(QWidget *parent)
        : KDialog(parent)
    {
        setupUi(mainWidget());

        setWindowTitle(i18nc("@title:window", "Edit Text"));
        setButtons(User1 | User2 | Cancel);
        setButtonText(User1, i18n("&Append as Child"));
        setButtonText(User2, i18n("Insert &Before Current"));

        connect(this, SIGNAL(cancelClicked()), this, SLOT(reject()));
        connect(this, SIGNAL(user1Clicked()), this, SLOT(accept()));
        connect(this, SIGNAL(user2Clicked()), this, SLOT(accept()));
    }
};


class AttributeEditDialog : public KDialog, public Ui::AttributeEditWidget
{
public:
    AttributeEditDialog(QWidget *parent)
        : KDialog(parent)
    {
        setupUi(mainWidget());

        setWindowTitle(i18nc("@title:window", "Edit Attribute"));
        setButtons(Ok | Cancel);

        connect(this, SIGNAL(okClicked()), this, SLOT(accept()));
        connect(this, SIGNAL(cancelClicked()), this, SLOT(reject()));
        connect(attrName, SIGNAL(returnPressed()), this, SLOT(accept()));
    }
};


DOMTreeView::DOMTreeView(QWidget *parent, bool /*allowSaving*/)
  : QWidget(parent), m_expansionDepth(5), m_maxDepth(0),
    m_bPure(true), m_bShowAttributes(true), m_bHighlightHTML(true),
    m_findDialog(0), focused_child(0)
{
  setupUi(this);

  part = 0;

  const QFont font(KGlobalSettings::generalFont());
  m_listView->setFont( font );

  connect(m_listView, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
	  SLOT(slotItemClicked(QTreeWidgetItem *)));
  m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_listView, SIGNAL(customContextMenuRequested(const QPoint &)),
  	SLOT(showDOMTreeContextMenu(const QPoint &)));
#if 0 // ### port to Qt 4
  connect(m_listView, SIGNAL(moved(Q3PtrList<QTreeWidgetItem> &, Q3PtrList<QTreeWidgetItem> &, Q3PtrList<QTreeWidgetItem> &)),
          SLOT(slotMovedItems(Q3PtrList<QTreeWidgetItem> &, Q3PtrList<QTreeWidgetItem> &, Q3PtrList<QTreeWidgetItem> &)));
#endif

  // set up message line
  messageLinePane->hide();
  connect(messageHideBtn, SIGNAL(clicked()), SLOT(hideMessageLine()));
  connect(messageListBtn, SIGNAL(clicked()), mainWindow(), SLOT(showMessageLog()));

  installEventFilter(m_listView);

  ManipulationCommand::connect(SIGNAL(nodeChanged(const DOM::Node &)), this, SLOT(slotRefreshNode(const DOM::Node &)));
  ManipulationCommand::connect(SIGNAL(structureChanged()), this, SLOT(refresh()));

  initDOMNodeInfo();

  installEventFilter(this);
}

DOMTreeView::~DOMTreeView()
{
  delete m_findDialog;
  disconnectFromActivePart();
}

void DOMTreeView::setHtmlPart(KHTMLPart *_part)
{
  KHTMLPart *oldPart = part;
  part = _part;

  if (oldPart) {
    // nothing here yet
  }

  parentWidget()->setWindowTitle( part ? i18nc( "@title:window", "DOM Tree for %1" , part->url().prettyUrl()) : i18nc("@title:window", "DOM Tree") );

  QTimer::singleShot(0, this, SLOT(slotSetHtmlPartDelayed()));
}

DOMTreeWindow *DOMTreeView::mainWindow() const
{
  return static_cast<DOMTreeWindow *>(parentWidget());
}

bool DOMTreeView::eventFilter(QObject *o, QEvent *e)
{
  if (e->type() == QEvent::FocusIn) {

    kDebug(90180) << " focusin o " << o->objectName();
    if (o != this) {
      focused_child = o;
    }

  } else if (e->type() == QEvent::FocusOut) {

    kDebug(90180) << " focusout o " << o->objectName();
    if (o != this) {
      focused_child = 0;
    }

  }

  return false;
}

void DOMTreeView::activateNode(const DOM::Node &node)
{
  slotShowNode(node);
  initializeOptionsFromNode(node);
}

void DOMTreeView::slotShowNode(const DOM::Node &pNode)
{

  if (QTreeWidgetItem *item = m_itemdict.value(pNode.handle(), 0)) {
    m_listView->setCurrentItem(item);
    m_listView->scrollToItem(item);
  }
}

void DOMTreeView::slotShowTree(const DOM::Node &pNode)
{
  DOM::Node child;

  m_listView->clear();
  m_itemdict.clear();

  try
  {
    child = pNode.firstChild();
  }
  catch (DOM::DOMException &)
  {
    return;
  }

  while(!child.isNull()) {
    showRecursive(0, child, 0);
    child = child.nextSibling();
  }

  m_maxDepth--;
  //kDebug(90180) << " Max Depth: " << m_maxDepth;
}

void DOMTreeView::showRecursive(const DOM::Node &pNode, const DOM::Node &node, uint depth)
{
  DOMListViewItem *cur_item;
  DOMListViewItem *parent_item = m_itemdict.value(pNode.handle(), 0);

  if (depth > m_maxDepth) {
    m_maxDepth = depth;
  }

  if (depth == 0) {
    cur_item = new DOMListViewItem(node, m_listView);
    m_document = pNode.ownerDocument();
  } else {
    cur_item = new DOMListViewItem(node, parent_item);
  }

  //kDebug(90180) << node.nodeName().string() << " [" << depth << "]";
  cur_item = addElement (node, cur_item, false);
  m_listView->setItemExpanded(cur_item, depth < m_expansionDepth);

  if(node.handle()) {
    m_itemdict.insert(node.handle(), cur_item);
  }

  DOM::Node child = node.firstChild();
  if (child.isNull()) {
    DOM::HTMLFrameElement frame = node;
    if (!frame.isNull()) {
      child = frame.contentDocument().documentElement();
    } else {
      DOM::HTMLIFrameElement iframe = node;
      if (!iframe.isNull())
        child = iframe.contentDocument().documentElement();
    }
  }
  while(!child.isNull()) {
    showRecursive(node, child, depth + 1);
    child = child.nextSibling();
  }

  const DOM::Element element = node;
  if (!m_bPure) {
    if (!element.isNull() && !element.firstChild().isNull()) {
      if(depth == 0) {
	cur_item = new DOMListViewItem(node, m_listView, cur_item);
	m_document = pNode.ownerDocument();
      } else {
	cur_item = new DOMListViewItem(node, parent_item, cur_item);
      }
      //kDebug(90180) << "</" << node.nodeName().string() << ">";
      cur_item = addElement(element, cur_item, true);
//       m_listView->setItemExpanded(cur_item, depth < m_expansionDepth);
    }
  }
}

DOMListViewItem* DOMTreeView::addElement( const DOM::Node &node,  DOMListViewItem *cur_item, bool isLast)
{
  assert(cur_item);
  cur_item->setClosing(isLast);

  const QString nodeName(node.nodeName().string());
  QString text;
  const DOM::Element element = node;
  if (!element.isNull()) {
    if (!m_bPure) {
      if (isLast) {
	text ="</";
      } else {
	text = "<";
      }
      text += nodeName;
    } else {
      text = nodeName;
    }

    if (m_bShowAttributes && !isLast) {
      QString attributes;
      DOM::Attr attr;
      DOM::NamedNodeMap attrs = element.attributes();
      unsigned long lmap = attrs.length();
      for( unsigned int j=0; j<lmap; j++ ) {
	attr = static_cast<DOM::Attr>(attrs.item(j));
	attributes += " " + attr.name().string() + "=\"" + attr.value().string() + "\"";
      }
      if (!(attributes.isEmpty())) {
	text += ' ';
      }
      text += attributes.simplified();
    }

    if (!m_bPure) {
      if(element.firstChild().isNull()) {
	text += "/>";
      } else {
	text += '>';
      }
    }
    cur_item->setText(0, text);
  } else {
    text = node.nodeValue().string();

    // Hacks to deal with PRE
    QTextStream ts( &text, QIODevice::ReadOnly );
    while (!ts.atEnd()) {
      const QString txt(ts.readLine());
      const QFont font(KGlobalSettings::fixedFont());
      cur_item->setFont( font );
      cur_item->setText(0, '`' + txt  + '\'');

      if(node.handle()) {
	m_itemdict.insert(node.handle(), cur_item);
      }

      DOMListViewItem *parent;
      if (cur_item->parent()) {
	parent = static_cast<DOMListViewItem *>(cur_item->parent());
      } else {
	parent = cur_item;
      }
      cur_item = new DOMListViewItem(node, parent, cur_item);
    }
    // This is one is too much
    DOMListViewItem *notLastItem = static_cast<DOMListViewItem *>(m_listView->itemAbove(cur_item));
    delete cur_item;
    cur_item = notLastItem;
  }

  if (cur_item && m_bHighlightHTML && node.ownerDocument().isHTMLDocument()) {
    highlightHTML(cur_item, nodeName);
  }
  return cur_item;
}

void DOMTreeView::highlightHTML(DOMListViewItem *cur_item, const QString &nodeName)
{
  assert(cur_item);
  /* This is slow. I could make it O(1) be using the tokenizer of khtml but I don't
   * think it's worth it.
   */

  QColor namedColor(palette().color(QPalette::Active, QPalette::Text));
  QString tagName = nodeName.toUpper();
  if ( tagName == "HTML" ) {
    namedColor = "#0000ff";
    cur_item->setBold(true);
  } else if ( tagName == "HEAD" ) {
    namedColor = "#0022ff";
    cur_item->setBold(true);

  } else if ( tagName == "TITLE" ) {
    namedColor = "#2200ff";
  } else if ( tagName == "SCRIPT" ) {
    namedColor = "#4400ff";
  } else if ( tagName == "NOSCRIPT" ) {
    namedColor = "#0044ff";
  } else if ( tagName == "STYLE" ) {
    namedColor = "#0066ff";
  } else if ( tagName == "LINK" ) {
    namedColor = "#6600ff";
  } else if ( tagName == "META" ) {
    namedColor = "#0088ff";

  } else if ( tagName == "BODY" ) {
    namedColor = "#ff0000";
    cur_item->setBold(true);
  } else if ( tagName == "A") {
    namedColor = "blue";
    cur_item->setUnderline(true);
  } else if ( tagName == "IMG") {
    namedColor = "magenta";
    cur_item->setUnderline(true);

  } else if ( tagName == "DIV" ) {
    namedColor = "#ff0044";
  } else if ( tagName == "SPAN" ) {
    namedColor = "#ff4400";
  } else if ( tagName == "P" ) {
    namedColor = "#ff0066";

  } else if ( tagName == "DL" || tagName == "OL"|| tagName == "UL" || tagName == "TABLE" ) {
    namedColor = "#880088";
  } else if ( tagName == "LI" ) {
    namedColor = "#884488";
  } else if ( tagName == "TBODY" ){
    namedColor = "#888888";
  } else if ( tagName == "TR" ) {
    namedColor = "#882288";
  } else if ( tagName == "TD" ) {
    namedColor = "#886688";

  } else if ((tagName == "H1")||(tagName == "H2")||(tagName == "H3") ||
	     (tagName == "H4")||(tagName == "H5")||(tagName == "H6")) {
    namedColor = "#008800";
  } else if (tagName == "HR" ) {
    namedColor = "#228822";

  } else if ( tagName == "FRAME" || tagName == "IFRAME" ) {
    namedColor = "#ff22ff";
  } else if ( tagName == "FRAMESET" ) {
    namedColor = "#dd22dd";

  } else if ( tagName == "APPLET" || tagName == "OBJECT" ) {
    namedColor = "#bb22bb";

  } else if ( tagName == "BASEFONT" || tagName == "FONT" ) {
    namedColor = "#097200";

  } else if ( tagName == "B" || tagName == "STRONG" ) {
    cur_item->setBold(true);
  } else if ( tagName == "I" || tagName == "EM" ) {
    cur_item->setItalic(true);
  } else if ( tagName == "U") {
    cur_item->setUnderline(true);
  }

  cur_item->setColor(namedColor);
}

void DOMTreeView::slotItemClicked(QTreeWidgetItem *cur_item)
{
  DOMListViewItem *cur = static_cast<DOMListViewItem *>(cur_item);
  if (!cur) return;

  DOM::Node handle = cur->node();
  kDebug()<<" handle :";
  if (!handle.isNull()) {
    part->setActiveNode(handle);
  }
}

void DOMTreeView::slotFindClicked()
{
  if (m_findDialog == 0) {
    m_findDialog = new KFindDialog(this);
    m_findDialog->setButtons( KDialog::User1|KDialog::Close );
    m_findDialog->setButtonGuiItem( KDialog::User1, KStandardGuiItem::find() );
    m_findDialog->setDefaultButton(KDialog::User1);

    m_findDialog->setSupportsWholeWordsFind(false);
    m_findDialog->setHasCursor(false);
    m_findDialog->setHasSelection(false);
    m_findDialog->setSupportsRegularExpressionFind(false);

    connect(m_findDialog, SIGNAL(user1Clicked()), this, SLOT(slotSearch()));
  }
  m_findDialog->show();
}

void DOMTreeView::slotRefreshNode(const DOM::Node &pNode)
{
  DOMListViewItem *cur = static_cast<DOMListViewItem *>(m_itemdict.value(pNode.handle(), 0));
  if (!cur) return;

  addElement(pNode, cur, false);
}

// ### unused
void DOMTreeView::slotPrepareMove()
{
  DOMListViewItem *item = static_cast<DOMListViewItem *>(m_listView->currentItem());

  if (!item)
    current_node = DOM::Node();
  else
    current_node = item->node();
}

#if 0 // ### port to Qt 4
void DOMTreeView::slotMovedItems(Q3PtrList<QTreeWidgetItem> &items, Q3PtrList<QTreeWidgetItem> &/*afterFirst*/, Q3PtrList<QTreeWidgetItem> &afterNow)
{
  MultiCommand *cmd = new MultiCommand(i18n("Move Nodes"));
  _refreshed = false;

  Q3PtrList<QTreeWidgetItem>::Iterator it = items.begin();
  Q3PtrList<QTreeWidgetItem>::Iterator anit = afterNow.begin();
  for (; it != items.end(); ++it, ++anit) {
    DOMListViewItem *item = static_cast<DOMListViewItem *>(*it);
    DOMListViewItem *anitem = static_cast<DOMListViewItem *>(*anit);
    DOM::Node parent = static_cast<DOMListViewItem *>(item->parent())->node();
    Q_ASSERT(!parent.isNull());

// kDebug(90180) << " afternow " << anitem << " node " << (anitem ? anitem->node().nodeName().string() : QString()) << "=" << (anitem ? anitem->node().nodeValue().string() : QString());

    cmd->addCommand(new MoveNodeCommand(item->node(), parent,
      anitem ? anitem->node().nextSibling() : parent.firstChild())
    );
  }

  mainWindow()->executeAndAddCommand(cmd);

  // refresh *anyways*, otherwise consistency is disturbed
  if (!_refreshed) refresh();

  slotShowNode(current_node);
}
#endif

void DOMTreeView::slotSearch()
{
  assert(m_findDialog);
  const QString searchText = m_findDialog->pattern();
  Qt::CaseSensitivity caseSensitivity = (m_findDialog->options() & KFind::CaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive;

  for (int i = 0; i < m_listView->topLevelItemCount(); ++i) {
    QTreeWidgetItem *topLevelItem = m_listView->topLevelItem(i);
    searchRecursive(static_cast<DOMListViewItem*>(topLevelItem),
                    searchText, caseSensitivity);
  }

  m_findDialog->hide();
}

void DOMTreeView::searchRecursive(DOMListViewItem* cur_item, const QString& searchText,
				  Qt::CaseSensitivity caseSensitivity)
{
  assert(cur_item);
  const QString text(cur_item->text(0));
  if (text.contains(searchText, caseSensitivity)) {
    cur_item->setUnderline(true);
    cur_item->setItalic(true);
    m_listView->setCurrentItem(cur_item);
    m_listView->scrollToItem(cur_item);
  } else {
    m_listView->setItemExpanded(cur_item, false);
  }

  for (int cp = 0; cp < cur_item->childCount(); ++cp)
    searchRecursive(static_cast<DOMListViewItem*>(cur_item->child(cp)), searchText, caseSensitivity);
}

#if 0
void DOMTreeView::slotSaveClicked()
{
  //kDebug(90180) << "void KfingerCSSWidget::slotSaveAs()";
  KUrl url = KFileDialog::getSaveFileName( part->url().url(), "*.html",
					   this, i18n("Save DOM Tree as HTML") );
  if (!(url.isEmpty()) && url.isValid()) {
    QFile file(url.path());

    if (file.exists()) {
      const QString title = i18nc( "@title:window", "File Exists" );
      const QString text = i18n( "Do you really want to overwrite: \n%1?" , url.url());
      if (KMessageBox::Continue != KMessageBox::warningContinueCancel(this, text, title, i18n("Overwrite") ) ) {
	return;
      }
    }

    if (file.open(QIODevice::WriteOnly) ) {
      kDebug(90180) << "Opened File: " << url.url();
      m_textStream = new QTextStream(&file); //(stdOut)
      saveTreeAsHTML(part->document());
      file.close();
      kDebug(90180) << "File closed ";
      delete m_textStream;
    } else {
      const QString title = i18nc( "@title:window", "Unable to Open File" );
      const QString text = i18n( "Unable to open \n %1 \n for writing" , url.path());
      KMessageBox::sorry( this, text, title );
    }
  } else {
    const QString title = i18nc( "@title:window", "Invalid URL" );
    const QString text = i18n( "This URL \n %1 \n is not valid." , url.url());
    KMessageBox::sorry( this, text, title );
  }
}

void DOMTreeView::saveTreeAsHTML(const DOM::Node &pNode)
{
  assert(m_textStream);

  // Add a doctype

  (*m_textStream) <<"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << endl;
  if(pNode.ownerDocument().isNull()) {
    saveRecursive(pNode, 0);
  } else {
    saveRecursive(pNode.ownerDocument(), 0);
  }
}

void DOMTreeView::saveRecursive(const DOM::Node &pNode, int indent)
{
  const QString nodeName(pNode.nodeName().string());
  QString text;
  QString strIndent;
  strIndent.fill(' ', indent);
  const DOM::Element element = static_cast<const DOM::Element>(pNode);

  text = strIndent;

  if ( !element.isNull() ) {
    if (nodeName.at(0)=='-') {
      /* Don't save khtml internal tags '-konq..'
       * Approximating it with <DIV>
       */
      text += "<DIV> <!-- -KONG_BLOCK -->";
    } else {
      text += '<' + nodeName;

      QString attributes;
      DOM::Attr attr;
      const DOM::NamedNodeMap attrs = element.attributes();
      unsigned long lmap = attrs.length();
      for( uint j=0; j<lmap; j++ ) {
	attr = static_cast<DOM::Attr>(attrs.item(j));
	attributes += " " + attr.name().string() + "=\"" + attr.value().string() + "\"";
      }
      if (!(attributes.isEmpty())){
	text += ' ';
      }

      text += attributes.simplified();

      if(element.firstChild().isNull()) {
	text += "/>";
      } else {
	text += '>';
      }
    }
  } else {
    text = strIndent + pNode.nodeValue().string();
  }

  kDebug(90180) << text;
  if (!(text.isEmpty())) {
    (*m_textStream) << text << endl;
  }

  DOM::Node child = pNode.firstChild();
  while(!child.isNull()) {
    saveRecursive(child, indent+2);
    child = child.nextSibling();
  }

  if (!(element.isNull()) && (!(element.firstChild().isNull()))) {
    if (nodeName.at(0)=='-') {
      text = strIndent + "</DIV> <!-- -KONG_BLOCK -->";
    } else {
      text = strIndent + "</" + pNode.nodeName().string() + '>';
    }
    kDebug(90180) << text;
    (*m_textStream) << text << endl;
  }
}
#endif

void DOMTreeView::updateIncrDecreaseButton()
{
#if 0
    m_decreaseButton->setEnabled((m_expansionDepth > 0));
    m_increaseButton->setEnabled((m_expansionDepth < m_maxDepth));
#endif
}

void DOMTreeView::refresh()
{
  if (!part) return;
  scroll_ofs_x = m_listView->horizontalScrollBar()->value();
  scroll_ofs_y = m_listView->verticalScrollBar()->value();

  m_listView->setUpdatesEnabled(false);
  slotShowTree(part->document());

  QTimer::singleShot(0, this, SLOT(slotRestoreScrollOffset()));
  _refreshed = true;
}

void DOMTreeView::increaseExpansionDepth()
{
  if (!part) return;
  if (m_expansionDepth < m_maxDepth) {
    ++m_expansionDepth;
    adjustDepth();
    updateIncrDecreaseButton();
  } else {
    QApplication::beep();
  }
}

void DOMTreeView::decreaseExpansionDepth()
{
  if (!part) return;
  if (m_expansionDepth > 0) {
    --m_expansionDepth;
    adjustDepth();
    updateIncrDecreaseButton();
  } else {
    QApplication::beep();
  }
}

void DOMTreeView::adjustDepth()
{
  // get current item in a hypersmart way
  DOMListViewItem *cur_node_item = m_itemdict.value(infoNode.handle(), 0);
  if (!cur_node_item)
    cur_node_item = static_cast<DOMListViewItem *>(m_listView->currentItem());

  for (int i = 0; i < m_listView->topLevelItemCount(); ++i) {
    QTreeWidgetItem *topLevelItem = m_listView->topLevelItem(i);
    adjustDepthRecursively(topLevelItem, 0);
  }

  // make current item visible again if possible
  if (cur_node_item)
    m_listView->scrollToItem(cur_node_item);

}

void DOMTreeView::adjustDepthRecursively(QTreeWidgetItem *curItem,  uint currDepth)
{
  if (!curItem)
	return;

  m_listView->setItemExpanded(curItem, m_expansionDepth > currDepth);

  for (int cp = 0; cp < curItem->childCount(); ++cp)
    adjustDepthRecursively(curItem->child(cp), currDepth + 1);
}

void DOMTreeView::setMessage(const QString &msg)
{
  messageLine->setText(msg);
  messageLinePane->show();
}

void DOMTreeView::hideMessageLine()
{
  messageLinePane->hide();
}

void DOMTreeView::moveToParent()
{
  // This is a hypersmart algorithm.
  // If infoNode is defined, go to the parent of infoNode, otherwise, go
  // to the parent of the tree view's current item.
  // Hope this isn't too smart.

  DOM::Node cur = infoNode;
  if (cur.isNull() && m_listView->currentItem()) {
    cur = static_cast<DOMListViewItem *>(m_listView->currentItem())->node();
  }

  if (cur.isNull()) return;

  cur = cur.parentNode();
  activateNode(cur);
}

void DOMTreeView::showDOMTreeContextMenu(const QPoint &pos)
{
  QMenu *ctx = mainWindow()->domTreeViewContextMenu();
  Q_ASSERT(ctx);
  ctx->popup(m_listView->mapToGlobal(pos));
}

void DOMTreeView::slotPureToggled(bool b)
{
  m_bPure = b;
  refresh();
}

void DOMTreeView::slotShowAttributesToggled(bool b)
{
  m_bShowAttributes = b;
  refresh();
}

void DOMTreeView::slotHighlightHTMLToggled(bool b)
{
  m_bHighlightHTML = b;
  refresh();
}

void DOMTreeView::deleteNodes()
{
// kDebug(90180) ;

  DOM::Node last;
  MultiCommand *cmd = new MultiCommand(i18n("Delete Nodes"));
  QTreeWidgetItemIterator it(m_listView, QTreeWidgetItemIterator::Selected);
  for (; *it; ++it) {
    DOMListViewItem *item = static_cast<DOMListViewItem *>(*it);
//     kDebug(90180) << " item->node " << item->node().nodeName().string() << " clos " << item->isClosing();
    if (item->isClosing()) continue;

    // don't regard node more than once
    if (item->node() == last) continue;

    // check for selected parent
    bool has_selected_parent = false;
    for (QTreeWidgetItem *p = item->parent(); p; p = p->parent()) {
      if (p->isSelected()) { has_selected_parent = true; break; }
    }
    if (has_selected_parent) continue;

//     kDebug(90180) << " item->node " << item->node().nodeName().string() << ": schedule for removal";
    // remove this node if it isn't already recursively removed by its parent
    cmd->addCommand(new RemoveNodeCommand(item->node(), item->node().parentNode(), item->node().nextSibling()));
    last = item->node();
  }
  mainWindow()->executeAndAddCommand(cmd);
}

void DOMTreeView::disconnectFromTornDownPart()
{
  if (!part) return;

  m_listView->clear();
  initializeOptionsFromNode(DOM::Node());

  // remove all references to nodes
  infoNode = DOM::Node();	// ### have this handled by dedicated info node panel method
  current_node = DOM::Node();
  active_node_rule = DOM::CSSRule();
  stylesheet = DOM::CSSStyleSheet();
}

void DOMTreeView::connectToPart()
{
  if (part) {
    connect(part, SIGNAL(nodeActivated(const DOM::Node &)), this,
	  SLOT(activateNode(const DOM::Node &)));
    connect(part, SIGNAL(completed()), this, SLOT(refresh()));

    if (!part->document().isNull()) {
      connectToDocument();
    } else {
      // TODO: regularly poll for a document? wait for a signal?
    }
  } else {
    slotShowTree(DOM::Node());
  }

  updateIncrDecreaseButton();
}

void DOMTreeView::connectToDocument()
{
  assert(part);
  assert(!part->document().isNull());

  {
    // insert a style rule to indicate activated nodes
    try {
kDebug(90180) << "(1) part.document: " << part->document().handle();
      stylesheet = part->document().implementation().createCSSStyleSheet("-domtreeviewer-style", "screen");
kDebug(90180) << "(2)";
      stylesheet.insertRule(":focus { outline: medium #f00 solid }", 0);
      // ### for testing only
//       stylesheet.insertRule("body { background: #f0f !important }", 1);
kDebug(90180) << "(3)";
      active_node_rule = stylesheet.cssRules().item(0);
kDebug(90180) << "(4)";
      part->document().addStyleSheet(stylesheet);
kDebug(90180) << "(5)";
    } catch (DOM::CSSException &ex) {
      kDebug(90180) << "CSS Exception " << ex.code;
    } catch (DOM::DOMException &ex) {
      kDebug(90180) << "DOM Exception " << ex.code;
    }
  }

  slotShowTree(part->document());
  updateIncrDecreaseButton();
}

void DOMTreeView::disconnectFromActivePart()
{
  if (!part) return;

  // remove style sheet
  try {
    part->document().removeStyleSheet(stylesheet);
  } catch (DOM::CSSException &ex) {
    kDebug(90180) << "CSS Exception " << ex.code;
  } catch (DOM::DOMException &ex) {
    kDebug(90180) << "DOM Exception " << ex.code;
  }

}

void DOMTreeView::slotSetHtmlPartDelayed()
{
  connectToPart();
  emit htmlPartChanged(part);
}

void DOMTreeView::slotRestoreScrollOffset()
{
  m_listView->setUpdatesEnabled(true);
  m_listView->horizontalScrollBar()->setValue(scroll_ofs_x);
  m_listView->verticalScrollBar()->setValue(scroll_ofs_y);
}

void DOMTreeView::slotAddElementDlg()
{
  DOMListViewItem *item = static_cast<DOMListViewItem *>(m_listView->currentItem());
  if (!item) return;

  QString qname;
  QString namespc;
  SignalReceiver addBefore;

  {
    ElementEditDialog dlg(this);
    dlg.setModal(true);
    connect(dlg.button(KDialog::User2), SIGNAL(clicked()), &addBefore, SLOT(slot()));

    // ### activate when namespaces are supported
    dlg.elemNamespace->setEnabled(false);

    if (dlg.exec() != QDialog::Accepted) return;

    qname = dlg.elemName->text();
    namespc = dlg.elemNamespace->currentText();
  }

  DOM::Node curNode = item->node();

  try {
    DOM::Node parent = addBefore() ? curNode.parentNode() : curNode;
    DOM::Node after = addBefore() ? curNode : 0;

    // ### take namespace into account
    DOM::Node newNode = curNode.ownerDocument().createElement(qname);

    ManipulationCommand *cmd = new InsertNodeCommand(newNode, parent, after);
    mainWindow()->executeAndAddCommand(cmd);

    if (cmd->isValid()) activateNode(newNode);

  } catch (DOM::DOMException &ex) {
    mainWindow()->addMessage(ex.code, domErrorMessage(ex.code));
  }
}

void DOMTreeView::slotAddTextDlg()
{
  DOMListViewItem *item = static_cast<DOMListViewItem *>(m_listView->currentItem());
  if (!item) return;

  QString text;
  SignalReceiver addBefore;

  {
    TextEditDialog dlg(this);
    dlg.setModal(true);
    connect(dlg.button(KDialog::User2), SIGNAL(clicked()), &addBefore, SLOT(slot()));

    if (dlg.exec() != QDialog::Accepted) return;

    text = dlg.textPane->toPlainText();
  }

  DOM::Node curNode = item->node();

  try {
    DOM::Node parent = addBefore() ? curNode.parentNode() : curNode;
    DOM::Node after = addBefore() ? curNode : 0;

    DOM::Node newNode = curNode.ownerDocument().createTextNode(text);

    ManipulationCommand *cmd = new InsertNodeCommand(newNode, parent, after);
    mainWindow()->executeAndAddCommand(cmd);

    if (cmd->isValid()) activateNode(newNode);

  } catch (DOM::DOMException &ex) {
    mainWindow()->addMessage(ex.code, domErrorMessage(ex.code));
  }
}

// == DOM Node info panel =============================================

static QString *clickToAdd;

/**
 * List view item for attribute list.
 */
class AttributeListItem : public QTreeWidgetItem
{
  typedef QTreeWidgetItem super;

  bool _new;

public:
  AttributeListItem(QTreeWidget *parent, QTreeWidgetItem *prev)
  : super(parent, prev), _new(true)
  {
    if (!clickToAdd) clickToAdd = new QString(i18n("<Click to add>"));
    setText(0, *clickToAdd);
    QColor c = QApplication::palette().color(QPalette::Disabled,
                                             QPalette::Text);
    setForeground(0, c);
    setFirstColumnSpanned(true);
  }

  AttributeListItem(const QString &attrName, const QString &attrValue,
		QTreeWidget *parent, QTreeWidgetItem *prev)
  : super(parent, prev), _new(false)
  {
    setText(0, attrName);
    setText(1, attrValue);
  }

  bool isNew() const { return _new; }
  void setNew(bool s) { _new = s; }

  bool operator<(const QTreeWidgetItem &other) const
  {
    // Keep the "Add" item at the end
    if (_new)
      return false;
    if (static_cast<const AttributeListItem&>(other).isNew())
      return true;
    return QTreeWidgetItem::operator<(other);
  }
};

void DOMTreeView::initDOMNodeInfo()
{
  connect(m_listView, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
	  SLOT(initializeOptionsFromListItem(QTreeWidgetItem *)));

  connect(nodeAttributes, SIGNAL(itemRenamed(QTreeWidgetItem *, const QString &, int)),
	SLOT(slotItemRenamed(QTreeWidgetItem *, const QString &, int)));
  connect(nodeAttributes, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
	  SLOT(slotEditAttribute(QTreeWidgetItem *, int)));
  nodeAttributes->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(nodeAttributes, SIGNAL(customContextMenuRequested(const QPoint &)),
          SLOT(showInfoPanelContextMenu(const QPoint &)));

  connect(applyContent, SIGNAL(clicked()), SLOT(slotApplyContent()));

  ManipulationCommand::connect(SIGNAL(nodeChanged(const DOM::Node &)), this, SLOT(initializeOptionsFromNode(const DOM::Node &)));

  // ### nodeAttributes->setRenameable(0, true);
  // ### nodeAttributes->setRenameable(1, true);

  nodeInfoStack->setCurrentIndex(EmptyPanel);

  installEventFilter(nodeAttributes);
}

void DOMTreeView::initializeOptionsFromNode(const DOM::Node &node)
{
  infoNode = node;

  nodeName->clear();
  nodeType->clear();
  nodeNamespace->clear();
  nodeValue->clear();

  if (node.isNull()) {
    nodeInfoStack->setCurrentIndex(EmptyPanel);
    return;
  }

  nodeName->setText(node.nodeName().string());
  nodeType->setText(QString::number(node.nodeType()));
  nodeNamespace->setText(node.namespaceURI().string());
//   nodeValue->setText(node.value().string());

  initializeStyleSheetsFromDocument(node.ownerDocument());

  DOM::Element element = node;
  if (!element.isNull()) {
    initializeOptionsFromElement(element);
    return;
  }

  DOM::CharacterData cdata = node;
  if (!cdata.isNull()) {
    initializeOptionsFromCData(cdata);
    return;
  }

  // Fallback
  nodeInfoStack->setCurrentIndex(EmptyPanel);
}

void DOMTreeView::initializeOptionsFromListItem(QTreeWidgetItem *item)
{
  const DOMListViewItem *cur_item = static_cast<const DOMListViewItem *>(item);

//   kDebug(90180) << "cur_item: " << cur_item;
  initializeOptionsFromNode(cur_item ? cur_item->node() : DOM::Node());
}

void DOMTreeView::initializeOptionsFromElement(const DOM::Element &element)
{
  initializeDOMInfoFromElement(element);
  initializeCSSInfoFromElement(element);
}

void DOMTreeView::initializeDOMInfoFromElement(const DOM::Element &element)
{
  QTreeWidgetItem *last = 0;
  nodeAttributes->clear();

  DOM::NamedNodeMap attrs = element.attributes();
  unsigned long lmap = attrs.length();
  for (unsigned int j = 0; j < lmap; j++) {
    DOM::Attr attr = attrs.item(j);
//     kDebug(90180) << attr.name().string() << "=" << attr.value().string();
    QTreeWidgetItem *item = new AttributeListItem(attr.name().string(),
                                                  attr.value().string(),
                                                  nodeAttributes, last);
    last = item;
  }

  // append new item
  last = new AttributeListItem(nodeAttributes, last);

  nodeAttributes->sortByColumn(0, Qt::AscendingOrder);

  nodeInfoStack->setCurrentIndex(ElementPanel);
}

void DOMTreeView::initializeCSSInfoFromElement(const DOM::Element &element)
{
  DOM::Document doc = element.ownerDocument();
  DOM::AbstractView view = doc.defaultView();
  DOM::CSSStyleDeclaration styleDecl = view.getComputedStyle(element,
                                                             DOM::DOMString());

  unsigned long l = styleDecl.length();
  cssProperties->clear();
  cssProperties->setEnabled(true);
  QList<QTreeWidgetItem *> items;
  for (unsigned long i = 0; i < l; ++i) {
    DOM::DOMString name = styleDecl.item(i);
    DOM::DOMString value = styleDecl.getPropertyValue(name);

    QStringList values;
    values.append(name.string());
    values.append(value.string());
    items.append(new QTreeWidgetItem(static_cast<QTreeWidget*>(0), values));
  }

  cssProperties->insertTopLevelItems(0, items);
  cssProperties->resizeColumnToContents(0);
}

void DOMTreeView::initializeStyleSheetsFromDocument(const DOM::Document &doc)
{
  styleSheetsTree->clear();
  styleSheetsTree->setEnabled(true);

  DOM::StyleSheetList sheets = doc.styleSheets();
  unsigned long l = sheets.length();
  for (unsigned long i = 0; i < l; ++i) {
    DOM::StyleSheet sheet = sheets.item(i);
    // some info about the sheet itself
    QString str = "type=\"" + sheet.type().string() + "\"";
    if (!sheet.href().isEmpty())
      str += " href=\"" + sheet.href().string() + "\"";
    if (!sheet.title().isEmpty())
      str += " title=\"" + sheet.title().string() + "\"";
    if (sheet.disabled())
      str += " disabled";
    QStringList strList = QStringList(str);
    QTreeWidgetItem *topLevel = new QTreeWidgetItem(strList);
    styleSheetsTree->addTopLevelItem(topLevel);

    // list the content
    DOM::CSSStyleSheet cssSheet(sheet);
    if (!cssSheet.isNull()) {
      DOM::CSSRuleList cssRules = cssSheet.cssRules();
      unsigned long numRules = cssRules.length();
      for (unsigned long r = 0; r < numRules; ++r) {
        DOM::CSSRule rule = cssRules.item(r);
        QString cssText = rule.cssText().string();
        (void)new QTreeWidgetItem(topLevel, QStringList(cssText));
      }
    }
  }
}

void DOMTreeView::initializeOptionsFromCData(const DOM::CharacterData &cdata)
{
  initializeDOMInfoFromCData(cdata);
  initializeCSSInfoFromCData(cdata);
}

void DOMTreeView::initializeDOMInfoFromCData(const DOM::CharacterData &cdata)
{
  contentEditor->setText(cdata.data().string());

  DOM::Text text = cdata;
  contentEditor->setEnabled(!text.isNull());

  nodeInfoStack->setCurrentIndex(CDataPanel);
}

void DOMTreeView::initializeCSSInfoFromCData(const DOM::CharacterData &)
{
  cssProperties->clear();
  cssProperties->setEnabled(false);
}

void DOMTreeView::slotItemRenamed(QTreeWidgetItem *lvi, const QString &str, int col)
{
  AttributeListItem *item = static_cast<AttributeListItem *>(lvi);

  DOM::Element element = infoNode;
  if (element.isNull()) return; // Should never happen

  switch (col) {
    case 0: {
      ManipulationCommand *cmd;
//       kDebug(90180) << "col 0: " << element.nodeName() << " isNew: " << item->isNew();
      if (item->isNew()) {
        cmd = new AddAttributeCommand(element, str, item->text(1));
	item->setNew(false);
      } else
        cmd = new RenameAttributeCommand(element, item->text(0), str);

      mainWindow()->executeAndAddCommand(cmd);
      break;
    }
    case 1: {
      if (item->isNew()) { lvi->setText(1, QString()); break; }

      ChangeAttributeValueCommand *cmd = new ChangeAttributeValueCommand(
      			  element, item->text(0), str);
      mainWindow()->executeAndAddCommand(cmd);
      break;
    }
  }
}

void DOMTreeView::slotEditAttribute(QTreeWidgetItem *lvi, int col)
{
  if (!lvi) return;

  QString attrName;
  QString attrValue;
  bool newItem = static_cast<AttributeListItem*>(lvi)->isNew();
  int res = 0;

  {
    AttributeEditDialog dlg(this);
    dlg.setModal(true);
    if (!newItem) {
      dlg.attrName->setText(lvi->text(0));
      dlg.attrValue->setText(lvi->text(1));
    }
    if (col == 0) {
      dlg.attrName->setFocus();
      dlg.attrName->selectAll();
    } else {
      dlg.attrValue->setFocus();
      dlg.attrValue->selectAll();
    }

    res = dlg.exec();

    attrName = dlg.attrName->text();
    attrValue = dlg.attrValue->toPlainText();
  }

//   kDebug(90180) << "name=" << attrName << " value=" << attrValue;

  if (res == QDialog::Accepted) do {
    if (attrName.isEmpty()) break;

    if (lvi->text(0) != attrName) {
      // hack: set value to assign attribute/value pair in one go
      lvi->setText(1, attrValue);

      slotItemRenamed(lvi, attrName, 0);
      // Reget, item may have been changed
      QList<QTreeWidgetItem*> il = nodeAttributes->findItems(attrName,
                                                             Qt::MatchExactly,
                                                             0);
      assert(il.count() == 1);
      lvi = il.at(0);
    }

    if (lvi && lvi->text(1) != attrValue)
      slotItemRenamed(lvi, attrValue, 1);

  } while(false) /*end if*/;
}


void DOMTreeView::slotApplyContent()
{
  DOM::CharacterData cdata = infoNode;

  if (cdata.isNull()) return;

  ManipulationCommand *cmd = new ChangeCDataCommand(cdata, contentEditor->toPlainText());
  mainWindow()->executeAndAddCommand(cmd);
}

void DOMTreeView::showInfoPanelContextMenu(const QPoint &pos)
{
  QMenu *ctx = mainWindow()->infoPanelAttrContextMenu();
  Q_ASSERT(ctx);
  ctx->popup(nodeAttributes->mapToGlobal(pos));
}

void DOMTreeView::copyAttributes()
{
  // TODO implement me
}

void DOMTreeView::cutAttributes()
{
  // TODO implement me
}

void DOMTreeView::pasteAttributes()
{
  // TODO implement me
}

void DOMTreeView::deleteAttributes()
{
  MultiCommand *cmd = new MultiCommand(i18n("Delete Attributes"));
  QTreeWidgetItemIterator it(nodeAttributes, QTreeWidgetItemIterator::Selected);
  for (; *it; ++it) {
    AttributeListItem *item = static_cast<AttributeListItem *>(*it);
    if (item->isNew()) continue;

    cmd->addCommand(new RemoveAttributeCommand(infoNode, item->text(0)));
  }
  mainWindow()->executeAndAddCommand(cmd);
}

#include "domtreeview.moc"
