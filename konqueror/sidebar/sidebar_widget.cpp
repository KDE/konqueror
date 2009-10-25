/***************************************************************************
                               sidebar_widget.cpp
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
    Copyright (c) 2009 David Faure <faure@kde.org>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Own
#include "sidebar_widget.h"
#include <QDebug>

// std
#include <limits.h>

// Qt
#include <QtCore/QDir>
#include <QtGui/QPushButton>
#include <QtGui/QLayout>
#include <QtGui/QSplitter>
#include <QtCore/QStringList>
#include <QtGui/QMenu>

// KDE
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kiconloader.h>
#include <kicondialog.h>
#include <kmessagebox.h>
#include <kmultitabbar.h>
#include <kinputdialog.h>
#include <konq_events.h>
#include <kfileitem.h>
#include <kio/netaccess.h>
#include <kmenu.h>
#include <kurlrequesterdialog.h>
#include <kfiledialog.h>


void Sidebar_Widget::aboutToShowAddMenu()
{
    m_addMenu->clear();
    m_pluginForAction.clear();

    QList<KConfigGroup> existingGroups;
    // Collect the "already shown" modules
    for (int i = 0; i < m_buttons.count(); ++i) {
        existingGroups.append(m_buttons[i].configFile->group("Desktop Entry"));
    }

    // We need to instanciate all available plugins
    // And since the web module isn't in the default entries at all, we can't just collect
    // the plugins there.
    const KService::List list = m_moduleManager.availablePlugins();
    Q_FOREACH(const KService::Ptr& service, list) {
        if (!service->isValid()) {
            continue;
        }
        KPluginLoader loader(*service, m_partParent->componentData());
        KPluginFactory* factory = loader.factory();
        if (!factory) {
            kWarning() << "Error loading plugin" << service->desktopEntryName() << loader.errorString();
        } else {
            KonqSidebarPlugin* plugin = factory->create<KonqSidebarPlugin>(this);
            if (!plugin) {
                kWarning() << "Error creating KonqSidebarPlugin from" << service->desktopEntryName();
            } else {
                const QList<QAction*> actions = plugin->addNewActions(&m_addMenuActionGroup,
                                                                      existingGroups,
                                                                      QVariant());
                // Remember which plugin the action came from.
                // We can't use QAction::setData for that, because we let plugins use that already.
                Q_FOREACH(QAction* action, actions) {
                    m_pluginForAction.insert(action, plugin);
                }
                m_addMenu->addActions(actions);
            }
        }
    }
    m_addMenu->addSeparator();
    m_addMenu->addAction(i18n("Rollback to System Default"), this, SLOT(slotRollback()));
}

void Sidebar_Widget::triggeredAddMenu(QAction* action)
{
    KonqSidebarPlugin* plugin = m_pluginForAction.value(action);
    m_pluginForAction.clear(); // save memory

    QString templ = plugin->templateNameForNewModule(action->data(), QVariant());
    Q_ASSERT(!templ.contains('/'));
    if (templ.isEmpty())
        return;
    const QString myFile = m_moduleManager.addModuleFromTemplate(templ);
    if (myFile.isEmpty())
        return;

    kDebug() << myFile << "filename=" << templ;
    KDesktopFile df(myFile);
    KConfigGroup configGroup = df.desktopGroup();
    const bool ok = plugin->createNewModule(action->data(), configGroup, this, QVariant());
    df.sync();
    if (ok) {
        m_moduleManager.moduleAdded(templ /*contains the final filename*/);
        // TODO only add the new button
        QTimer::singleShot(0, this, SLOT(updateButtons()));
    } else {
        QFile::remove(myFile);
    }
}


