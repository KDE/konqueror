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

#ifndef DOMTREEVIEW_H
#define DOMTREEVIEW_H

#include <dom/css_stylesheet.h>
#include <dom/css_rule.h>
#include <dom/dom_node.h>
#include <qtreewidget.h>

#include "ui_domtreeviewbase.h"

namespace DOM {
  class Element;
  class CharacterData;
}

class DOMListViewItem;
class DOMTreeWindow;

class KPushButton;
class KFindDialog;
class KHTMLPart;

class DOMTreeView : public QWidget, public Ui::DOMTreeViewBase
{
    Q_OBJECT

    public:
	explicit DOMTreeView(QWidget *parent, bool allowSaving = true);
	~DOMTreeView();

        KHTMLPart *htmlPart() const { return part; }
        void setHtmlPart(KHTMLPart *);

	/** returns the main window */
	DOMTreeWindow *mainWindow() const;

    protected:
    /*
	void saveTreeAsHTML(const DOM::Node &pNode);
    */
        virtual bool eventFilter(QObject *o, QEvent *e);

    signals:
        /** emitted when the part has been changed. */
	void htmlPartChanged(KHTMLPart *part);

    public slots:
        void refresh();
	void increaseExpansionDepth();
	void decreaseExpansionDepth();
	void setMessage(const QString &msg);
	void hideMessageLine();

	void moveToParent();

	void activateNode(const DOM::Node &node);
	void deleteNodes();

	/**
	 * Takes measures to disconnect this instance from the current html
	 * part as long as it is active.
	 */
	void disconnectFromActivePart();

	/**
	 * Takes measures to disconnect this instance from the current html
	 * part when it is being torn down.
	 */
	void disconnectFromTornDownPart();

	/**
	 * Connects to the current html part.
	 */
	void connectToPart();

	void slotFindClicked();
	void slotAddElementDlg();
	void slotAddTextDlg();

    protected slots:
        void slotShowNode(const DOM::Node &pNode);
        void slotShowTree(const DOM::Node &pNode);
	void slotItemClicked(QTreeWidgetItem *);
	void slotRefreshNode(const DOM::Node &pNode);
#if 0 // ### port to Qt 4
        void slotMovedItems(Q3PtrList<QTreeWidgetItem> &items, Q3PtrList<QTreeWidgetItem> &afterFirst, Q3PtrList<QTreeWidgetItem> &afterNow);
#endif
	void slotPrepareMove();
	void slotSearch();

	// void slotSaveClicked();

	void slotPureToggled(bool);
        void slotShowAttributesToggled(bool);
	void slotHighlightHTMLToggled(bool);

	void showDOMTreeContextMenu(const QPoint &);

	void slotSetHtmlPartDelayed();
	void slotRestoreScrollOffset();

    private:
	QHash<void*, DOMListViewItem*> m_itemdict;
	DOM::Node m_document;

	uint m_expansionDepth, m_maxDepth;
	bool m_bPure, m_bShowAttributes, m_bHighlightHTML;

    private:
	void connectToDocument();
	void showRecursive(const DOM::Node &pNode, const DOM::Node &node,
			   uint depth);

	// void saveRecursive(const DOM::Node &node, int ident);

	void searchRecursive(DOMListViewItem *cur_item,
			     const QString &searchText,
			     Qt::CaseSensitivity caseSensitivity);

        void adjustDepth();
	void adjustDepthRecursively(QTreeWidgetItem *cur_item,  uint currDepth);
	void highlightHTML(DOMListViewItem *cur_item,
			   const QString &nodeName);

	DOMListViewItem* addElement(const DOM::Node &node, DOMListViewItem *cur_item,
			bool isLast);
        void updateIncrDecreaseButton();

    private:
	KFindDialog* m_findDialog;

	KHTMLPart *part;
	QTextStream* m_textStream;

	KPushButton* m_saveButton;
	QObject *focused_child;
	DOM::Node current_node;
	DOM::CSSStyleSheet stylesheet;
	DOM::CSSRule active_node_rule;

	bool _refreshed;
	int scroll_ofs_x, scroll_ofs_y;


    // == DOM Node Info panel ======================================

    public:
        // Keep in sync with the widget stack children
        enum InfoPanel { ElementPanel, CDataPanel, EmptyPanel };

    public slots:
        void initializeOptionsFromNode(const DOM::Node &);
	void initializeOptionsFromListItem(QTreeWidgetItem *);

	void copyAttributes();
	void cutAttributes();
	void pasteAttributes();
	void deleteAttributes();

    private:
        void initDOMNodeInfo();

        void initializeStyleSheetsFromDocument(const DOM::Document &);

        void initializeOptionsFromElement(const DOM::Element &);
        void initializeDOMInfoFromElement(const DOM::Element &);
        void initializeCSSInfoFromElement(const DOM::Element &);

        void initializeOptionsFromCData(const DOM::CharacterData &);
        void initializeDOMInfoFromCData(const DOM::CharacterData &);
        void initializeCSSInfoFromCData(const DOM::CharacterData &);

    private slots:
	void slotItemRenamed(QTreeWidgetItem *, const QString &str, int col);
	void slotEditAttribute(QTreeWidgetItem *, int col);
	void slotApplyContent();

	void showInfoPanelContextMenu(const QPoint &);

    private:
        DOM::Node infoNode;	// node these infos apply to

    // == End DOM Node Info panel ==================================

};

#endif

