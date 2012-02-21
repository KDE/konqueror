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

#include "searchbar.h"

#include "OpenSearchManager.h"
#include "WebShortcutWidget.h"

#include <KDE/KAction>
#include <KDE/KBuildSycocaProgressDialog>
#include <KDE/KCompletionBox>
#include <KDE/KConfigGroup>
#include <KDE/KSharedConfig>
#include <KDE/KDebug>
#include <KDE/KDesktopFile>
#include <KDE/KPluginFactory>
#include <KDE/KGlobal>
#include <KDE/KIconLoader>
#include <KDE/KLocale>
#include <KDE/KActionCollection>
#include <KDE/KRun>
#include <KDE/KMainWindow>
#include <KDE/KStandardDirs>
#include <KParts/Part>
#include <KParts/BrowserExtension>
#include <KParts/TextExtension>
#include <KParts/HtmlExtension>


//#include <kparts/mainwindow.h>

#include <QLineEdit>
#include <QApplication>
#include <QtCore/QTimer>
#include <QMenu>
#include <QStyle>
#include <QPixmap>
#include <QPainter>
#include <QMouseEvent>
#include <QtDBus/QtDBus>


K_PLUGIN_FACTORY(SearchBarPluginFactory, registerPlugin<SearchBarPlugin>();)
K_EXPORT_PLUGIN(SearchBarPluginFactory("searchbarplugin"))

SearchBarPlugin::SearchBarPlugin(QObject *parent,
                                 const QVariantList &) :
    KParts::Plugin(parent),
    m_popupMenu(0),
    m_addWSWidget(0),
    m_searchMode(UseSearchProvider),
    m_urlEnterLock(false),
    m_openSearchManager(new OpenSearchManager(this)),
    m_reloadConfiguration(false)
{
    m_searchCombo = new SearchBarCombo(0);
    m_searchCombo->lineEdit()->installEventFilter(this);
    connect(m_searchCombo, SIGNAL(activated(QString)),
            SLOT(startSearch(QString)));
    connect(m_searchCombo, SIGNAL(iconClicked()), SLOT(showSelectionMenu()));
    m_searchCombo->setWhatsThis(i18n("Search Bar<p>"
                                     "Enter a search term. Click on the icon to change search mode or provider.</p>"));
    connect(m_searchCombo, SIGNAL(suggestionEnabled(bool)), this, SLOT(enableSuggestion(bool)));

    m_searchComboAction = actionCollection()->addAction("toolbar_search_bar");
    m_searchComboAction->setText(i18n("Search Bar"));
    m_searchComboAction->setDefaultWidget(m_searchCombo);
    m_searchComboAction->setShortcutConfigurable(false);

    KAction *a = actionCollection()->addAction("focus_search_bar");
    a->setText(i18n("Focus Searchbar"));
    a->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_S));
    connect(a, SIGNAL(triggered()), this, SLOT(focusSearchbar()));

    configurationChanged();

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()), SLOT(requestSuggestion()));

    // parent is the KonqMainWindow and we want to listen to PartActivateEvent events.
    parent->installEventFilter(this);

    connect(m_searchCombo->lineEdit(), SIGNAL(textEdited(QString)),
            SLOT(searchTextChanged(QString)));
    connect(m_openSearchManager, SIGNAL(suggestionReceived(QStringList)),
            SLOT(addSearchSuggestion(QStringList)));
    connect(m_openSearchManager, SIGNAL(openSearchEngineAdded(QString,QString,QString)),
            SLOT(openSearchEngineAdded(QString,QString,QString)));
    
    QDBusConnection::sessionBus().connect(QString(), QString(), "org.kde.KUriFilterPlugin",
                                          "configure", this, SLOT(reloadConfiguration()));
}

SearchBarPlugin::~SearchBarPlugin()
{
    KConfigGroup config(KGlobal::config(), "SearchBar");
    config.writeEntry("Mode", (int) m_searchMode);
    config.writeEntry("CurrentEngine", m_currentEngine);
    config.writeEntry("SuggestionEnabled", m_suggestionEnabled);

    delete m_searchCombo;
    m_searchCombo = 0;
}

