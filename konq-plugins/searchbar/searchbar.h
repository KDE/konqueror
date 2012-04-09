/* This file is part of the KDE project
   Copyright (C) 2004 Arend van Beelen jr. <arend@auton.nl>
   Copyright (C) 2009 Fredy Yanardi <fyanardi@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef SEARCHBAR_PLUGIN
#define SEARCHBAR_PLUGIN

#include <KDE/KHistoryComboBox>
#include <KDE/KUriFilter>
#include <KDE/KParts/Plugin>

#include <QtCore/QStringList>
#include <QItemDelegate>
#include <QPixmap>

namespace KParts {
    class Part;
    class ReadOnlyPart;
}

class OpenSearchManager;
class WebShortcutWidget;

class QAction;
class QMenu;
class QTimer;

/**
 * Combo box which catches mouse clicks on the pixmap.
 */
class SearchBarCombo : public KHistoryComboBox
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    SearchBarCombo(QWidget *parent);

    /**
     * Destructor.
     */
    ~SearchBarCombo();

    /**
     * Returns the icon currently displayed in the combo box.
     */
    const QPixmap &icon() const;

    /**
     * Sets the icon displayed in the combo box.
     */
    void setIcon(const QPixmap &icon);

    /**
     * Check/uncheck the menu entry for setting suggestion according to enable
     */
    void setSuggestionEnabled(bool enable);

    /**
     * Finds a history item by its text.
     * @return The item number, or -1 if the item is not found.
     */
    int findHistoryItem(const QString &text);

    /**
     * Sets whether the plugin is active. It can be inactive
     * in case the current Konqueror part isn't a KHTML part.
     */
    void setPluginActive(bool pluginActive);

    /**
     * Set the suggestion items after to be displayed by this ComboBox
     * This method will automatically pop up the ComboBox's list.
     */
    void setSuggestionItems(const QStringList &suggestions);

    /**
     * Clear all presviously set suggestion items for this ComboBox
     */
    void clearSuggestions();

Q_SIGNALS:
    /**
     * Emitted when the icon was clicked.
     */
    void iconClicked();

    /**
     * Emitted when the suggestion enable state is changed via the popup menu
     */
    void suggestionEnabled(bool enable);  

protected:
    /**
     * Captures mouse clicks and emits iconClicked() if the icon
     * was clicked.
     */
    virtual void mousePressEvent(QMouseEvent *e);

private Q_SLOTS:
    void historyCleared();
    void addEnableMenuItem(QMenu *menu);

private:
    QPixmap m_icon;
    QAction *m_enableAction;
    QStringList m_suggestions;
};

/**
 * Plugin that provides a search bar for Konqueror. This search bar is located
 * next to the location bar and will show a small icon indicating the search
 * provider it will use.
 *
 * @author Arend van Beelen jr. <arend@auton.nl>
 * @version $Id$
 */
class SearchBarPlugin : public KParts::Plugin
{
    Q_OBJECT

public:
    /** Possible search modes */
    enum SearchModes { FindInThisPage = 0, UseSearchProvider };

    SearchBarPlugin(QObject *parent,
                    const QVariantList &);
    virtual ~SearchBarPlugin();

protected:
    bool eventFilter(QObject *o, QEvent *e);

private Q_SLOTS:
    /**
     * Starts a search by putting the query URL from the selected
     * search provider in the locationbar and calling goURL()
     */
    void startSearch(const QString &search);

    /**
     * Sets the icon to indicate which search engine is used.
     */
    void setIcon();

    /**
     * Opens the selection menu.
     */
    void showSelectionMenu();

    void useFindInThisPage();
    void menuActionTriggered(QAction *);
    void selectSearchEngines();
    void configurationChanged();
    void reloadConfiguration();

    /**
     * Show or hide the combo box
     */
    void updateComboVisibility();

    void focusSearchbar();

    void searchTextChanged(const QString &text);

    void addSearchSuggestion(const QStringList &suggestion);

    void requestSuggestion();

    void enableSuggestion(bool enable);

    void HTMLLoadingStarted();

    void HTMLDocLoaded();

    void openSearchEngineAdded(const QString &name, const QString &searchUrl, const QString &fileName);

    void webShortcutSet(const QString &name, const QString &webShortcut, const QString &fileName);

private:
    bool enableFindInPage() const;
    void nextSearchEntry();
    void previousSearchEntry();

    QWeakPointer<KParts::ReadOnlyPart> m_part;
    SearchBarCombo* m_searchCombo;
    KAction* m_searchComboAction;
    QList<KAction *> m_addSearchActions;
    QMenu* m_popupMenu;
    WebShortcutWidget* m_addWSWidget;
    QPixmap m_searchIcon;
    SearchModes m_searchMode;
    QString m_providerName;
    bool m_urlEnterLock;
    QString m_lastSearch;
    QString m_currentEngine;
    QStringList m_searchEngines;
    QMap<QString, KUriFilterSearchProvider> m_searchProviders;
    QChar m_delimiter;
    OpenSearchManager* m_openSearchManager;
    QTimer* m_timer;
    bool m_suggestionEnabled;
    QMap<QString, QString> m_openSearchDescs;
    bool m_reloadConfiguration;
};

/**
 * An item delegate for combo box completion items, to give some fancy stuff
 * to the completion items
 */
class SearchBarItemDelegate : public QItemDelegate
{
public:
    SearchBarItemDelegate(QObject *parent = 0);
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
};

#endif // SEARCHBAR_PLUGIN
