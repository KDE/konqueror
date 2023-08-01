/*
    SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "sidebar_widget.h"
#include "konqmultitabbar.h"
#include "sidebar_debug.h"

// std
#include <limits.h>

// Qt
#include <QDir>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <QInputDialog>
#include <QIcon>
#include <QHBoxLayout>
#include <QStandardPaths>

// KDE
#include <KLocalizedString>
#include <kconfig.h>

#include <kdesktopfile.h>
#include <kicondialog.h>
#include <kmessagebox.h>
#include <konq_events.h>
#include <kfileitem.h>
#include <kurlrequesterdialog.h>
#include <KUrlRequester>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KPluginFactory>

void Sidebar_Widget::aboutToShowAddMenu()
{
    m_addMenu->clear();
    m_pluginForAction.clear();

    QList<KConfigGroup> existingGroups;
    // Collect the "already shown" modules
    for (int i = 0; i < m_buttons.count(); ++i) {
        existingGroups.append(m_buttons[i].configFile->group("Desktop Entry"));
    }

//TODO KF6: remove version check and replace with code using json
    // We need to instantiate all available plugins
    // And since the web module isn't in the default entries at all, we can't just collect
    // the plugins there.
    const QVector<KPluginMetaData> plugins = m_moduleManager.availablePlugins();
    for (const KPluginMetaData &service : plugins) {
        if (!service.isValid()) {
            continue;
        }
        auto pluginResult = KPluginFactory::instantiatePlugin<KonqSidebarPlugin>(service, this);
        if (pluginResult) {
          KonqSidebarPlugin *plugin = pluginResult.plugin;
          const QList<QAction *> actions = plugin->addNewActions(&m_addMenuActionGroup,
                                                                 existingGroups,
                                                                 QVariant());
          // Remember which plugin the action came from.
          // We can't use QAction::setData for that, because we let plugins use
          // that already.
          Q_FOREACH (QAction *action, actions) {
            m_pluginForAction.insert(action, plugin);
          }
          m_addMenu->addActions(actions);
        } else {
          qCWarning(SIDEBAR_LOG) << "Error loading plugin" << pluginResult.errorText;
        }
    }
    m_addMenu->addSeparator();
    m_addMenu->addAction(KStandardGuiItem::defaults().icon(), i18n("Restore All Removed Default Buttons"), this, &Sidebar_Widget::slotRestoreDeletedButtons);
    m_addMenu->addAction(KStandardGuiItem::defaults().icon(), i18n("Rollback to System Default"), this, &Sidebar_Widget::slotRollback);
}

void Sidebar_Widget::triggeredAddMenu(QAction *action)
{
    KonqSidebarPlugin *plugin = m_pluginForAction.value(action);
    m_pluginForAction.clear(); // save memory

    QString templ = plugin->templateNameForNewModule(action->data(), QVariant());
    Q_ASSERT(!templ.contains('/'));
    if (templ.isEmpty()) {
        return;
    }
    const QString myFile = m_moduleManager.addModuleFromTemplate(templ);
    if (myFile.isEmpty()) {
        return;
    }

    qCDebug(SIDEBAR_LOG) << myFile << "filename=" << templ;
    KDesktopFile df(myFile);
    KConfigGroup configGroup = df.desktopGroup();
    configGroup.writeEntry("X-KDE-Weight", m_moduleManager.getNextAvailableKDEWeight());
    const bool ok = plugin->createNewModule(action->data(), configGroup, this, QVariant());
    df.sync();
    if (ok) {
        m_moduleManager.moduleAdded(templ /*contains the final filename*/);
        // TODO only add the new button
        QTimer::singleShot(0, this, &Sidebar_Widget::updateButtons);
    } else {
        QFile::remove(myFile);
    }
}