bool SearchBarPlugin::eventFilter(QObject *o, QEvent *e)
{
    if (qobject_cast<KMainWindow*>(o) && KParts::PartActivateEvent::test(e)) {
        KParts::PartActivateEvent* partEvent = static_cast<KParts::PartActivateEvent *>(e);
        KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(partEvent->part());
        //kDebug() << "Embedded part changed to " << part;
        if (part && (!m_part || part != m_part.data())) {
            m_part = part;

            // Delete the popup menu so a new one can be created with the
            // appropriate entries the next time it is shown...
            // ######## TODO: This loses the opensearch entries for the old part!!!
            if (m_popupMenu) {
              delete m_popupMenu;
              m_popupMenu = 0;
              m_addSearchActions.clear(); // the actions had the menu as parent, so they're deleted now
            }

            // Change the search mode if it is set to FindInThisPage since
            // that feature is currently KHTML specific. It is also completely
            // redundant and unnecessary.
            if (m_searchMode == FindInThisPage && enableFindInPage())
              nextSearchEntry();

            connect(part, SIGNAL(completed()), this, SLOT(HTMLDocLoaded()));
            connect(part, SIGNAL(started(KIO::Job*)), this, SLOT(HTMLLoadingStarted()));
        }
        // Delay since when destroying tabs part 0 gets activated for a bit, before the proper part
        QTimer::singleShot(0, this, SLOT(updateComboVisibility()));
    }
    else if (o == m_searchCombo->lineEdit() && e->type() == QEvent::KeyPress) {
        QKeyEvent *k = (QKeyEvent *)e;
        if (k->modifiers() & Qt::ControlModifier) {
            if (k->key()==Qt::Key_Down) {
                nextSearchEntry();
                return true;
            }
            if (k->key()==Qt::Key_Up) {
                previousSearchEntry();
                return true;
            }
        }
    }
    return KParts::Plugin::eventFilter(o, e);
}

void SearchBarPlugin::nextSearchEntry()
{
    if (m_searchMode == FindInThisPage) {
        m_searchMode = UseSearchProvider;
        if (m_searchEngines.isEmpty())
            m_currentEngine = QLatin1String("google");
        else
            m_currentEngine = m_searchEngines.first();
    } else {
        const int index = m_searchEngines.indexOf(m_currentEngine) + 1;
        if (index >= m_searchEngines.count())
            m_searchMode = FindInThisPage;
        else
            m_currentEngine = m_searchEngines.at(index);
    }
    setIcon();
}

void SearchBarPlugin::previousSearchEntry()
{
    if (m_searchMode == FindInThisPage) {
        m_searchMode = UseSearchProvider;
        if (m_searchEngines.isEmpty())
            m_currentEngine = QLatin1String("google");
        else
            m_currentEngine =  m_searchEngines.last();
    } else {
        const int index = m_searchEngines.indexOf(m_currentEngine) - 1;
        if (index <= 0)
            m_searchMode = FindInThisPage;
        else
            m_currentEngine = m_searchEngines.at(index);
    }
    setIcon();
}

