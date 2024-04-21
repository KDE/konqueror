/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2004 Arend van Beelen jr. <arend@auton.nl>
    SPDX-FileCopyrightText: 2009 Fredy Yanardi <fyanardi@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SEARCHBAR_PLUGIN
#define SEARCHBAR_PLUGIN

#include <KHistoryComboBox>
#include <KUriFilter>

#include "asyncselectorinterface.h"
#include <konq_kpart_plugin.h>

#include <QStringList>
#include <QItemDelegate>
#include <QPixmap>


namespace KParts
{
class Part;
class ReadOnlyPart;
}

class WebShortcutWidget;

class QAction;
class QMenu;
class QTimer;
class QWidgetAction;

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
    ~SearchBarCombo() override;

    /**
     * Returns the icon currently displayed in the combo box.
     */
    const QPixmap &icon() const;

    /**
     * Sets the icon displayed in the combo box.
     */
    void setIcon(const QPixmap &icon);

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
     * Clear all previously set suggestion items for this ComboBox
     */
    void clearSuggestions();

Q_SIGNALS:
    /**
     * Emitted when the icon was clicked.
     */
    void iconClicked();

protected:
    /**
     * Captures mouse clicks and emits iconClicked() if the icon
     * was clicked.
     */
    void mousePressEvent(QMouseEvent *e) override;

    /**
     * Captures context menu requests and ignores any over the icon.
     */
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    /**
     * See whether a mouse click is over the search engine icon.
     *
     * @param x X coordinate of mouse event
     * @return @c true if the click was over the icon
     */
    bool overIcon(int x);

private Q_SLOTS:
    void historyCleared();

private:
    QPixmap m_icon;
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
class SearchBarPlugin : public KonqParts::Plugin
{
    Q_OBJECT

public:
    /** Possible search modes */
    enum SearchModes { FindInThisPage = 0, UseSearchProvider };

    SearchBarPlugin(QObject *parent,
                    const QVariantList &);
    ~SearchBarPlugin() override;

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

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

    void HTMLLoadingStarted();

    void HTMLDocLoaded();

    void insertOpenSearchEntries(const QList<AsyncSelectorInterface::Element> &elements);

    void openSearchEngineAdded(const QString &name, const QString &searchUrl, const QString &fileName);

    void webShortcutSet(const QString &name, const QString &webShortcut, const QString &fileName);

private:
    bool enableFindInPage() const;
    void nextSearchEntry();
    void previousSearchEntry();

    QPointer<KParts::ReadOnlyPart> m_part;
    SearchBarCombo *m_searchCombo;
    QWidgetAction *m_searchComboAction;
    QList<QAction *> m_addSearchActions;
    QMenu *m_popupMenu;
    WebShortcutWidget *m_addWSWidget;
    QPixmap m_searchIcon;
    SearchModes m_searchMode;
    QString m_providerName;
    bool m_urlEnterLock;
    QString m_lastSearch;
    QString m_currentEngine;
    QStringList m_searchEngines;
    QMap<QString, KUriFilterSearchProvider> m_searchProviders;
    QChar m_delimiter;
    QMap<QString, QString> m_openSearchDescs;
    bool m_reloadConfiguration;
    QString m_searchProvidersDir;
};

/**
 * An item delegate for combo box completion items, to give some fancy stuff
 * to the completion items
 */
class SearchBarItemDelegate : public QItemDelegate
{
public:
    SearchBarItemDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // SEARCHBAR_PLUGIN