Sidebar_Widget::Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par, const QString &currentProfile)
    : QWidget(parent),
      m_partParent(par),
      m_addMenuActionGroup(this),
      m_config(new KConfigGroup(KSharedConfig::openConfig("konqsidebartngrc"),
                                currentProfile)),
      m_moduleManager(m_config)
{
    m_somethingVisible = false;
    m_noUpdate = false;
    m_layout = nullptr;
    m_currentButtonIndex = -1;
    m_activeModule = nullptr;
    //m_userMovedSplitter = false;
    m_latestViewed = -1;
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_area = new QSplitter(Qt::Vertical, this);
    m_area->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    m_area->setMinimumWidth(0);

    m_buttonBar = new KonqMultiTabBar(this);
    connect(m_buttonBar, &KonqMultiTabBar::urlsDropped, this, &Sidebar_Widget::slotUrlsDropped);

    m_menu = new QMenu(this);
    m_menu->setIcon(QIcon::fromTheme("configure"));
    m_menu->setTitle(i18n("Configure Sidebar"));

    m_addMenu = m_menu->addMenu(KStandardGuiItem::add().icon(), i18n("Add New"));
    connect(m_addMenu, &QMenu::aboutToShow, this, &Sidebar_Widget::aboutToShowAddMenu);
    connect(&m_addMenuActionGroup, &QActionGroup::triggered, this, &Sidebar_Widget::triggeredAddMenu);
    m_menu->addSeparator();
    m_multiViews = m_menu->addAction(i18n("Multiple Views"), this, &Sidebar_Widget::slotMultipleViews);
    m_multiViews->setCheckable(true);
    m_showTabLeft = m_menu->addAction(i18n("Show Tabs on Left"), this, &Sidebar_Widget::slotShowTabsLeft);
    m_showConfigButton = m_menu->addAction(i18n("Show Configuration Button"), this, &Sidebar_Widget::slotShowConfigurationButton);
    m_showConfigButton->setCheckable(true);
    m_menu->addSeparator();
    m_menu->addAction(KStandardGuiItem::close().icon(), i18n("Close Sidebar"), par, &QObject::deleteLater);

    connect(m_menu, &QMenu::aboutToShow, this, &Sidebar_Widget::aboutToShowConfigMenu);

    m_configTimer.setSingleShot(true);
    connect(&m_configTimer, &QTimer::timeout, this, &Sidebar_Widget::saveConfig);
    readConfig();
    m_openViews = m_config->readEntry("OpenViews", QStringList());
    m_savedWidth = m_config->readEntry("SavedWidth", 200);
    m_somethingVisible = !m_openViews.isEmpty();
    doLayout();
    QTimer::singleShot(0, this, &Sidebar_Widget::createButtons);
}

bool Sidebar_Widget::createDirectModule(const QString &templ,
                                        const QString &name,
                                        const QUrl &url,
                                        const QString &icon,
                                        const QString &module,
                                        const QString &treeModule)
{
    QString filename = templ;
    const QString myFile = m_moduleManager.addModuleFromTemplate(filename);
    if (!myFile.isEmpty()) {
        qCDebug(SIDEBAR_LOG) << "Writing" << myFile;
        KDesktopFile df(myFile);
        KConfigGroup scf = df.desktopGroup();
        scf.writeEntry("Type", "Link");
        scf.writePathEntry("URL", url.url());
        scf.writeEntry("Icon", icon);
        scf.writeEntry("Name", name);
        scf.writeEntry("X-KDE-KonqSidebarModule", module);
        if (!treeModule.isEmpty()) {
            scf.writeEntry("X-KDE-TreeModule", treeModule);
        }
        int maxKDEWeight = m_moduleManager.getNextAvailableKDEWeight();
        scf.writeEntry("X-KDE-Weight", maxKDEWeight); // because modules with same weight as an already displayed module are inaccessible.
        scf.sync();
        m_moduleManager.moduleAdded(filename);
        QTimer::singleShot(0, this, &Sidebar_Widget::updateButtons);
        return true;
    }
    return false;
}

void Sidebar_Widget::addWebSideBar(const QUrl &url, const QString &name)
{
    //qCDebug(SIDEBAR_LOG) << "Web sidebar entry to be added: " << url << name << endl;

    // Look for existing ones with this URL
    const QStringList files = m_moduleManager.localModulePaths("websidebarplugin*.desktop");
    Q_FOREACH (const QString &file, files) {
        KConfig _scf(file, KConfig::SimpleConfig);
        KConfigGroup scf(&_scf, "Desktop Entry");
        if (scf.readPathEntry("URL", QString()) == url.url()) {
            KMessageBox::information(this, i18n("This entry already exists."));
            return;
        }
    }

    createDirectModule("websidebarplugin%1.desktop", name, url, "internet-web-browser", "konqsidebar_web");
}

void Sidebar_Widget::slotRestoreDeletedButtons()
{
    m_moduleManager.restoreDeletedButtons();
    QTimer::singleShot(0, this, &Sidebar_Widget::updateButtons);
}