// Called when activating the combobox (Key_Return, or item in popup or in completionbox)
void SearchBarPlugin::startSearch(const QString &search)
{
    if (m_urlEnterLock || search.isEmpty() || !m_part) {
        return;
    }
    m_timer->stop();
    m_lastSearch = search;

    if (m_searchMode == FindInThisPage) {
        KParts::TextExtension* textExt = KParts::TextExtension::childObject(m_part.data());
        if (textExt)
            textExt->findText(search, 0);
    } else if (m_searchMode == UseSearchProvider) {
        m_urlEnterLock = true;
        const KUriFilterSearchProvider& provider = m_searchProviders.value(m_currentEngine);
        KUriFilterData data;
        data.setData(provider.defaultKey() + m_delimiter + search);
        //kDebug() << "Query:" << (provider.defaultKey() + m_delimiter + search);
        if (!KUriFilter::self()->filterSearchUri(data, KUriFilter::WebShortcutFilter)) {          
            kWarning() << "Failed to filter using web shortcut:" << provider.defaultKey();
            return;
        }
        
        KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject(m_part.data());
        if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
            KParts::OpenUrlArguments arguments;
            KParts::BrowserArguments browserArguments;
            browserArguments.setNewTab(true);
            if (ext)
                emit ext->createNewWindow(data.uri(), arguments, browserArguments);
        } else {
            if (ext) {
                emit ext->openUrlRequest(data.uri());
                if (m_part)
                    m_part.data()->widget()->setFocus(); // #152923
            }
        }
    }

    m_searchCombo->addToHistory(search);
    m_searchCombo->setItemIcon(0, m_searchIcon);

    m_urlEnterLock = false;
}

void SearchBarPlugin::setIcon()
{
    if (m_searchMode == FindInThisPage) {
        m_searchIcon = SmallIcon("edit-find");
    } else {
        const QString engine = (m_currentEngine.isEmpty() ? m_searchEngines.first() : m_currentEngine);
        //kDebug() << "Icon Name:" << m_searchProviders.value(engine).iconName();
        const QString iconName = m_searchProviders.value(engine).iconName();
        if (iconName.startsWith(QLatin1Char('/')))
            m_searchIcon = QPixmap(iconName);
        else
            m_searchIcon = SmallIcon(iconName);
    }

    // Create a bit wider icon with arrow
    QPixmap arrowmap = QPixmap(m_searchIcon.width()+5,m_searchIcon.height()+5);
    arrowmap.fill(m_searchCombo->lineEdit()->palette().color(m_searchCombo->lineEdit()->backgroundRole()));
    QPainter p(&arrowmap);
    p.drawPixmap(0, 2, m_searchIcon);
    QStyleOption opt;
    opt.state = QStyle::State_None;
    opt.rect = QRect(arrowmap.width()-6, arrowmap.height()-5, 6, 5);
    m_searchCombo->style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &opt, &p, m_searchCombo);
    p.end();
    m_searchIcon = arrowmap;
    m_searchCombo->setIcon(m_searchIcon);

    // Set the placeholder text to be the search engine name...
    if (m_searchProviders.contains(m_currentEngine)) {
        m_searchCombo->lineEdit()->setPlaceholderText(m_searchProviders.value(m_currentEngine).name());
    }
}

void SearchBarPlugin::showSelectionMenu()
{
    // Update the configuration, if needed, before showing the menu items...
    if (m_reloadConfiguration)
        configurationChanged();
    
    if (!m_popupMenu) {
        m_popupMenu = new QMenu(m_searchCombo);
        m_popupMenu->setObjectName("search selection menu");

        if (enableFindInPage()) {
            m_popupMenu->addAction(KIcon("edit-find"), i18n("Find in This Page"), this, SLOT(useFindInThisPage()));
            m_popupMenu->addSeparator();
        }

        for (int i=0, count=m_searchEngines.count(); i != count; ++i) {
            const KUriFilterSearchProvider& provider = m_searchProviders.value(m_searchEngines.at(i));
            QAction* action = m_popupMenu->addAction(KIcon(provider.iconName()), provider.name());
            action->setData(qVariantFromValue(i));
        }

        m_popupMenu->addSeparator();
        m_popupMenu->addAction(KIcon("preferences-web-browser-shortcuts"), i18n("Select Search Engines..."),
                               this, SLOT(selectSearchEngines()));
        connect(m_popupMenu, SIGNAL(triggered(QAction*)), SLOT(menuActionTriggered(QAction*)));
    } else {
        Q_FOREACH (KAction *action, m_addSearchActions) {
            m_popupMenu->removeAction(action);
            delete action;
        }
        m_addSearchActions.clear();
    }

    QList<QAction *> actions = m_popupMenu->actions();
    QAction *before = 0;
    if (actions.size() > 1) {
        before = actions[actions.size() - 2];
    }

    Q_FOREACH (const QString &title, m_openSearchDescs.keys()) {
        KAction *addSearchAction = new KAction(m_popupMenu);
        addSearchAction->setText(i18n("Add %1...", title));
        m_addSearchActions.append(addSearchAction);
        addSearchAction->setData(QVariant::fromValue(title));
        m_popupMenu->insertAction(before, addSearchAction);
    }

    m_popupMenu->popup(m_searchCombo->mapToGlobal(QPoint(0, m_searchCombo->height() + 1)));
}

