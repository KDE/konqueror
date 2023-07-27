/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2004 Arend van Beelen jr. <arend@auton.nl>
    SPDX-FileCopyrightText: 2009 Fredy Yanardi <fyanardi@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "searchbar.h"

#include "WebShortcutWidget.h"

#include <KBuildSycocaProgressDialog>
#include <KCompletionBox>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KDesktopFile>
#include <KDialogJobUiDelegate>
#include <KPluginFactory>
#include <KActionCollection>
#include <KIO/CommandLauncherJob>
#include <KMainWindow>
#include <KParts/Part>
#include <KParts/BrowserExtension>
#include <KParts/SelectorInterface>
#include <KParts/PartActivateEvent>
#include <KLocalizedString>
#include <KIO/Job>

#include <QLineEdit>
#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QMenu>
#include <QStyle>
#include <QPainter>
#include <QMouseEvent>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QWidgetAction>
#include <QStandardPaths>

#include "searchbar_debug.h"
#include <asyncselectorinterface.h>
#include <htmlextension.h>
#include <textextension.h>

K_PLUGIN_CLASS_WITH_JSON(SearchBarPlugin, "searchbar.json")

SearchBarPlugin::SearchBarPlugin(QObject *parent,
                                 const QVariantList &) :
    KonqParts::Plugin(parent),
    m_popupMenu(nullptr),
    m_addWSWidget(nullptr),
    m_searchMode(UseSearchProvider),
    m_urlEnterLock(false),
    m_reloadConfiguration(false)
{
    m_searchCombo = new SearchBarCombo(nullptr);
    m_searchCombo->lineEdit()->installEventFilter(this);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    connect(m_searchCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int n){startSearch(m_searchCombo->itemText(n));});
#else
    connect(m_searchCombo, &QComboBox::textActivated, this, &SearchBarPlugin::startSearch);
#endif
    connect(m_searchCombo, &SearchBarCombo::iconClicked, this, &SearchBarPlugin::showSelectionMenu);
    m_searchCombo->setWhatsThis(i18n("Search Bar<p>"
                                     "Enter a search term. Click on the icon to change search mode or provider.</p>"));

    m_searchComboAction = new QWidgetAction(actionCollection());
    actionCollection()->addAction(QStringLiteral("toolbar_search_bar"), m_searchComboAction);
    m_searchComboAction->setText(i18n("Search Bar"));
    m_searchComboAction->setDefaultWidget(m_searchCombo);
    actionCollection()->setShortcutsConfigurable(m_searchComboAction, false);

    QAction *a = actionCollection()->addAction(QStringLiteral("focus_search_bar"));
    a->setText(i18n("Focus Searchbar"));
    actionCollection()->setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
    connect(a, &QAction::triggered, this, &SearchBarPlugin::focusSearchbar);
    m_searchProvidersDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kde5/services/searchproviders/";
    QDir().mkpath(m_searchProvidersDir);
    configurationChanged();

    // parent is the KonqMainWindow and we want to listen to PartActivateEvent events.
    parent->installEventFilter(this);

    connect(m_searchCombo->lineEdit(), &QLineEdit::textEdited,
            this, &SearchBarPlugin::searchTextChanged);

    QDBusConnection::sessionBus().connect(QString(), QString(), QStringLiteral("org.kde.KUriFilterPlugin"),
                                          QStringLiteral("configure"), this, SLOT(reloadConfiguration()));
}

SearchBarPlugin::~SearchBarPlugin()
{
    KConfigGroup config(KSharedConfig::openConfig(), "SearchBar");
    config.writeEntry("Mode", (int) m_searchMode);
    config.writeEntry("CurrentEngine", m_currentEngine);

    delete m_searchCombo;
    m_searchCombo = nullptr;
}