void Sidebar_Widget::slotRollback()
{
    if (KMessageBox::warningContinueCancel(this, i18n("<qt>This removes all your entries from the sidebar and adds the system default ones.<br /><b>This procedure is irreversible.</b><br />Do you want to proceed?</qt>")) == KMessageBox::Continue) {
        m_moduleManager.rollbackToDefault();
        QTimer::singleShot(0, this, &Sidebar_Widget::updateButtons);
    }
}

void Sidebar_Widget::saveConfig()
{
    m_config->writeEntry("SingleWidgetMode", m_singleWidgetMode);
    m_config->writeEntry("ShowExtraButtons", m_showExtraButtons);
    m_config->writeEntry("ShowTabsLeft", m_showTabsLeft);
    m_config->writeEntry("HideTabs", m_hideTabs);
    m_config->writeEntry("SavedWidth", m_savedWidth);
    m_config->sync();
}

void Sidebar_Widget::doLayout()
{
    delete m_layout;
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    if (m_showTabsLeft) {
        m_layout->addWidget(m_buttonBar);
        m_layout->addWidget(m_area);
        m_buttonBar->setPosition(KMultiTabBar::Left);
    } else {
        m_layout->addWidget(m_area);
        m_layout->addWidget(m_buttonBar);
        m_buttonBar->setPosition(KMultiTabBar::Right);
    }
    m_layout->activate();
    if (m_hideTabs) {
        m_buttonBar->hide();
    } else {
        m_buttonBar->show();
    }
}

void Sidebar_Widget::aboutToShowConfigMenu()
{
    m_multiViews->setChecked(!m_singleWidgetMode);
    m_showTabLeft->setText(m_showTabsLeft ? i18n("Show Tabs on Right") : i18n("Show Tabs on Left"));
    m_showConfigButton->setChecked(m_showExtraButtons);
}

void Sidebar_Widget::slotSetName()
{
    // Set a name for this sidebar tab
    bool ok;

    // Pop up the dialog asking the user for name.
    const QString name = QInputDialog::getText(this,
                                               i18nc("@title:window", "Set Name"),
                                               i18n("Enter the name:"),
                                               QLineEdit::Normal,
                                               currentButtonInfo().displayName,
                                               &ok);
    if (ok) {
        m_moduleManager.setModuleName(currentButtonInfo().file, name);

        // Update the buttons with a QTimer (why?)
        // Because we're in the RMB of a button that updateButtons deletes...
        // TODO: update THAT button only.
        QTimer::singleShot(0, this, &Sidebar_Widget::updateButtons);
    }
}

// TODO make this less generic. Bookmarks and history have no URL, only folders and websidebars do.
// So this should move to the modules that need it.
void Sidebar_Widget::slotSetURL()
{
    KUrlRequesterDialog dlg(currentButtonInfo().initURL, i18n("Enter a URL:"), this);
    dlg.urlRequester()->setMode(KFile::Directory);
    if (dlg.exec()) {
        m_moduleManager.setModuleUrl(currentButtonInfo().file, dlg.selectedUrl());
        // TODO: update THAT button only.
        QTimer::singleShot(0, this, &Sidebar_Widget::updateButtons);
    }
}

void Sidebar_Widget::slotSetIcon()
{
//  kicd.setStrictIconSize(true);
    const QString iconname = KIconDialog::getIcon(KIconLoader::Small);
    if (!iconname.isEmpty()) {
        m_moduleManager.setModuleIcon(currentButtonInfo().file, iconname);
        // TODO: update THAT button only.
        QTimer::singleShot(0, this, &Sidebar_Widget::updateButtons);
    }
}

void Sidebar_Widget::slotRemove()
{
    if (KMessageBox::warningContinueCancel(this, i18n("<qt>Do you really want to remove the <b>%1</b> tab?</qt>", currentButtonInfo().displayName),
                                           QString(), KStandardGuiItem::del()) == KMessageBox::Continue) {
        m_moduleManager.removeModule(currentButtonInfo().file);
        QTimer::singleShot(0, this, &Sidebar_Widget::updateButtons);
    }
}

void Sidebar_Widget::slotToggleShowHiddenFolders()
{
    Q_ASSERT(currentButtonInfo().canToggleShowHiddenFolders);
    bool newToggleState = !currentButtonInfo().showHiddenFolders;
    m_moduleManager.setShowHiddenFolders(currentButtonInfo().file, newToggleState);
    // TODO: update THAT button only.
    QTimer::singleShot(0, this, SLOT(updateButtons()));
}