void SearchBarPlugin::useFindInThisPage()
{
    m_searchMode = FindInThisPage;
    setIcon();
}

void SearchBarPlugin::menuActionTriggered(QAction *action)
{
    bool ok = false;
    const int id = action->data().toInt(&ok);
    if (ok) {
        m_searchMode = UseSearchProvider;
        m_currentEngine = m_searchEngines.at(id);
        setIcon();
        m_openSearchManager->setSearchProvider(m_currentEngine);
        m_searchCombo->lineEdit()->selectAll();
        return;
    }

    m_searchCombo->lineEdit()->setPlaceholderText(QString());
    const QString openSearchTitle = action->data().toString();
    if (!openSearchTitle.isEmpty()) {
        const QString openSearchHref = m_openSearchDescs.value(openSearchTitle);
        KUrl url;
        if (QUrl(openSearchHref).isRelative()) {
            const KUrl docUrl = m_part ? m_part.data()->url() : KUrl();
            QString host = docUrl.scheme() + QLatin1String("://") + docUrl.host();
            if (docUrl.port() != -1) {
                host += QLatin1String(":") + QString::number(docUrl.port());
            }
            url = KUrl(docUrl, openSearchHref);
        }
        else {
            url = KUrl(openSearchHref);
        }
        //kDebug() << "Adding open search Engine: " << openSearchTitle << " : " << openSearchHref;
        m_openSearchManager->addOpenSearchEngine(url, openSearchTitle);
    }
}

void SearchBarPlugin::selectSearchEngines()
{
    KRun::runCommand("kcmshell4 ebrowsing", (m_part ? m_part.data()->widget() : 0));
}

void SearchBarPlugin::configurationChanged()
{
    delete m_popupMenu;
    m_popupMenu = 0;
    m_addSearchActions.clear();
    m_searchEngines.clear();
    m_searchProviders.clear();

    KUriFilterData data;
    data.setSearchFilteringOptions(KUriFilterData::RetrievePreferredSearchProvidersOnly);
    data.setAlternateDefaultSearchProvider(QLatin1String("google"));

    if (KUriFilter::self()->filterSearchUri(data, KUriFilter::NormalTextFilter)) {
        m_delimiter = data.searchTermSeparator();
        Q_FOREACH(const QString& engine, data.preferredSearchProviders()) {
            //kDebug() << "Found search provider:" << engine;
            const KUriFilterSearchProvider& provider = data.queryForSearchProvider(engine);
            m_searchProviders.insert(provider.desktopEntryName(), provider);
            m_searchEngines << provider.desktopEntryName();
        }
    }

    //kDebug() << "Found search engines:" << m_searchEngines;
    KConfigGroup config = KConfigGroup(KGlobal::config(), "SearchBar");
    m_searchMode = (SearchModes) config.readEntry("Mode", static_cast<int>(UseSearchProvider));
    const QString defaultSearchEngine ((m_searchEngines.isEmpty() ?  QString::fromLatin1("google") : m_searchEngines.first()));
    m_currentEngine = config.readEntry("CurrentEngine", defaultSearchEngine);
    m_suggestionEnabled = config.readEntry("SuggestionEnabled", true);

    m_searchCombo->setSuggestionEnabled(m_suggestionEnabled);
    m_openSearchManager->setSearchProvider(m_currentEngine);

    m_reloadConfiguration = false;
    setIcon();
}