bool SearchBarPlugin::eventFilter(QObject *o, QEvent *e)
{
    if (KParts::PartActivateEvent::test(e)) {
        KParts::PartActivateEvent *partEvent = static_cast<KParts::PartActivateEvent *>(e);
        KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(partEvent->part());
        //qCDebug(SEARCHBAR_LOG) << "Embedded part changed to " << part;
        if (part && (m_part.isNull() || part != m_part)) {
            m_part = part;

            // Delete the popup menu so a new one can be created with the
            // appropriate entries the next time it is shown...
            // ######## TODO: This loses the opensearch entries for the old part!!!
            if (m_popupMenu) {
                delete m_popupMenu;
                m_popupMenu = nullptr;
                m_addSearchActions.clear(); // the actions had the menu as parent, so they're deleted now
            }

            // Change the search mode if it is set to FindInThisPage since
            // that feature is currently KHTML specific. It is also completely
            // redundant and unnecessary.
            if (m_searchMode == FindInThisPage && enableFindInPage()) {
                nextSearchEntry();
            }

            connect(part, QOverload<>::of(&KParts::ReadOnlyPart::completed), this, &SearchBarPlugin::HTMLDocLoaded);
            connect(part, &KParts::ReadOnlyPart::started, this, &SearchBarPlugin::HTMLLoadingStarted);
        }
        // Delay since when destroying tabs part 0 gets activated for a bit, before the proper part
        QTimer::singleShot(0, this, &SearchBarPlugin::updateComboVisibility);
    } else if (o == m_searchCombo->lineEdit() && e->type() == QEvent::KeyPress) {
        QKeyEvent *k = (QKeyEvent *)e;
        if (k->modifiers() & Qt::ControlModifier) {
            if (k->key() == Qt::Key_Down) {
                nextSearchEntry();
                return true;
            }
            if (k->key() == Qt::Key_Up) {
                previousSearchEntry();
                return true;
            }
        }
    }
    return KonqParts::Plugin::eventFilter(o, e);
}

void SearchBarPlugin::nextSearchEntry()
{
    if (m_searchMode == FindInThisPage) {
        m_searchMode = UseSearchProvider;
        if (m_searchEngines.isEmpty()) {
            m_currentEngine = QStringLiteral("google");
        } else {
            m_currentEngine = m_searchEngines.first();
        }
    } else {
        const int index = m_searchEngines.indexOf(m_currentEngine) + 1;
        if (index >= m_searchEngines.count()) {
            m_searchMode = FindInThisPage;
        } else {
            m_currentEngine = m_searchEngines.at(index);
        }
    }
    setIcon();
}

void SearchBarPlugin::previousSearchEntry()
{
    if (m_searchMode == FindInThisPage) {
        m_searchMode = UseSearchProvider;
        if (m_searchEngines.isEmpty()) {
            m_currentEngine = QStringLiteral("google");
        } else {
            m_currentEngine =  m_searchEngines.last();
        }
    } else {
        const int index = m_searchEngines.indexOf(m_currentEngine) - 1;
        if (index <= 0) {
            m_searchMode = FindInThisPage;
        } else {
            m_currentEngine = m_searchEngines.at(index);
        }
    }
    setIcon();
}