void Sidebar_Widget::slotMultipleViews()
{
    m_singleWidgetMode = !m_singleWidgetMode;
    if ((m_singleWidgetMode) && (m_visibleViews.count() > 1)) {
        int tmpViewID = m_latestViewed;
        for (int i = 0; i < m_buttons.count(); i++) {
            if (i != tmpViewID) {
                const ButtonInfo &button = m_buttons.at(i);
                if (button.dock && button.dock->isVisibleTo(this)) {
                    showHidePage(i);
                }
            }
        }
        m_latestViewed = tmpViewID;
    }
    m_configTimer.start(400);
}

void Sidebar_Widget::slotShowTabsLeft()
{
    m_showTabsLeft = ! m_showTabsLeft;
    doLayout();
    m_configTimer.start(400);
}

void Sidebar_Widget::slotShowConfigurationButton()
{
    m_showExtraButtons = ! m_showExtraButtons;
    if (m_showExtraButtons) {
        m_buttonBar->button(-1)->show();
    } else {
        m_buttonBar->button(-1)->hide();

        KMessageBox::information(this,
                                 i18n("You have hidden the sidebar configuration button. To make it visible again, click the right mouse button on any of the sidebar buttons and select \"Show Configuration Button\"."));

    }
    m_configTimer.start(400);
}

void Sidebar_Widget::readConfig()
{
    m_singleWidgetMode = m_config->readEntry("SingleWidgetMode", true);
    m_showExtraButtons = m_config->readEntry("ShowExtraButtons", false);
    m_showTabsLeft = m_config->readEntry("ShowTabsLeft", true);
    m_hideTabs = m_config->readEntry("HideTabs", false);
}

void Sidebar_Widget::stdAction(const char *handlestd)
{
    // ### problem: what about multi mode? We could have multiple modules shown,
    // and if we use Edit/Copy, which one should be used? Need to care about focus...
    qCDebug(SIDEBAR_LOG) << handlestd << "m_activeModule=" << m_activeModule;
    if (m_activeModule) {
        QMetaObject::invokeMethod(m_activeModule, handlestd);
    }
}

void Sidebar_Widget::updateButtons()
{
    //PARSE ALL DESKTOP FILES
    m_openViews = m_visibleViews;

    for (int i = 0; i < m_buttons.count(); ++i) {
        const ButtonInfo &button = m_buttons.at(i);
        if (button.dock) {
            m_noUpdate = true;
            if (button.dock->isVisibleTo(this)) {
                showHidePage(i);
            }
            delete button.module;
            delete button.dock;
        }
        m_buttonBar->removeTab(i);
    }
    m_buttons.clear();

    readConfig();
    doLayout();
    createButtons();
}

void Sidebar_Widget::createButtons()
{
    const QStringList modules = m_moduleManager.modules();
    Q_FOREACH (const QString &fileName, modules) {
        addButton(fileName);
    }

    if (!m_buttonBar->button(-1)) {
        m_buttonBar->appendButton(QIcon::fromTheme("configure"), -1, m_menu,
                                  i18n("Configure Sidebar"));
    }

    if (m_showExtraButtons) {
        m_buttonBar->button(-1)->show();
    } else {
        m_buttonBar->button(-1)->hide();
    }

    for (int i = 0; i < m_buttons.count(); i++) {
        const ButtonInfo &button = m_buttons.at(i);
        if (m_openViews.contains(button.file)) {
            m_buttonBar->setTab(i, true);
            m_noUpdate = true;
            showHidePage(i);
            if (m_singleWidgetMode) {
                break;
            }
        }
    }

    collapseExpandSidebar();
    m_noUpdate = false;
}

bool Sidebar_Widget::openUrl(const QUrl &url)
{
    if (url.scheme() == "sidebar") {
        for (int i = 0; i < m_buttons.count(); i++)
            if (m_buttons.at(i).file == url.path()) {
                KMultiTabBarTab *tab = m_buttonBar->tab(i);
                if (!tab->isChecked()) {
                    tab->animateClick();
                }
                return true;
            }
        return false;
    }
    
    bool ret = false;
    if (m_buttons.isEmpty()) { // special case, since KonqMainWindow uses openURL to launch sidebar before buttons exist
        m_urlBeforeInstanceFlag = true;
    }
    setStoredCurViewUrl(cleanupURL(url));
    m_origURL = m_storedCurViewUrl;

    for (int i = 0; i < m_buttons.count(); i++) {
        const ButtonInfo &button = m_buttons.at(i);
        if (button.dock) {
            if ((button.dock->isVisibleTo(this)) && (button.module)) {
                ret = true;
                button.module->openUrl(url);
            }
        }
    }
    return ret;
}