void SearchBarPlugin::reloadConfiguration()
{
    // NOTE: We do not directly connect the dbus signal to the configurationChanged
    // slot because our slot my be called before the filter plugins, in which case we
    // simply end up retrieving the same configuration information from the plugin.
    m_reloadConfiguration = true;
}

void SearchBarPlugin::updateComboVisibility()
{
    if (!m_part)
        return;
    // NOTE: We hide the search combobox if the embedded kpart is ReadWrite
    // because web browsers by their very nature are ReadOnly kparts...
    m_searchComboAction->setVisible((!m_part.data()->inherits("ReadWritePart") &&
                                     !m_searchComboAction->associatedWidgets().isEmpty()));
    m_openSearchDescs.clear();
}

void SearchBarPlugin::focusSearchbar()
{
    m_searchCombo->setFocus(Qt::ShortcutFocusReason);
}

void SearchBarPlugin::searchTextChanged(const QString &text)
{
    // Don't do anything if the user just activated the search for this text
    // Popping up suggestions again would just lead to an annoying popup (#231213)
    if (m_lastSearch == text) {
        return;
    }

    // Don't do anything if the user is still pressing on the mouse button
    if (qApp->mouseButtons()) {
        return;
    }

    // 400 ms delay before requesting for suggestions, so we don't flood the provider with suggestion request
    m_timer->start(400);
}

void SearchBarPlugin::requestSuggestion() {
    m_searchCombo->clearSuggestions();

    if (m_suggestionEnabled && m_searchMode != FindInThisPage &&
        m_openSearchManager->isSuggestionAvailable() &&
        !m_searchCombo->lineEdit()->text().isEmpty()) {
        m_openSearchManager->requestSuggestion(m_searchCombo->lineEdit()->text());
    }
}

void SearchBarPlugin::enableSuggestion(bool enable)
{
    m_suggestionEnabled = enable;
}

void SearchBarPlugin::HTMLDocLoaded()
{
    if (!m_part || m_part.data()->url().host().isEmpty())
        return;
    
    // Testcase for this code: http://search.iwsearch.net
    KParts::HtmlExtension* ext = KParts::HtmlExtension::childObject(m_part.data());
    KParts::SelectorInterface* selectorInterface = qobject_cast<KParts::SelectorInterface*>(ext);

    if (selectorInterface) {
        //if (headElelement.getAttribute("profile") != "http://a9.com/-/spec/opensearch/1.1/") {
        //    kWarning() << "Warning: there is no profile attribute or wrong profile attribute in <head>, as specified by open search specification 1.1";
        //}
        const QString query (QLatin1String("head > link[rel=\"search\"][type=\"application/opensearchdescription+xml\"]"));
        const QList<KParts::SelectorInterface::Element> linkNodes = selectorInterface->querySelectorAll(query, KParts::SelectorInterface::EntireContent);
        //kDebug() << "Found" << linkNodes.length() << "links in" << m_part->url();
        Q_FOREACH(const KParts::SelectorInterface::Element& link, linkNodes) {
            const QString title = link.attribute("title");
            const QString href = link.attribute("href");
            //kDebug() << "Found opensearch" << title << href;
            m_openSearchDescs.insert(title, href);
            // TODO associate this with m_part; we can get descs from multiple tabs here...
        }
    }
}

void SearchBarPlugin::openSearchEngineAdded(const QString &name, const QString &searchUrl, const QString &fileName)
{
    //kDebug() << "New Open Search Engine Added: " << name << ", searchUrl " << searchUrl;
    QString path = KGlobal::mainComponent().dirs()->saveLocation("services", "searchproviders/");

    KConfig _service(path + fileName + ".desktop", KConfig::SimpleConfig);
    KConfigGroup service(&_service, "Desktop Entry");
    service.writeEntry("Type", "Service");
    service.writeEntry("ServiceTypes", "SearchProvider");
    service.writeEntry("Name", name);
    service.writeEntry("Query", searchUrl);
    service.writeEntry("Keys", fileName);
    // TODO
    service.writeEntry("Charset", "" /* provider->charset() */);

    // we might be overwriting a hidden entry
    service.writeEntry("Hidden", false);

    // Show the add web shortcut widget
    if (!m_addWSWidget) {
        m_addWSWidget = new WebShortcutWidget(m_searchCombo);
        m_addWSWidget->setWindowFlags(Qt::Popup);

        connect(m_addWSWidget, SIGNAL(webShortcutSet(QString,QString,QString)),
                this, SLOT(webShortcutSet(QString,QString,QString)));
    }

    QPoint pos = m_searchCombo->mapToGlobal(QPoint(m_searchCombo->width()-m_addWSWidget->width(), m_searchCombo->height() + 1));
    m_addWSWidget->setGeometry(QRect(pos, m_addWSWidget->size()));
    m_addWSWidget->show(name, fileName);
}

