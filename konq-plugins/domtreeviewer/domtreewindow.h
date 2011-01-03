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

#ifndef domtreewindow_H
#define domtreewindow_H

#include <kxmlguiwindow.h>

#include <qpointer.h>


namespace domtreeviewer {
  class ManipulationCommand;
}

namespace KParts {
  class Part;
  class PartManager;
}

class DOMTreeView;
class PluginDomtreeviewer;

class QAction;
class KConfig;
class KUndoStack;
class KHTMLPart;
class MessageDialog;

class QMenu;

/**
 * This class serves as the main window for DOM Tree Viewer.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Leo Savernik
 */
class DOMTreeWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    DOMTreeWindow(PluginDomtreeviewer *plugin);

    /**
     * Default Destructor
     */
    virtual ~DOMTreeWindow();

    /**
     * returns the dom tree view
     */
    DOMTreeView *view() const { return m_view; }

    /**
     * returns the command history
     */
    KUndoStack *commandHistory() const { return m_commandHistory; }

    /**
     * creates and returns the context menu for the list info panel
     */
    QMenu *createInfoPanelAttrContextMenu();

    /**
     * returns the context menu for the list info panel
     */
    QMenu *infoPanelAttrContextMenu() { return infopanel_ctx; }

    /**
     * creates and returns the context menu for the DOM tree view
     */
    QMenu *createDOMTreeViewContextMenu();

    /**
     * returns the context menu for the DOM tree view
     */
    QMenu *domTreeViewContextMenu() { return domtree_ctx; }

    /**
     * Executes the given command and adds it to the history.
     *
     * If the command could not be executed, it will not be added.
     */
    void executeAndAddCommand(domtreeviewer::ManipulationCommand *);

    /**
     * Returns the config object for this plugin.
     */
    KConfig *config() const { return _config; }

    /** returns the attribute delete action */
    QAction *deleteAttributeAction() const { return del_attr; }
    /** returns the node delete action */
    QAction *deleteNodeAction() const { return del_tree; }

public slots:
    /**
     * Adds a log message
     * @param id message id
     * @param msg message text
     */
    void addMessage(int id, const QString &msg);

    /**
     * Displays the message log window.
     */
    void showMessageLog();

protected:
    /**
     * Overridden virtuals for Qt drag 'n drop (XDND)
     */
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
protected:
    /**
     * This function is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfigGroup &);

    /**
     * This function is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(const KConfigGroup &);


private slots:
    void slotCut();
    void slotCopy();
    void slotPaste();
    //void slotUndo();
    //void slotRedo();
    void slotFind();
    void optionsConfigureToolbars();
    void optionsPreferences();
    void newToolbarConfig();

    void changeStatusbar(const QString& text);
    void changeCaption(const QString& text);

    void slotHtmlPartChanged(KHTMLPart *);
    void slotActivePartChanged(KParts::Part *);
    void slotPartRemoved(KParts::Part *);
    void slotClosePart();

private:
    void setupAccel();
    void setupActions();

private:
    PluginDomtreeviewer *m_plugin;
    DOMTreeView *m_view;
    MessageDialog *msgdlg;

    KUndoStack *m_commandHistory;
    QMenu *infopanel_ctx;
    QMenu *domtree_ctx;
    KConfig *_config;

    QAction *del_tree, *del_attr;

    QPointer<KParts::PartManager> part_manager;
};

#endif // domtreewindow_H