void Sidebar_Widget::setStoredCurViewUrl(const QUrl& url)
{
    m_storedCurViewUrl = url;
    emit curViewUrlChanged(url);
}

QUrl Sidebar_Widget::cleanupURL(const QString &dirtyURL)
{
    return cleanupURL(QUrl(dirtyURL));
}

QUrl Sidebar_Widget::cleanupURL(const QUrl &dirtyURL)
{
    if (!dirtyURL.isValid()) {
        return dirtyURL;
    }
    QUrl url = dirtyURL;
    if (url.isRelative()) {
        url.setScheme("file");
        if (url.path() == "~") {
            url.setPath(QDir::homePath());
        }
    }
    return url;
}

bool Sidebar_Widget::addButton(const QString &desktopFileName, int pos)
{
    int lastbtn = m_buttons.count();

    qCDebug(SIDEBAR_LOG) << "addButton:" << desktopFileName;

    const QString moduleDataPath = m_moduleManager.moduleDataPath(desktopFileName);
    // Check the desktop file still exists
    if (QStandardPaths::locate(QStandardPaths::GenericDataLocation, moduleDataPath).isEmpty()) {
        return false;
    }

    KSharedConfig::Ptr config = KSharedConfig::openConfig(moduleDataPath,
                                KConfig::NoGlobals,
                                QStandardPaths::GenericDataLocation);
    KConfigGroup configGroup(config, "Desktop Entry");
    const QString icon = configGroup.readEntry("Icon", QString());
    const QString name = configGroup.readEntry("Name", QString());
    const QString comment = configGroup.readEntry("Comment", QString());
    const QUrl url(configGroup.readPathEntry("URL", QString()));
    const QString lib = configGroup.readEntry("X-KDE-KonqSidebarModule");
    const QString configOpenStr = configGroup.readEntry("Open", QString()); // NOTE: is this redundant?

    qDebug() << "Adding button for" << desktopFileName << "at position" << pos;
    if (pos == -1) { // TODO handle insertion
        m_buttonBar->appendTab(QIcon::fromTheme(icon), lastbtn, name);
        ButtonInfo buttonInfo(config, desktopFileName, cleanupURL(url), lib, name, icon);
        buttonInfo.configOpen = configGroup.readEntry("Open", false);
        buttonInfo.canToggleShowHiddenFolders = (configGroup.readEntry("X-KDE-KonqSidebarModule", QString()) == "konqsidebar_tree");
        buttonInfo.showHiddenFolders = configGroup.readEntry("ShowHiddenFolders", false);
        m_buttons.insert(lastbtn, buttonInfo);
        KMultiTabBarTab *tab = m_buttonBar->tab(lastbtn);
        tab->installEventFilter(this);
        connect(tab, &KMultiTabBarTab::clicked, this, &Sidebar_Widget::showHidePage);

        // Set Whats This help
        // This uses the comments in the .desktop files
        tab->setWhatsThis(comment);
    }

    return true;
}

bool Sidebar_Widget::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonPress && ((QMouseEvent *)ev)->button() == Qt::RightButton) {
        KMultiTabBarTab *bt = dynamic_cast<KMultiTabBarTab *>(obj);
        if (bt) {
            qCDebug(SIDEBAR_LOG) << "Request for popup";
            m_currentButtonIndex = -1;
            for (int i = 0; i < m_buttons.count(); i++) {
                if (bt == m_buttonBar->tab(i)) {
                    m_currentButtonIndex = i;
                    break;
                }
            }

            if (m_currentButtonIndex > -1) {
                QMenu *buttonPopup = new QMenu(this);
                buttonPopup->setTitle(currentButtonInfo().displayName);
                buttonPopup->setIcon(QIcon::fromTheme(currentButtonInfo().iconName));
                buttonPopup->addAction(QIcon::fromTheme("edit-rename"), i18n("Set Name..."), this, &Sidebar_Widget::slotSetName); // Item to open a dialog to change the name of the sidebar item (by Pupeno)
                buttonPopup->addAction(QIcon::fromTheme("internet-web-browser"), i18n("Set URL..."), this, &Sidebar_Widget::slotSetURL);
                buttonPopup->addAction(QIcon::fromTheme("preferences-desktop-icons"), i18n("Set Icon..."), this, &Sidebar_Widget::slotSetIcon);
                if (currentButtonInfo().canToggleShowHiddenFolders) {
                    QAction *toggleShowHiddenFolders = buttonPopup->addAction(i18n("Show Hidden Folders..."), this, &Sidebar_Widget::slotToggleShowHiddenFolders);
                    toggleShowHiddenFolders->setCheckable(true);
                    toggleShowHiddenFolders->setChecked(currentButtonInfo().showHiddenFolders);
                }
                buttonPopup->addSeparator();
                buttonPopup->addAction(QIcon::fromTheme("edit-delete"), i18n("Remove"), this, &Sidebar_Widget::slotRemove);
                buttonPopup->addSeparator();
                buttonPopup->addMenu(m_menu);
                buttonPopup->exec(QCursor::pos());
                delete buttonPopup;
            }
            return true;
        }
    }
    return false;
}