Sidebar_Widget::Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par, const QString &currentProfile)
    : QWidget(parent),
      m_partParent(par),
      m_addMenuActionGroup(this),
      m_config(new KConfigGroup(KSharedConfig::openConfig("konqsidebartng.rc"),
                                currentProfile)),
      m_moduleManager(m_config)
{
    m_somethingVisible = false;
    m_noUpdate = false;
    m_layout = 0;
    m_currentButtonIndex = -1;
    //m_activeModule = 0; // TODO REMOVE?
    //m_userMovedSplitter = false;
    //kDebug() << "**** Sidebar_Widget:SidebarWidget()";
    m_hasStoredUrl = false;
    m_latestViewed = -1;
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    m_area = new QSplitter(Qt::Vertical, this);
    m_area->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    m_area->setMinimumWidth(0);

    m_buttonBar = new KMultiTabBar(KMultiTabBar::Left,this);

    m_menu = new QMenu(this);
    m_menu->setIcon(KIcon("configure"));
    m_menu->setTitle(i18n("Configure Navigation Panel"));

    m_addMenu = m_menu->addMenu(i18n("Add New"));
    connect(m_addMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowAddMenu()));
    connect(&m_addMenuActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(triggeredAddMenu(QAction*)));
    m_menu->addSeparator();
    m_multiViews = m_menu->addAction(i18n("Multiple Views"), this, SLOT(slotMultipleViews()));
    m_multiViews->setCheckable(true);
    m_showTabLeft = m_menu->addAction(i18n("Show Tabs Left"), this, SLOT(slotShowTabsLeft()));
    m_showConfigButton = m_menu->addAction(i18n("Show Configuration Button"), this, SLOT(slotShowConfigurationButton()));
    m_showConfigButton->setCheckable(true);
    m_menu->addSeparator();
    m_menu->addAction(KIcon("window-close"), i18n("Close Navigation Panel"),
                      par, SLOT(deleteLater()));

    connect(m_menu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowConfigMenu()));

    m_configTimer.setSingleShot(true);
    connect(&m_configTimer, SIGNAL(timeout()),
            this, SLOT(saveConfig()));
    readConfig();
    m_openViews = m_config->readEntry("OpenViews",QStringList());
    m_savedWidth = m_config->readEntry("SavedWidth",200);
    m_somethingVisible = !m_openViews.isEmpty();
    doLayout();
    QTimer::singleShot(0,this,SLOT(createButtons()));
}

void Sidebar_Widget::addWebSideBar(const KUrl& url, const QString& name) {
    //kDebug() << "Web sidebar entry to be added: " << url << name << endl;

    // Look for existing ones with this URL
    const QStringList files = m_moduleManager.localModulePaths("websidebarplugin*.desktop");
    Q_FOREACH(const QString& file, files) {
        KConfig _scf(file, KConfig::SimpleConfig);
        KConfigGroup scf(&_scf, "Desktop Entry");
        if (scf.readPathEntry("URL", QString()) == url.url()) {
            // We already have this one!
            KMessageBox::information(this,
                                     i18n("This entry already exists."));
            return;
        }
    }

    QString filename = "websidebarplugin%1.desktop";
    const QString myFile = m_moduleManager.addModuleFromTemplate(filename);
    if (!myFile.isEmpty()) {
        KDesktopFile df(myFile);
        KConfigGroup scf = df.desktopGroup();
        scf.writeEntry("Type", "Link");
        scf.writePathEntry("URL", url.url());
        scf.writeEntry("Icon", "internet-web-browser");
        scf.writeEntry("Name", name);
        scf.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_web");
        scf.sync();
        m_moduleManager.moduleAdded(filename);
        QTimer::singleShot(0, this, SLOT(updateButtons()));
    }
}


void Sidebar_Widget::slotRollback()
{
    if (KMessageBox::warningContinueCancel(this, i18n("<qt>This removes all your entries from the sidebar and adds the system default ones.<br /><b>This procedure is irreversible</b><br />Do you want to proceed?</qt>"))==KMessageBox::Continue)
    {
        m_moduleManager.rollbackToDefault();
        QTimer::singleShot(0, this, SLOT(updateButtons()));
    }
}


void Sidebar_Widget::saveConfig()
{
    m_config->writeEntry("SingleWidgetMode",m_singleWidgetMode);
    m_config->writeEntry("ShowExtraButtons",m_showExtraButtons);
    m_config->writeEntry("ShowTabsLeft", m_showTabsLeft);
    m_config->writeEntry("HideTabs", m_hideTabs);
    m_config->writeEntry("SavedWidth",m_savedWidth);
    m_config->sync();
}