void SearchBarPlugin::webShortcutSet(const QString &name, const QString &webShortcut, const QString &fileName)
{
    Q_UNUSED(name);
    QString path = KGlobal::mainComponent().dirs()->saveLocation("services", "searchproviders/");
    KConfig _service(path + fileName + ".desktop", KConfig::SimpleConfig);
    KConfigGroup service(&_service, "Desktop Entry");
    service.writeEntry("Keys", webShortcut);
    _service.sync();
    
    // Update filters in running applications including ourselves...
    QDBusConnection::sessionBus().send(QDBusMessage::createSignal("/", "org.kde.KUriFilterPlugin", "configure"));

    // If the providers changed, tell sycoca to rebuild its database...
    KBuildSycocaProgressDialog::rebuildKSycoca(m_searchCombo);
}

void SearchBarPlugin::HTMLLoadingStarted()
{
    // reset the open search availability, so that if there is previously detected engine,
    // it will not be shown
    m_openSearchDescs.clear();
}

void SearchBarPlugin::addSearchSuggestion(const QStringList &suggestions)
{
    m_searchCombo->setSuggestionItems(suggestions);
}

SearchBarCombo::SearchBarCombo(QWidget *parent)
    :KHistoryComboBox(true, parent)
{
    setDuplicatesEnabled(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMaximumWidth(300);
    connect(this, SIGNAL(cleared()), SLOT(historyCleared()));

    Q_ASSERT(useCompletion());

    KConfigGroup config(KGlobal::config(), "SearchBar");
    const int defaultMode = KGlobalSettings::completionMode();
    setCompletionMode (static_cast<KGlobalSettings::Completion>(config.readEntry("CompletionMode", defaultMode)));
    const QStringList list = config.readEntry( "History list", QStringList() );
    setHistoryItems(list, true);
    Q_ASSERT(currentText().isEmpty()); // KHistoryComboBox calls clearEditText

    m_enableAction = new QAction(i18n("Enable Suggestion"), this);
    m_enableAction->setCheckable(true);
    connect(m_enableAction, SIGNAL(toggled(bool)), this, SIGNAL(suggestionEnabled(bool)));

    connect(this, SIGNAL(aboutToShowContextMenu(QMenu*)), SLOT(addEnableMenuItem(QMenu*)));

    // use our own item delegate to display our fancy stuff :D
    KCompletionBox* box = completionBox();
    box->setItemDelegate(new SearchBarItemDelegate(this));
    connect(lineEdit(), SIGNAL(textEdited(QString)), box, SLOT(setCancelledText(QString)));
}

SearchBarCombo::~SearchBarCombo()
{
    KConfigGroup config(KGlobal::config(), "SearchBar");
    config.writeEntry( "History list", historyItems() );
    const int mode = completionMode();
    config.writeEntry( "CompletionMode", mode);
    delete m_enableAction;
}

const QPixmap &SearchBarCombo::icon() const
{
    return m_icon;
}

void SearchBarCombo::setIcon(const QPixmap &icon)
{
    m_icon = icon;
    const QString editText = currentText();
    if (count() == 0) {
        insertItem(0, m_icon, 0);
    } else {
        for(int i = 0; i < count(); i++) {
            setItemIcon(i, m_icon);
        }
    }
    setEditText(editText);
}

void SearchBarCombo::setSuggestionEnabled(bool enable)
{
    m_enableAction->setChecked(enable);
}

int SearchBarCombo::findHistoryItem(const QString &searchText)
{
    for(int i = 0; i < count(); i++) {
        if (itemText(i) == searchText) {
            return i;
        }
    }

    return -1;
}

void SearchBarCombo::mousePressEvent(QMouseEvent *e)
{
    QStyleOptionComplex opt;
    int x0 = QStyle::visualRect(layoutDirection(), style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this), rect()).x();

    if (e->x() > x0 + 2 && e->x() < lineEdit()->x()) {
        emit iconClicked();
        e->accept();
    } else {
        KHistoryComboBox::mousePressEvent(e);
    }
}