void Sidebar_Widget::mousePressEvent(QMouseEvent *ev)
{
    // TODO move to contextMenuEvent?
    if (ev->type() == QEvent::MouseButtonPress && ev->button() == Qt::RightButton) {
        m_menu->exec(QCursor::pos());
    }
}

KonqSidebarModule *Sidebar_Widget::loadModule(QWidget *parent, const QString &desktopName,
        ButtonInfo &buttonInfo, const KSharedConfig::Ptr &config)
{
    const KConfigGroup configGroup = config->group("Desktop Entry");
    KonqSidebarPlugin *plugin = buttonInfo.plugin(this);
    if (!plugin) {
        return nullptr;
    }

    return plugin->createModule(parent, configGroup, desktopName, QVariant());
}

KParts::NavigationExtension *Sidebar_Widget::getExtension()
{
    return KParts::NavigationExtension::childObject(m_partParent);
}

bool Sidebar_Widget::createView(ButtonInfo &buttonInfo)
{
    buttonInfo.dock = nullptr;
    buttonInfo.module = loadModule(m_area, buttonInfo.file, buttonInfo, buttonInfo.configFile);

    if (buttonInfo.module == nullptr) {
        return false;
    }

    buttonInfo.dock = buttonInfo.module->getWidget();
    connectModule(buttonInfo.module);
    connect(this, &Sidebar_Widget::fileSelection, buttonInfo.module, &KonqSidebarModule::openPreview);
    connect(this, &Sidebar_Widget::fileMouseOver, buttonInfo.module, &KonqSidebarModule::openPreviewOnMouseOver);
    connect(this, &Sidebar_Widget::curViewUrlChanged, buttonInfo.module, &KonqSidebarModule::slotCurViewUrlChanged);

    return true;
}

void Sidebar_Widget::showHidePage(int page)
{
    Q_ASSERT(page >= 0);
    Q_ASSERT(page < m_buttons.count());
    ButtonInfo &buttonInfo = m_buttons[page];

    auto buttonInfoHandleURL = [&] () {
        buttonInfo.dock->show();
        m_area->show();
        openUrl(m_storedCurViewUrl); // also runs the buttonInfo.module->openUrl()
        m_visibleViews << buttonInfo.file;
        m_latestViewed = page;
        m_moduleManager.saveOpenViews(m_visibleViews); // TODO: this would be best stored per-window, in the session file
    };

    if (!buttonInfo.dock) {
        if (m_buttonBar->isTabRaised(page)) {
            //SingleWidgetMode
            if (m_singleWidgetMode) {
                if (m_latestViewed != -1) {
                    m_noUpdate = true;
                    showHidePage(m_latestViewed);
                }
            }

            if (!createView(buttonInfo)) {
                m_buttonBar->setTab(page, false);
                return;
            }

            m_buttonBar->setTab(page, true);

            connect(buttonInfo.module, &KonqSidebarModule::setIcon,
                    [this,page](const QString &iconName)
                    { m_buttonBar->tab(page)->setIcon(QIcon::fromTheme(iconName)); });
            connect(buttonInfo.module, &KonqSidebarModule::setCaption,
                    m_buttonBar->tab(page), &KMultiTabBarTab::setText);

            m_area->addWidget(buttonInfo.dock);
            buttonInfoHandleURL();
        }
    } else {
        if ((!buttonInfo.dock->isVisibleTo(this)) && (m_buttonBar->isTabRaised(page))) {
            //SingleWidgetMode
            if (m_singleWidgetMode) {
                if (m_latestViewed != -1) {
                    m_noUpdate = true;
                    showHidePage(m_latestViewed);
                }
            }
            buttonInfoHandleURL();
            m_buttonBar->setTab(page, true);
        } else {
            m_buttonBar->setTab(page, false);
            buttonInfo.dock->hide();
            m_latestViewed = -1;
            m_visibleViews.removeAll(buttonInfo.file);
            if (m_visibleViews.empty()) {
                m_area->hide();
            }
        }
    }

    if (!m_noUpdate) {
        collapseExpandSidebar();
    }
    m_noUpdate = false;
}