// Called when activating the combobox (Key_Return, or item in popup or in completionbox)
void SearchBarPlugin::startSearch(const QString &search)
{
    if (m_urlEnterLock || search.isEmpty() || m_part.isNull()) {
        return;
    }
    m_lastSearch = search;

    if (m_searchMode == FindInThisPage) {
        TextExtension *textExt = TextExtension::childObject(m_part);
        if (textExt) {
            textExt->findText(search, KFind::SearchOptions());
        }
    } else if (m_searchMode == UseSearchProvider) {
        m_urlEnterLock = true;
        const KUriFilterSearchProvider &provider = m_searchProviders.value(m_currentEngine);
        KUriFilterData data;
        data.setData(provider.defaultKey() + m_delimiter + search);
        //qCDebug(SEARCHBAR_LOG) << "Query:" << (provider.defaultKey() + m_delimiter + search);
        if (!KUriFilter::self()->filterSearchUri(data, KUriFilter::WebShortcutFilter)) {
            qCWarning(SEARCHBAR_LOG) << "Failed to filter using web shortcut:" << provider.defaultKey();
            return;
        }

        KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(m_part);
        if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
            KParts::OpenUrlArguments arguments;
            KParts::BrowserArguments browserArguments;
            browserArguments.setNewTab(true);
            if (ext) {
                emit ext->createNewWindow(data.uri(), arguments, browserArguments);
            }
        } else {
            if (ext) {
                emit ext->openUrlRequest(data.uri());
                if (!m_part.isNull()) {
                    m_part->widget()->setFocus();    // #152923
                }
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
        m_searchIcon = QIcon::fromTheme(QStringLiteral("edit-find")).pixmap(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize));
    } else {
        const QString engine = (m_currentEngine.isEmpty() ? m_searchEngines.first() : m_currentEngine);
        //qCDebug(SEARCHBAR_LOG) << "Icon Name:" << m_searchProviders.value(engine).iconName();
        const QString iconName = m_searchProviders.value(engine).iconName();
        if (iconName.startsWith(QLatin1Char('/'))) {
            m_searchIcon = QPixmap(iconName);
        } else {
            m_searchIcon = QIcon::fromTheme(iconName).pixmap(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize));
        }
    }

    // Create a bit wider icon with arrow
    QPixmap arrowmap = QPixmap(m_searchIcon.width() + 5, m_searchIcon.height() + 5);
    arrowmap.fill(m_searchCombo->lineEdit()->palette().color(m_searchCombo->lineEdit()->backgroundRole()));
    QPainter p(&arrowmap);
    p.drawPixmap(0, 2, m_searchIcon);
    QStyleOption opt;
    opt.state = QStyle::State_None;
    opt.rect = QRect(arrowmap.width() - 6, arrowmap.height() - 5, 6, 5);
    m_searchCombo->style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &opt, &p, m_searchCombo);
    p.end();
    m_searchIcon = arrowmap;
    m_searchCombo->setIcon(m_searchIcon);

    // Set the placeholder text to be the search engine name...

    if (m_searchMode == FindInThisPage) {
        m_searchCombo->lineEdit()->setPlaceholderText(i18n("Find in Page..."));
    } else {
        if (m_searchProviders.contains(m_currentEngine)) {
            m_searchCombo->lineEdit()->setPlaceholderText(m_searchProviders.value(m_currentEngine).name());
        }
    }
}

void SearchBarPlugin::showSelectionMenu()
{
    // Update the configuration, if needed, before showing the menu items...
    if (m_reloadConfiguration) {
        configurationChanged();
    }

    if (!m_popupMenu) {
        m_popupMenu = new QMenu(m_searchCombo);
        m_popupMenu->setObjectName(QStringLiteral("search selection menu"));

        if (enableFindInPage()) {
            m_popupMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-find")), i18n("Find in This Page"),
                                   this, &SearchBarPlugin::useFindInThisPage);
            m_popupMenu->addSeparator();
        }

        for (int i = 0, count = m_searchEngines.count(); i != count; ++i) {
            const KUriFilterSearchProvider &provider = m_searchProviders.value(m_searchEngines.at(i));
            QAction *action = m_popupMenu->addAction(QIcon::fromTheme(provider.iconName()), provider.name());
            action->setData(QVariant::fromValue(i));
        }

        m_popupMenu->addSeparator();
        m_popupMenu->addAction(QIcon::fromTheme(QStringLiteral("preferences-web-browser-shortcuts")), i18n("Select Search Engines..."),
                               this, &SearchBarPlugin::selectSearchEngines);
        connect(m_popupMenu, &QMenu::triggered, this, &SearchBarPlugin::menuActionTriggered);
    } else {
        Q_FOREACH (QAction *action, m_addSearchActions) {
            m_popupMenu->removeAction(action);
            delete action;
        }
        m_addSearchActions.clear();
    }

    QList<QAction *> actions = m_popupMenu->actions();
    QAction *before = nullptr;
    if (actions.size() > 1) {
        before = actions[actions.size() - 2];
    }

    Q_FOREACH (const QString &title, m_openSearchDescs.keys()) {
        QAction *addSearchAction = new QAction(m_popupMenu);
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
        m_searchCombo->lineEdit()->selectAll();
        return;
    }

    m_searchCombo->lineEdit()->setPlaceholderText(QString());
    const QString openSearchTitle = action->data().toString();
    if (!openSearchTitle.isEmpty()) {
        const QString openSearchHref = m_openSearchDescs.value(openSearchTitle);
        QUrl url;
        QUrl openSearchUrl = QUrl(openSearchHref);
        if (openSearchUrl.isRelative()) {
            const QUrl docUrl = !m_part.isNull() ? m_part->url() : QUrl();
            QString host = docUrl.scheme() + QLatin1String("://") + docUrl.host();
            if (docUrl.port() != -1) {
                host += QLatin1String(":") + QString::number(docUrl.port());
            }
            url = docUrl.resolved(QUrl(openSearchHref));
        } else {
            url = QUrl(openSearchHref);
        }
    }
}

