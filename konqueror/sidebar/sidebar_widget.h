/***************************************************************************
                               sidebar_widget.h
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _SIDEBAR_WIDGET_
#define _SIDEBAR_WIDGET_

#include <QActionGroup>
#include <kconfiggroup.h>
#include <QtCore/QTimer>

#include <QtCore/QPointer>

#include <kurl.h>
#include <kparts/part.h>

#include "konqsidebarplugin.h"
#include "module_manager.h"

class KonqMultiTabBar;
class KonqSidebarPlugin;
class QMenu;
class KMultiTabBar;
class QHBoxLayout;
class QSplitter;
class QStringList;
class KMenu;

class ButtonInfo
{
public:
    ButtonInfo()
        : module(NULL), m_plugin(NULL)
    {
    }
    ButtonInfo(const KSharedConfig::Ptr& configFile_,
               const QString& file_,
               const QString &url_,const QString &lib,
               const QString &dispName_, const QString &iconName_)
        : configFile(configFile_),
          file(file_), dock(NULL),
          module(NULL), m_plugin(NULL),
          URL(url_), libName(lib), displayName(dispName_), iconName(iconName_)
    {
    }

    ~ButtonInfo() {}

    KonqSidebarPlugin* plugin(QObject* parent);

    KSharedConfig::Ptr configFile;
    QString file;
    QPointer<QWidget> dock;
    KonqSidebarModule *module;
    KonqSidebarPlugin* m_plugin;
    QString URL; // TODO remove
    QString libName;
    QString displayName;
    QString iconName;
};

class Sidebar_Widget: public QWidget
{
    Q_OBJECT
public:
    friend class ButtonInfo;
public:
    Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par,
                   const QString &currentProfile);
    ~Sidebar_Widget();
    bool openUrl(const KUrl &url);
    void stdAction(const char *handlestd);

    KParts::BrowserExtension *getExtension();
    virtual QSize sizeHint() const;

public Q_SLOTS:
    void addWebSideBar(const KUrl& url, const QString& name);

protected:
    void customEvent(QEvent* ev);
    //void resizeEvent(QResizeEvent* ev);
    virtual bool eventFilter(QObject*,QEvent*);
    virtual void mousePressEvent(QMouseEvent*);

protected Q_SLOTS:
    void showHidePage(int value);
    void createButtons();
    void updateButtons();
    void slotRollback();
    void aboutToShowConfigMenu();
    void saveConfig();

    void slotMultipleViews();
    void slotShowTabsLeft();
    void slotShowConfigurationButton();

    void slotSetName();
    void slotSetURL();
    void slotSetIcon();
    void slotRemove();

    void slotUrlsDropped(const KUrl::List& urls);

Q_SIGNALS:
    void started(KIO::Job *);
    void completed();
    void fileSelection(const KFileItemList& iems);
    void fileMouseOver(const KFileItem& item);

    /* The following public slots are wrappers for browserextension signals */
public Q_SLOTS:
    void openUrlRequest( const KUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs );
    /* @internal
     * ### KDE4 remove me
     */
    void submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&);
    void createNewWindow(const KUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs,
                         const KParts::WindowArgs &windowArgs);

    void slotEnableAction(KonqSidebarModule* module, const char * name, bool enabled);

private:

    bool addButton(const QString &desktopFileName, int pos = -1);
    bool createView(ButtonInfo &buttonInfo);
    KonqSidebarModule *loadModule(QWidget *par, const QString &desktopName,
                                  ButtonInfo& buttonInfo, const KSharedConfig::Ptr& config);
    void readConfig();
    void doLayout();
    void connectModule(KonqSidebarModule *mod);
    void collapseExpandSidebar();
    void doEnableActions();
    ButtonInfo& currentButtonInfo() { return m_buttons[m_currentButtonIndex]; }

    /**
     * Create a module without the interactive "createNewModule" implemented
     * in the plugin.
     */
    bool createDirectModule(const QString& templ,
                            const QString& name,
                            const KUrl& url,
                            const QString& icon,
                            const QString& module,
                            const QString& treeModule = QString());
private Q_SLOTS:
    void aboutToShowAddMenu();
    void triggeredAddMenu(QAction* action);

    void slotPopupMenu(KonqSidebarModule*, const QPoint &global, const KFileItemList &items,
                       const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                       const KParts::BrowserArguments &browserArgs = KParts::BrowserArguments(),
                       KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::DefaultPopupItems,
                       const KParts::BrowserExtension::ActionGroupMap& actionGroups = KParts::BrowserExtension::ActionGroupMap());

    void slotAddItem(const KFileItem& item);

private:
    KParts::ReadOnlyPart *m_partParent;
    QSplitter *m_area;

    KonqMultiTabBar *m_buttonBar;
    QVector<ButtonInfo> m_buttons;
    QHBoxLayout *m_layout;
    QAction *m_showTabLeft;
    QMenu *m_menu;

    QMenu *m_addMenu;
    QActionGroup m_addMenuActionGroup;
    QMap<QAction*, KonqSidebarPlugin*> m_pluginForAction;

    QPointer<KonqSidebarModule> m_activeModule; // during RMB popups inside the module

    int m_currentButtonIndex; // during RMB popups (over tabs) only, see currentButtonInfo

    KConfigGroup *m_config;
    QTimer m_configTimer;

    KUrl m_storedUrl;
    int m_savedWidth;
    int m_latestViewed;

    bool m_hasStoredUrl;
    bool m_singleWidgetMode;
    bool m_showTabsLeft;
    bool m_hideTabs;
    bool m_showExtraButtons;
    bool m_somethingVisible;
    bool m_noUpdate;

    QAction *m_multiViews;
    QAction *m_showConfigButton;

    QStringList m_visibleViews; // The views that are actually open
    QStringList m_openViews; // The views that should be opened

    ModuleManager m_moduleManager;

Q_SIGNALS:
    void panelHasBeenExpanded(bool);
};

#endif