void Sidebar_Widget::doLayout()
{
    delete m_layout;
    m_layout = new QHBoxLayout(this);
    m_layout->setMargin( 0 );
    m_layout->setSpacing( 0 );
    if  (m_showTabsLeft) {
        m_layout->addWidget(m_buttonBar);
        m_layout->addWidget(m_area);
        m_buttonBar->setPosition(KMultiTabBar::Left);
    } else {
        m_layout->addWidget(m_area);
        m_layout->addWidget(m_buttonBar);
        m_buttonBar->setPosition(KMultiTabBar::Right);
    }
    m_layout->activate();
    if (m_hideTabs) m_buttonBar->hide();
    else m_buttonBar->show();
}


void Sidebar_Widget::aboutToShowConfigMenu()
{
    m_multiViews->setChecked(!m_singleWidgetMode);
    m_showTabLeft->setText(m_showTabsLeft ? i18n("Show Tabs Right") : i18n("Show Tabs Left"));
    m_showConfigButton->setChecked(m_showExtraButtons);
}

void Sidebar_Widget::slotSetName()
{
    // Set a name for this sidebar tab
    bool ok;

    // Pop up the dialog asking the user for name.
    const QString name = KInputDialog::getText(i18n("Set Name"), i18n("Enter the name:"),
                                               currentButtonInfo().displayName, &ok, this);

    if(ok) {
        m_moduleManager.setModuleName(currentButtonInfo().file, name);

        // Update the buttons with a QTimer (why?)
        // Because we're in the RMB of a button that updateButtons deletes...
        // TODO: update THAT button only.
        QTimer::singleShot(0, this, SLOT(updateButtons()));
    }
}

// TODO make this less generic. Bookmarks and history have no URL, only folders and websidebars do.
// So this should move to the modules that need it.
void Sidebar_Widget::slotSetURL()
{
    KUrlRequesterDialog dlg( currentButtonInfo().URL, i18n("Enter a URL:"), this );
    dlg.fileDialog()->setMode( KFile::Directory );
    if (dlg.exec())
    {
        m_moduleManager.setModuleUrl(currentButtonInfo().file, dlg.selectedUrl());
        // TODO: update THAT button only.
        QTimer::singleShot(0,this,SLOT(updateButtons()));
    }
}

void Sidebar_Widget::slotSetIcon()
{
//	kicd.setStrictIconSize(true);
    const QString iconname = KIconDialog::getIcon(KIconLoader::Small);
    if (!iconname.isEmpty()) {
        m_moduleManager.setModuleIcon(currentButtonInfo().file, iconname);
        // TODO: update THAT button only.
        QTimer::singleShot(0,this,SLOT(updateButtons()));
    }
}

void Sidebar_Widget::slotRemove()
{
    if (KMessageBox::warningContinueCancel(this,i18n("<qt>Do you really want to remove the <b>%1</b> tab?</qt>", currentButtonInfo().displayName),
                                           QString(),KStandardGuiItem::del())==KMessageBox::Continue)
    {
        m_moduleManager.removeModule(currentButtonInfo().file);
        QTimer::singleShot(0,this,SLOT(updateButtons()));
    }
}

void Sidebar_Widget::slotMultipleViews()
{
    m_singleWidgetMode = !m_singleWidgetMode;
    if ((m_singleWidgetMode) && (m_visibleViews.count()>1))
    {
        int tmpViewID = m_latestViewed;
        for (int i=0; i < m_buttons.count(); i++) {
            if (i != tmpViewID) {
                const ButtonInfo &button = m_buttons.at(i);
                if (button.dock && button.dock->isVisibleTo(this))
                    showHidePage(i);
            }
        }
        m_latestViewed = tmpViewID;
    }
    m_configTimer.start(400);
}

void Sidebar_Widget::slotShowTabsLeft( )
{
    m_showTabsLeft = ! m_showTabsLeft;
    doLayout();
    m_configTimer.start(400);
}