void SearchBarCombo::historyCleared()
{
    setIcon(m_icon);
}

void SearchBarCombo::setSuggestionItems(const QStringList &suggestions)
{
    if (!m_suggestions.isEmpty()) {
        clearSuggestions();
    }

    m_suggestions = suggestions;
    if (!suggestions.isEmpty()) {
        const int size = completionBox()->count();
        QListWidgetItem *item = new QListWidgetItem(suggestions.at(0));
        item->setData(Qt::UserRole, "suggestion");
        completionBox()->insertItem(size + 1, item);
        const int suggestionCount = suggestions.count();
        for (int i = 1; i < suggestionCount; i++) {
            completionBox()->insertItem(size + 1 + i, suggestions.at(i));
        }
        completionBox()->popup();
    }
}

void SearchBarCombo::clearSuggestions()
{
    // Removing items can change the current item in completion box,
    // which makes the lineEdit emit textEdited, and we would then
    // re-enter this method, so block lineEdit signals.
    const bool oldBlock = lineEdit()->blockSignals(true);
    int size = completionBox()->count();
    if (!m_suggestions.isEmpty() && size >= m_suggestions.count()) {
        for (int i = size - 1; i >= size - m_suggestions.size(); i--) {
            completionBox()->takeItem(i);
        }
    }
    m_suggestions.clear();
    lineEdit()->blockSignals(oldBlock);
}


void SearchBarCombo::addEnableMenuItem(QMenu *menu)
{
    if (menu) {
        menu->addAction(m_enableAction);
    }
}

SearchBarItemDelegate::SearchBarItemDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

void SearchBarItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString userText = index.data(Qt::UserRole).toString();
    QString text = index.data(Qt::DisplayRole).toString();

    // Get item data
    if (!userText.isEmpty()) {
        // This font is for the "information" text, small size + italic + gray in color
        QFont usrTxtFont = option.font;
        usrTxtFont.setItalic(true);
        usrTxtFont.setPointSize(6);

        QFontMetrics usrTxtFontMetrics(usrTxtFont);
        int width = usrTxtFontMetrics.width(userText);
        QRect rect(option.rect.x(), option.rect.y(), option.rect.width() - width, option.rect.height());
        QFontMetrics textFontMetrics(option.font);
        QString elidedText = textFontMetrics.elidedText(text,
                Qt::ElideRight, option.rect.width() - width - option.decorationSize.width());

        QAbstractItemModel *itemModel = const_cast<QAbstractItemModel *>(index.model());
        itemModel->setData(index, elidedText, Qt::DisplayRole);
        QItemDelegate::paint(painter, option, index);
        itemModel->setData(index, text, Qt::DisplayRole);

        painter->setFont(usrTxtFont);
        painter->setPen(QPen(QColor(Qt::gray)));
        painter->drawText(option.rect, Qt::AlignRight, userText);

        // Draw a separator above this item
        if (index.row() > 0) {
            painter->drawLine(option.rect.x(), option.rect.y(), option.rect.x() + option.rect.width(), option.rect.y());
        }
    }
    else {
        QItemDelegate::paint(painter, option, index);
    }
}

bool SearchBarPlugin::enableFindInPage() const
{
    return true;
}

#include "searchbar.moc"