void Sidebar_Widget::collapseExpandSidebar()
{
    if (!parentWidget()) {
        return;    // Can happen during destruction
    }
    
    if (m_visibleViews.count() == 0) {
        m_somethingVisible = false;
        parentWidget()->setMaximumWidth(minimumSizeHint().width());
        updateGeometry();
        emit panelHasBeenExpanded(false);
    } else {
        m_somethingVisible = true;
        parentWidget()->setMaximumWidth(32767);
        updateGeometry();
        emit panelHasBeenExpanded(true);
    }
}

QSize Sidebar_Widget::sizeHint() const
{
    if (m_somethingVisible) {
        return QSize(m_savedWidth, 200);
    }
    return minimumSizeHint();
}

void Sidebar_Widget::submitFormRequest(const char *action,
                                       const QString &url,
                                       const QByteArray &formData,
                                       const QString & /*target*/,
                                       const QString &contentType,
                                       const QString & /*boundary*/)
{
    KParts::OpenUrlArguments arguments;
    BrowserArguments browserArguments;
    browserArguments.setContentType("Content-Type: " + contentType);
    browserArguments.postData = formData;
    browserArguments.setDoPost(QByteArray(action).toLower() == "post");
    // boundary?
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    emit getExtension()->openUrlRequest(QUrl(url), arguments, browserArguments);
#else
    if (getBrowserExtension()) {
        emit getBrowserExtension()->browserOpenUrlRequest(QUrl(url), arguments, browserArguments);
    } else {
        emit getExtension()->openUrlRequest(QUrl(url));
    }
#endif
}

void Sidebar_Widget::openUrlRequest(const QUrl &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs)
{
    if (m_storedCurViewUrl == url) { // don't pollute the history stack
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    getExtension()->openUrlRequest(url, args, browserArgs);
#else
    if (getBrowserExtension()) {
        getBrowserExtension()->browserOpenUrlRequest(url, args, browserArgs);
    } else {
        getExtension()->openUrlRequest(url);
    }
#endif

    setStoredCurViewUrl(url);
}

void Sidebar_Widget::createNewWindow(const QUrl &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs,
                                     const WindowArgs &windowArgs)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    getExtension()->createNewWindow(url, args, browserArgs, windowArgs);
#else
    if (getBrowserExtension()) {
        getBrowserExtension()->browserCreateNewWindow(url, args, browserArgs, windowArgs);
    } else {
        getExtension()->createNewWindow(url);
    }
#endif
}

void Sidebar_Widget::slotEnableAction(KonqSidebarModule *module, const char *name, bool enabled)
{
    if (module->getWidget()->isVisible()) {
        emit getExtension()->enableAction(name, enabled);
    }
}

void Sidebar_Widget::doEnableActions()
{
    if (m_activeModule) {
        getExtension()->enableAction("copy", m_activeModule->isCopyEnabled());
        getExtension()->enableAction("cut", m_activeModule->isCutEnabled());
        getExtension()->enableAction("paste", m_activeModule->isPasteEnabled());
    }
}

void Sidebar_Widget::connectModule(KonqSidebarModule *mod)
{
    connect(mod, &KonqSidebarModule::started, this, &Sidebar_Widget::started);
    connect(mod, &KonqSidebarModule::completed, this, &Sidebar_Widget::completed);

    connect(mod, &KonqSidebarModule::popupMenu, this, &Sidebar_Widget::slotPopupMenu);

    connect(mod, &KonqSidebarModule::openUrlRequest, this, &Sidebar_Widget::openUrlRequest);
    connect(mod, &KonqSidebarModule::createNewWindow, this, &Sidebar_Widget::createNewWindow);

    // TODO define in base class
    if (mod->metaObject()->indexOfSignal("submitFormRequest(const char*,QString,QByteArray,QString,QString,QString)") != -1) {
        connect(mod, &KonqSidebarModule::submitFormRequest, this, &Sidebar_Widget::submitFormRequest);
    }

    connect(mod, &KonqSidebarModule::enableAction, this, &Sidebar_Widget::slotEnableAction);
}

Sidebar_Widget::~Sidebar_Widget()
{
    m_config->writeEntry("OpenViews", m_visibleViews);
    if (m_configTimer.isActive()) {
        saveConfig();
    }
    delete m_config;
    m_buttons.clear();
    m_noUpdate = true;
}