void Sidebar_Widget::slotShowConfigurationButton( )
{
    m_showExtraButtons = ! m_showExtraButtons;
    if(m_showExtraButtons)
    {
        m_buttonBar->button(-1)->show();
    }
    else
    {
        m_buttonBar->button(-1)->hide();

        KMessageBox::information(this,
                                 i18n("You have hidden the navigation panel configuration button. To make it visible again, click the right mouse button on any of the navigation panel buttons and select \"Show Configuration Button\"."));

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
// PENDING(kdab) Review
#ifdef KDAB_TEMPORARILY_REMOVED
    ButtonInfo* mod = m_activeModule;

    if (!mod || !mod->module)
        return;

    kDebug() << "Try calling >active< module's (" << mod->module->metaObject()->className() << ") slot " << handlestd;

    QMetaObject::invokeMethod( mod->module, handlestd );
#else // KDAB_TEMPORARILY_REMOVED
    qWarning("Sorry, not implemented: Sidebar_Widget::stdAction");
    return ;
#endif // KDAB_TEMPORARILY_REMOVED
}


void Sidebar_Widget::updateButtons()
{
    //PARSE ALL DESKTOP FILES
    m_openViews = m_visibleViews;

    for (int i = 0; i < m_buttons.count(); ++i) {
        const ButtonInfo& button = m_buttons.at(i);
        if (button.dock)
        {
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
    Q_FOREACH(const QString& fileName, modules) {
        addButton(fileName);
    }

    if (!m_buttonBar->button(-1)) {
        m_buttonBar->appendButton(SmallIcon("configure"), -1, m_menu,
                                  i18n("Configure Sidebar"));
    }

    if (m_showExtraButtons) {
        m_buttonBar->button(-1)->show();
    } else {
        m_buttonBar->button(-1)->hide();
    }

    for (int i = 0; i < m_buttons.count(); i++)
    {
        const ButtonInfo& button = m_buttons.at(i);
        if (m_openViews.contains(button.file))
        {
            m_buttonBar->setTab(i,true);
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

bool Sidebar_Widget::openUrl(const KUrl &url)
{
    if (url.protocol()=="sidebar")
    {
        for (int i=0;i<m_buttons.count();i++)
            if (m_buttons.at(i).file==url.path())
            {
                KMultiTabBarTab *tab = m_buttonBar->tab(i);
                if (!tab->isChecked())
                    tab->animateClick();
                return true;
            }
        return false;
    }

    m_storedUrl=url;
    m_hasStoredUrl=true;
    bool ret = false;
    for (int i=0;i<m_buttons.count();i++)
    {
        const ButtonInfo& button = m_buttons.at(i);
        if (button.dock)
        {
            if ((button.dock->isVisibleTo(this)) && (button.module))
            {
                ret = true;
                button.module->openUrl(url);
            }
        }
    }
    return ret;
}

bool Sidebar_Widget::addButton(const QString &desktopFileName, int pos)
{
    int lastbtn = m_buttons.count();

    kDebug() << "addButton:" << desktopFileName;

    const QString moduleDataPath = m_moduleManager.moduleDataPath(desktopFileName);
    // Check the desktop file still exists
    if (KStandardDirs::locate("data", moduleDataPath).isEmpty())
        return false;

    KSharedConfig::Ptr config = KSharedConfig::openConfig(moduleDataPath,
                                                          KConfig::NoGlobals,
                                                          "data");
    KConfigGroup configGroup(config, "Desktop Entry");
    const QString icon = configGroup.readEntry("Icon", QString());
    const QString name = configGroup.readEntry("Name", QString());
    const QString comment = configGroup.readEntry("Comment", QString());
    const QString url = configGroup.readPathEntry("URL",QString());
    const QString lib = configGroup.readEntry("X-KDE-KonqSidebarModule");

    if (pos == -1) // TODO handle insertion
    {
        m_buttonBar->appendTab(SmallIcon(icon), lastbtn, name);
        ButtonInfo buttonInfo(config, desktopFileName, url, lib, name, icon);
        m_buttons.insert(lastbtn, buttonInfo);
        KMultiTabBarTab *tab = m_buttonBar->tab(lastbtn);
        tab->installEventFilter(this);
        connect(tab, SIGNAL(clicked(int)), this, SLOT(showHidePage(int)));

        // Set Whats This help
        // This uses the comments in the .desktop files
        tab->setWhatsThis(comment);
    }

    return true;
}

bool Sidebar_Widget::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type()==QEvent::MouseButtonPress && ((QMouseEvent *)ev)->button()==Qt::RightButton)
    {
        KMultiTabBarTab *bt=dynamic_cast<KMultiTabBarTab*>(obj);
        if (bt)
        {
            kDebug()<<"Request for popup";
            m_currentButtonIndex = -1;
            for (int i=0;i<m_buttons.count();i++) {
                if (bt == m_buttonBar->tab(i)) {
                    m_currentButtonIndex = i;
                    break;
                }
            }

            if (m_currentButtonIndex > -1)
            {
                KMenu *buttonPopup=new KMenu(this);
                buttonPopup->addTitle(SmallIcon(currentButtonInfo().iconName), currentButtonInfo().displayName);
                buttonPopup->addAction(KIcon("edit-rename"), i18n("Set Name..."), this, SLOT(slotSetName())); // Item to open a dialog to change the name of the sidebar item (by Pupeno)
                buttonPopup->addAction(KIcon("internet-web-browser"), i18n("Set URL..."), this, SLOT(slotSetURL()));
                buttonPopup->addAction(KIcon("preferences-desktop-icons"), i18n("Set Icon..."), this, SLOT(slotSetIcon()));
                buttonPopup->addSeparator();
                buttonPopup->addAction(KIcon("edit-delete"), i18n("Remove"), this, SLOT(slotRemove()));
                buttonPopup->addSeparator();
                buttonPopup->addMenu(m_menu);
                buttonPopup->setItemEnabled(2,!currentButtonInfo().URL.isEmpty());
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
    if (ev->type() == QEvent::MouseButtonPress && ((QMouseEvent *)ev)->button() == Qt::RightButton)
        m_menu->exec(QCursor::pos());
}

KonqSidebarModule *Sidebar_Widget::loadModule(QWidget *parent, const QString &desktopName,
                                              ButtonInfo& buttonInfo, const KSharedConfig::Ptr& config)
{
    const KConfigGroup configGroup = config->group("Desktop Entry");
    KonqSidebarPlugin* plugin = buttonInfo.plugin(this);
    if (!plugin)
        return 0;

    return plugin->createModule(m_partParent->componentData(),
                                parent, configGroup, desktopName, QVariant());
}

KParts::BrowserExtension *Sidebar_Widget::getExtension()
{
    return KParts::BrowserExtension::childObject(m_partParent);
}

bool Sidebar_Widget::createView(ButtonInfo& buttonInfo)
{
    buttonInfo.dock = 0;
    buttonInfo.module = loadModule(m_area, buttonInfo.file, buttonInfo, buttonInfo.configFile);

    if (buttonInfo.module == 0) {
        return false;
    }

    buttonInfo.dock = buttonInfo.module->getWidget();
    connectModule(buttonInfo.module);
    connect(this, SIGNAL(fileSelection(KFileItemList)),
            buttonInfo.module, SLOT(openPreview(KFileItemList)));
    connect(this, SIGNAL(fileMouseOver(KFileItem)),
            buttonInfo.module, SLOT(openPreviewOnMouseOver(KFileItem)));

    return true;
}

void Sidebar_Widget::showHidePage(int page)
{
    Q_ASSERT(page >= 0);
    Q_ASSERT(page < m_buttons.count());
    ButtonInfo& buttonInfo = m_buttons[page];
    if (!buttonInfo.dock)
    {
        if (m_buttonBar->isTabRaised(page))
        {
            //SingleWidgetMode
            if (m_singleWidgetMode)
            {
                if (m_latestViewed != -1)
                {
                    m_noUpdate = true;
                    showHidePage(m_latestViewed);
                }
            }

            if (!createView(buttonInfo))
            {
                m_buttonBar->setTab(page,false);
                return;
            }

            m_buttonBar->setTab(page,true);

            connect(buttonInfo.module,
                    SIGNAL(setIcon(const QString&)),
                    m_buttonBar->tab(page),
                    SLOT(setIcon(const QString&)));

            connect(buttonInfo.module,
                    SIGNAL(setCaption(const QString&)),
                    m_buttonBar->tab(page),
                    SLOT(setText(const QString&)));

            m_area->addWidget(buttonInfo.dock);
            buttonInfo.dock->show();
            m_area->show();
            if (m_hasStoredUrl)
                buttonInfo.module->openUrl(m_storedUrl);
            m_visibleViews<<buttonInfo.file;
            m_latestViewed=page;
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

            buttonInfo.dock->show();
            m_area->show();
            m_latestViewed = page;
            if (m_hasStoredUrl)
                buttonInfo.module->openUrl(m_storedUrl);
            m_visibleViews << buttonInfo.file;
            m_buttonBar->setTab(page,true);
        } else {
            m_buttonBar->setTab(page,false);
            buttonInfo.dock->hide();
            m_latestViewed = -1;
            m_visibleViews.removeAll(buttonInfo.file);
            if (m_visibleViews.empty()) m_area->hide();
        }
    }

    if (!m_noUpdate)
        collapseExpandSidebar();
    m_noUpdate = false;
}

void Sidebar_Widget::collapseExpandSidebar()
{
    if (!parentWidget())
        return; // Can happen during destruction

    if (m_visibleViews.count()==0)
    {
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
    if (m_somethingVisible)
        return QSize(m_savedWidth,200);
    return minimumSizeHint();
}

void Sidebar_Widget::submitFormRequest(const char *action,
                                       const QString& url,
                                       const QByteArray& formData,
                                       const QString& /*target*/,
                                       const QString& contentType,
                                       const QString& /*boundary*/ )
{
    KParts::OpenUrlArguments arguments;
    KParts::BrowserArguments browserArguments;
    browserArguments.setContentType("Content-Type: " + contentType);
    browserArguments.postData = formData;
    browserArguments.setDoPost(QByteArray(action).toLower() == "post");
    // boundary?
    emit getExtension()->openUrlRequest(KUrl( url ), arguments, browserArguments);
}

void Sidebar_Widget::openUrlRequest( const KUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs)
{
    getExtension()->openUrlRequest(url,args, browserArgs);
}

void Sidebar_Widget::createNewWindow( const KUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs,
                                      const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart **part )
{
    getExtension()->createNewWindow(url,args,browserArgs, windowArgs,part);
}

void Sidebar_Widget::enableAction( const char * name, bool enabled )
{
// PENDING(kdab) Review
#ifdef KDAB_TEMPORARILY_REMOVED
    // TODO ###### How could this ever happen?!?
    if ((qstrcmp("ButtonInfo", sender()->parent()->metaObject()->className()) == 0))
    {
        ButtonInfo *btninfo = static_cast<ButtonInfo*>(sender()->parent());
        if (btninfo)
        {
            QString n(name);
            if (n == "copy")
                btninfo->copy = enabled;
            else if (n == "cut")
                btninfo->cut = enabled;
            else if (n == "paste")
                btninfo->paste = enabled;
            else if (n == "trash")
                btninfo->trash = enabled;
            else if (n == "del")
                btninfo->del = enabled;
            else if (n == "rename")
                btninfo->rename = enabled;
        }
    }
#else // KDAB_TEMPORARILY_REMOVED
    qWarning("Sorry, not implemented: Sidebar_Widget::enableAction");
    return ;
#endif // KDAB_TEMPORARILY_REMOVED
}


bool  Sidebar_Widget::doEnableActions()
{
// PENDING(kdab) Review
#ifdef KDAB_TEMPORARILY_REMOVED
    if ((qstrcmp("ButtonInfo", sender()->parent()->metaObject()->className()) != 0))
    {
        kDebug()<<"Couldn't set active module, aborting";
        return false;
    } else {
        m_activeModule=static_cast<ButtonInfo*>(sender()->parent());
        getExtension()->enableAction( "copy", m_activeModule->copy );
        getExtension()->enableAction( "cut", m_activeModule->cut );
        getExtension()->enableAction( "paste", m_activeModule->paste );
        getExtension()->enableAction( "trash", m_activeModule->trash );
        getExtension()->enableAction( "del", m_activeModule->del );
        getExtension()->enableAction( "rename", m_activeModule->rename );
        return true;
    }

#else // KDAB_TEMPORARILY_REMOVED
    qWarning("Sorry, not implemented: Sidebar_Widget::doEnableActions");
    return false;
#endif // KDAB_TEMPORARILY_REMOVED
}

void Sidebar_Widget::popupMenu( const QPoint &global, const KFileItemList &items )
{
    if (doEnableActions())
        getExtension()->popupMenu(global,items);
}

void Sidebar_Widget::popupMenu(
    const QPoint &global, const KUrl &url,
    const QString &mimeType, mode_t mode )
{
    if (doEnableActions()) {
        KParts::OpenUrlArguments args;
        args.setMimeType(mimeType);
        getExtension()->popupMenu(global,url,mode, args);
    }
}

void Sidebar_Widget::connectModule(QObject *mod)
{
    if (mod->metaObject()->indexOfSignal("started(KIO::Job*)") != -1) {
        connect(mod,SIGNAL(started(KIO::Job *)),this, SIGNAL(started(KIO::Job*)));
    }

    if (mod->metaObject()->indexOfSignal("completed()") != -1) {
        connect(mod,SIGNAL(completed()),this,SIGNAL(completed()));
    }

    if (mod->metaObject()->indexOfSignal("popupMenu(QPoint,KUrl,QString,mode_t)") != -1) {
        connect(mod,SIGNAL(popupMenu( const QPoint &, const KUrl &,
                                      const QString &, mode_t)),this,SLOT(popupMenu( const
                                                                                     QPoint &, const KUrl&, const QString &, mode_t)));
    }

    if (mod->metaObject()->indexOfSignal("popupMenu(QPoint,KUrl,QString,mode_t)") != -1) {
        connect(mod,SIGNAL(popupMenu( const QPoint &,
                                      const KUrl &,const QString &, mode_t)),this,
                SLOT(popupMenu( const QPoint &,
                                const KUrl &,const QString &, mode_t)));
    }

    if (mod->metaObject()->indexOfSignal("popupMenu(QPoint,KFileItemList)") != -1) {
        connect(mod,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )),
                this,SLOT(popupMenu( const QPoint &, const KFileItemList & )));
    }

    if (mod->metaObject()->indexOfSignal("openUrlRequest(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)") != -1) {
        connect(mod,SIGNAL(openUrlRequest( const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments&)),
                this,SLOT(openUrlRequest( const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments&)));
    }

    if (mod->metaObject()->indexOfSignal("submitFormRequest(const char*,QString,QByteArray,QString,QString,QString)") != -1) {
        connect(mod,
                SIGNAL(submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)),
                this,
                SLOT(submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)));
    }

    if (mod->metaObject()->indexOfSignal("enableAction(const char*,bool)") != -1) {
        connect(mod,SIGNAL(enableAction( const char *, bool)),
                this,SLOT(enableAction(const char *, bool)));
    }

    if (mod->metaObject()->indexOfSignal("createNewWindow(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)") != -1) {
        connect(mod,SIGNAL(createNewWindow(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)),
                this,SLOT(createNewWindow(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs,KParts::ReadOnlyPart**)));
    }
}

Sidebar_Widget::~Sidebar_Widget()
{
    m_config->writeEntry("OpenViews", m_visibleViews);
    if (m_configTimer.isActive())
        saveConfig();
    delete m_config;
    m_buttons.clear();
    m_noUpdate = true;
}

void Sidebar_Widget::customEvent(QEvent* ev)
{
    if (KonqFileSelectionEvent::test(ev)) {
        emit fileSelection(static_cast<KonqFileSelectionEvent*>(ev)->selection());
    } else if (KonqFileMouseOverEvent::test(ev)) {
        emit fileMouseOver(static_cast<KonqFileMouseOverEvent*>(ev)->item());
    }
}

KonqSidebarPlugin* ButtonInfo::plugin(QObject* parent)
{
    if (!m_plugin) {
        KPluginLoader loader(libName);
        KPluginFactory* factory = loader.factory();
        if (!factory) {
            kWarning() << "error loading" << libName << loader.errorString();
            return 0;
        }
        KonqSidebarPlugin* plugin = factory->create<KonqSidebarPlugin>(parent);
        if (!plugin) {
            kWarning() << "error creating object from" << libName;
            return 0;
        }
        m_plugin = plugin;
    }
    return m_plugin;
}

#include "sidebar_widget.moc"