void SearchBarPlugin::selectSearchEngines()
{
    KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(QStringLiteral("kcmshell5 webshortcuts"));
    job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, !m_part.isNull() ? m_part->widget() : nullptr));
    job->start();
}

void SearchBarPlugin::configurationChanged()
{
    delete m_popupMenu;
    m_popupMenu = nullptr;
    m_addSearchActions.clear();
    m_searchEngines.clear();
    m_searchProviders.clear();

    KUriFilterData data;
    data.setSearchFilteringOptions(KUriFilterData::RetrievePreferredSearchProvidersOnly);
    data.setAlternateDefaultSearchProvider(QStringLiteral("google"));

    if (KUriFilter::self()->filterSearchUri(data, KUriFilter::NormalTextFilter)) {
        m_delimiter = data.searchTermSeparator();
        Q_FOREACH (const QString &engine, data.preferredSearchProviders()) {
            //qCDebug(SEARCHBAR_LOG) << "Found search provider:" << engine;
            const KUriFilterSearchProvider &provider = data.queryForSearchProvider(engine);

            m_searchProviders.insert(provider.desktopEntryName(), provider);
            m_searchEngines << provider.desktopEntryName();
        }
    }

    //qCDebug(SEARCHBAR_LOG) << "Found search engines:" << m_searchEngines;
    KConfigGroup config = KConfigGroup(KSharedConfig::openConfig(), "SearchBar");
    m_searchMode = (SearchModes) config.readEntry("Mode", static_cast<int>(UseSearchProvider));
    const QString defaultSearchEngine((m_searchEngines.isEmpty() ?  QStringLiteral("google") : m_searchEngines.first()));
    m_currentEngine = config.readEntry("CurrentEngine", defaultSearchEngine);

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
    if (m_part.isNull()) {
        return;
    }
    // NOTE: We hide the search combobox if the embedded kpart is ReadWrite
    // because web browsers by their very nature are ReadOnly kparts...
    m_searchComboAction->setVisible(!m_part->inherits("ReadWritePart") &&
                                    !m_searchComboAction->associatedWidgets().isEmpty());
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
}

void SearchBarPlugin::HTMLDocLoaded()
{
    if (m_part.isNull() || m_part->url().host().isEmpty()) {
        return;
    }

    //NOTE: the link below seems to be dead
    // Testcase for this code: http://search.iwsearch.net
    HtmlExtension *ext = HtmlExtension::childObject(m_part);
    KParts::SelectorInterface *selectorInterface = qobject_cast<KParts::SelectorInterface *>(ext);
    AsyncSelectorInterface *asyncIface = qobject_cast<AsyncSelectorInterface*>(ext);
    const QString query(QStringLiteral("head > link[rel=\"search\"][type=\"application/opensearchdescription+xml\"]"));

    if (selectorInterface) {
        //if (headElelement.getAttribute("profile") != "http://a9.com/-/spec/opensearch/1.1/") {
        //    kWarning() << "Warning: there is no profile attribute or wrong profile attribute in <head>, as specified by open search specification 1.1";
        //}
        const QList<KParts::SelectorInterface::Element> linkNodes = selectorInterface->querySelectorAll(query, KParts::SelectorInterface::EntireContent);
        insertOpenSearchEntries(linkNodes);
    } else if (asyncIface) {
        auto callback = [this](const QList<KParts::SelectorInterface::Element>& elements) {
            insertOpenSearchEntries(elements);
        };
        asyncIface->querySelectorAllAsync(query, KParts::SelectorInterface::EntireContent, callback);
    }
}

void SearchBarPlugin::insertOpenSearchEntries(const QList<KParts::SelectorInterface::Element>& elements)
{
    for (const KParts::SelectorInterface::Element &link : elements) {
        const QString title = link.attribute(QStringLiteral("title"));
        const QString href = link.attribute(QStringLiteral("href"));
        //qCDebug(SEARCHBAR_LOG) << "Found opensearch" << title << href;
        m_openSearchDescs.insert(title, href);
        // TODO associate this with m_part; we can get descs from multiple tabs here...
    }
}