void Sidebar_Widget::customEvent(QEvent *ev)
{
    if (KonqFileSelectionEvent::test(ev)) {
        emit fileSelection(static_cast<KonqFileSelectionEvent *>(ev)->selection());
    } else if (KonqFileMouseOverEvent::test(ev)) {
        emit fileMouseOver(static_cast<KonqFileMouseOverEvent *>(ev)->item());
    } else if (KParts::PartActivateEvent::test(ev)) {
        KParts::ReadOnlyPart* rpart = static_cast<KParts::ReadOnlyPart *>( static_cast<KParts::PartActivateEvent *>(ev)->part() );
	
        if (! rpart->url().isEmpty()) {
             setStoredCurViewUrl(cleanupURL(rpart->url()));
        }
	
        if (m_buttons.isEmpty()) { // special case when the event is received but the buttons have not been created yet
            m_urlBeforeInstanceFlag = true;
            m_origURL = m_storedCurViewUrl;
        }

        for (int i = 0; i < m_buttons.count(); i++) {
            const ButtonInfo &button = m_buttons.at(i);
            if (button.dock) {
                if ((button.dock->isVisibleTo(this)) && (button.module)) {
                    // Forward the event to the widget
                    QApplication::sendEvent(button.module, ev);
                    break; // if you found the one you wanted.. exit. (Not sure how this would play out in a split-view sidepanel
                }
            }
        }

        // Forward the event to the widget
        // QApplication::sendEvent(button(), ev);
    }
}

KonqSidebarPlugin *ButtonInfo::plugin(QObject *parent)
{
    if (!m_plugin) {
        KPluginMetaData md = KPluginMetaData::findPluginById(ModuleManager::pluginDirectory(), pluginId);
        auto pluginResult = KPluginFactory::instantiatePlugin<KonqSidebarPlugin>(md, parent);
        if (pluginResult) {
            m_plugin = pluginResult.plugin;
        } else {
            qCWarning(SIDEBAR_LOG) << "error loading sidebar plugin" << pluginResult.errorText;
        }
    }
    return m_plugin;
}

void Sidebar_Widget::slotPopupMenu(KonqSidebarModule *module,
                                   const QPoint &global, const KFileItemList &items,
                                   const KParts::OpenUrlArguments &args,
                                   const BrowserArguments &browserArgs,
                                   KParts::NavigationExtension::PopupFlags flags,
                                   const KParts::NavigationExtension::ActionGroupMap &actionGroups)
{
    m_activeModule = module;
    doEnableActions();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    emit getExtension()->popupMenu(global, items, args, browserArgs, flags, actionGroups);
#else
    if (getBrowserExtension()) {
        emit getBrowserExtension()->browserPopupMenuFromFiles(global, items, args, browserArgs, flags, actionGroups);
    } else {
        emit getExtension()->popupMenu(global, items, args, flags, actionGroups);
    }
#endif
}

void Sidebar_Widget::slotUrlsDropped(const QList<QUrl> &urls)
{
    Q_FOREACH (const QUrl &url, urls) {
        KIO::StatJob *job = KIO::stat(url);
        KJobWidgets::setWindow(job, this);
        connect(job, &KIO::StatJob::result, this, &Sidebar_Widget::slotStatResult);
    }
}

void Sidebar_Widget::slotStatResult(KJob *job)
{
    KIO::StatJob *statJob = static_cast<KIO::StatJob *>(job);
    if (statJob->error()) {
        statJob->uiDelegate()->showErrorMessage();
    } else {
        const QUrl url = statJob->url();
        KFileItem item(statJob->statResult(), url);
        if (item.isDir()) {
            createDirectModule("folder%1.desktop", url.fileName(), url, item.iconName(), "konqsidebar_tree", "Directory");
        } else if (item.currentMimeType().inherits("text/html") || url.scheme().startsWith("http")) {
            const QString name = i18n("Web module");
            createDirectModule("websidebarplugin%1.desktop", name, url, "internet-web-browser", "konqsidebar_web");
        } else {
            // What to do about other kinds of files?
            qCWarning(SIDEBAR_LOG) << "The dropped URL" << url << "is" << item.mimetype() << ", which is not a directory nor an HTML page, what should we do with it?";
        }
    }
}

BrowserExtension *Sidebar_Widget::getBrowserExtension()
{
    return qobject_cast<BrowserExtension*>(getExtension());
}


