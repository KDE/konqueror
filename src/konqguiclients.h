/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __konq_guiclients_h__
#define __konq_guiclients_h__

#include "pluginmetadatautils.h"
#include "konq_popupmenu.h"

#include <KActionCollection>
#include <KXMLGUIClient>
#include <KPluginMetaData>

#include <QObject>
#include <QHash>

class QAction;
class KonqMainWindow;
class KonqView;

/**
 * @brief Class implementing Konqueror-specific functionality for KonqPopupMenu
 *
 * It provides actions, signals and slots to handle functionality which isn't provided
 * by KonqPopupMenu.
 *
 * @todo Merge this class with KonqPopupMenu
 */
class PopupMenuGUIClient : public QObject
{
    Q_OBJECT
public:
    // The action groups are inserted into @p actionGroups
    /**
     * @brief Constructor
     *
     * It inserts actions for @p embeddingServices and the actions @p showMenuBar
     * and @p stopFullScreen in the appropriate section of @p actionGroups.
     *
     * The actions for @p embeddingServices are added to this object's actionCollection(),
     * but @p showMenuBar and @p stopFullScreen are not.
     *
     * @param embeddingServices the available parts which to show in the "preview in" submenu
     * @param [inout] actionGroups the map where to insert the new actions
     * @param showMenuBar the action to toggle the menu bar, if any
     * @param stopFullScreen the action to turn off full screen, if any
     */
    PopupMenuGUIClient(const QVector<KPluginMetaData> &embeddingServices,
                       KonqPopupMenu::ActionGroupMap &actionGroups,
                       QAction *showMenuBar, QAction *stopFullScreen);

    ~PopupMenuGUIClient() override; //!< Destructor

    /**
     * @brief The action collection used by this object
     * @return the action collection used by this object
     */
    KActionCollection *actionCollection()
    {
        return &m_actionCollection;
    }

signals:
    /**
     * @brief Signal emitted when the user clicks on one of the Preview In... actions
     *
     * @param part the part chosen by the user
     */
    void openEmbedded(const KPluginMetaData &part);

private slots:

    /**
     * @brief Slot called when the user activates one of the Preview In ... actions
     *
     * It emits the openEmbedded() signal for the action.
     *
     * @warning This uses `QObject::sender()` to determine which action was triggered,
     * so it must only be called from the slot connected with the `triggered()` signal
     */
    void slotOpenEmbedded();

private:

    /**
     * @brief Creates a Preview In... action for the given part
     *
     * @param idx a number to identify the part. It can be anything but it must
     * be unique for this client, as its string representation will be used as
     * name for the action in actionCollection()
     * @param name the text of the action
     * @param plugin the metadata of the part
     * @return an action to preview in the given part
     */
    QAction *addEmbeddingPlugin(int idx, const QString &name, const KPluginMetaData &plugin);

    KActionCollection m_actionCollection; //!< The action collection
    QVector<KPluginMetaData>  m_embeddingServices; //!< A list of available parts for previews
};

/**
 * @brief Class which handles views which can be turned on or off, such as the sidebar or the terminal emulator
 *
 * Toggable views are provided by parts with the `KParts::PartCapability::BrowserView`
 * capability and having the `X-KDE-BrowserView-Toggable` and `X-KDE-BrowserView-ToggableView-Orientation`
 * entries in their metadata.
 *
 * This class provides actions to toggle each available toggable view and the slot connected to them
 * which toggles a view on or off.
 */
class ToggleViewGUIClient : public QObject
{
    Q_OBJECT
public:

    /**
     * @brief Constructor
     * @param mainWindow the main window whose toggable views this object will manage
     */
    explicit ToggleViewGUIClient(KonqMainWindow *mainWindow);

    ~ToggleViewGUIClient() override; //!< Destructor

    /**
     * @brief Whether this object is empty
     *
     * A ToggleViewGUIClient is considered empty if it couldn't find any part
     * providing toggable views.
     *
     * @return `true` if this object is empty and `false` otherwise
     */
    bool empty() const
    {
        return m_empty;
    }

    /**
     * @brief The actions to toggle all available toggable views
     *
     * These actions are created by the constructor and are already connected with
     * the slots which handles their `toggled()` signal.
     *
     * @return the actions to toggle all available toggable views
     */
    QList<QAction *> actions() const;

    /**
     * @brief The action to toggle the view with a given name
     *
     * @param name the name of the action. It corresponds to the plugin id of
     * the corresponding part
     * @return the action to toggle the view associated with the part with plugin
     * id @name or `nullptr` if no such action exists
     */
    QAction *action(const QString &name)
    {
        return m_actions[ name ];
    }

    /**
     * @brief Updates the list of visible toggable views in the configuration file
     *
     * @param add whether to add or remove a toggable view from the list
     * @param serviceName the plugin id of the part associated with the view to add or remove
     */
    void saveConfig(bool add, const QString &serviceName);

private Q_SLOTS:
    /**
     * @brief Slot called when a view has been toggled on or off using the corresponding action
     *
     * @warning This function should only be called from the slot associated with the `triggered()`
     * signal of the action, since it uses `QObject::sender()` to determine which action
     * was triggered
     *
     * If the view should be turned on, it creates the view, splits the main window and adds the view to
     * the resulting container frame and activates it. If the view should be turned off, it deletes the
     * view.
     *
     * @param toggle whether the view should be turned on or off
     */
    void slotToggleView(bool toggle);

    /**
     * @brief Slot called when a new is added to the main window
     *
     * If the view is a toggable view, this function ensures that the corresponding
     * action is checked and that the view is added to the list of open toggable views
     * in the configuration file
     */
    void slotViewAdded(KonqView *view);

    /**
     * @brief Slot called when a new is removed from the main window
     *
     * If the view is a toggable view, this function ensures that the corresponding
     * action is unchecked and that the view is removed from the list of open toggable views
     * in the configuration file
     */
    void slotViewRemoved(KonqView *view);
private:
    KonqMainWindow *m_mainWindow; //!< The main window
    QHash<QString, QAction *> m_actions; //!< A list of action to toggle toggable views

    //TODO: is this really needed? Wouldn't it be enough to use m_actions.isEmpty()?
    bool m_empty; //!< Whether the object is empty or not
    /**
     * @brief The orientation for each toggable view
     *
     * The keys are the name of the actions. A value of `true` means that the toggle
     * view is horizontal, while a `false` means it's vertical
     */
    QMap<QString, bool> m_mapOrientation;
};

#endif