void SearchBarPlugin::openSearchEngineAdded(const QString &name, const QString &searchUrl, const QString &fileName)
{
    //qCDebug(SEARCHBAR_LOG) << "New Open Search Engine Added: " << name << ", searchUrl " << searchUrl;

    KConfig _service(m_searchProvidersDir + fileName + ".desktop", KConfig::SimpleConfig);
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

        connect(m_addWSWidget, &WebShortcutWidget::webShortcutSet, this, &SearchBarPlugin::webShortcutSet);
    }

    QPoint pos = m_searchCombo->mapToGlobal(QPoint(m_searchCombo->width() - m_addWSWidget->width(), m_searchCombo->height() + 1));
    m_addWSWidget->setGeometry(QRect(pos, m_addWSWidget->size()));
    m_addWSWidget->show(name, fileName);
}

void SearchBarPlugin::webShortcutSet(const QString &name, const QString &webShortcut, const QString &fileName)
{
    Q_UNUSED(name);
    KConfig _service(m_searchProvidersDir + fileName + ".desktop", KConfig::SimpleConfig);
    KConfigGroup service(&_service, "Desktop Entry");
    service.writeEntry("Keys", webShortcut);
    _service.sync();

    // Update filters in running applications including ourselves...
    QDBusConnection::sessionBus().send(QDBusMessage::createSignal(QStringLiteral("/"), QStringLiteral("org.kde.KUriFilterPlugin"), QStringLiteral("configure")));

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
    : KHistoryComboBox(true, parent)
{
    setDuplicatesEnabled(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMaximumWidth(300);
    connect(this, &KHistoryComboBox::cleared, this, &SearchBarCombo::historyCleared);

    Q_ASSERT(useCompletion());

    KConfigGroup config(KSharedConfig::openConfig(), "SearchBar");
    setCompletionMode(static_cast<KCompletion::CompletionMode>(config.readEntry("CompletionMode", static_cast<int>(KCompletion::CompletionPopup))));
    const QStringList list = config.readEntry("History list", QStringList());
    setHistoryItems(list, true);
    Q_ASSERT(currentText().isEmpty()); // KHistoryComboBox calls clearEditText

    // use our own item delegate to display our fancy stuff :D
    KCompletionBox *box = completionBox();
    box->setItemDelegate(new SearchBarItemDelegate(this));
    connect(lineEdit(), &QLineEdit::textEdited, box, &KCompletionBox::setCancelledText);
}

SearchBarCombo::~SearchBarCombo()
{
    KConfigGroup config(KSharedConfig::openConfig(), "SearchBar");
    config.writeEntry("History list", historyItems());
    const int mode = completionMode();
    config.writeEntry("CompletionMode", mode);
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
        insertItem(0, m_icon, nullptr);
    } else {
        for (int i = 0; i < count(); i++) {
            setItemIcon(i, m_icon);
        }
    }
    setEditText(editText);
}

int SearchBarCombo::findHistoryItem(const QString &searchText)
{
    for (int i = 0; i < count(); i++) {
        if (itemText(i) == searchText) {
            return i;
        }
    }

    return -1;
}

bool SearchBarCombo::overIcon(int x)
{
    QStyleOptionComplex opt;
    const int x0 = QStyle::visualRect(layoutDirection(), style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxEditField, this), rect()).x();
    return (x > x0 + 2 && x < lineEdit()->x());
}

void SearchBarCombo::contextMenuEvent(QContextMenuEvent *e)
{
    if (overIcon(e->x())) {
        // Do not pass on the event, so that the combo box context menu
        // does not pop up over the search engine selection menu.  That
        // menu will have been triggered by the iconClicked() signal emitted
        // in mousePressEvent() below.
        e->accept();
    } else {
        KHistoryComboBox::contextMenuEvent(e);
    }
}

void SearchBarCombo::mousePressEvent(QMouseEvent *e)
{
    if (overIcon(e->x())) {
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
        int width = usrTxtFontMetrics.horizontalAdvance(userText);
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
    } else {
        QItemDelegate::paint(painter, option, index);
    }
}

bool SearchBarPlugin::enableFindInPage() const
{
    return true;
}

#include "searchbar.moc"
