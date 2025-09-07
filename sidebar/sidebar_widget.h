/*
    SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _SIDEBAR_WIDGET_
#define _SIDEBAR_WIDGET_

#include <QActionGroup>
#include <kconfiggroup.h>
#include <QTimer>

#include <QPointer>
#include <QUrl>

#include <kparts/part.h>
#include <KParts/PartActivateEvent>
#include <KSharedConfig>

#include "konqsidebarplugin.h"
#include "module_manager.h"
#include "browserextension.h"

class KonqMultiTabBar;
class KonqSidebarPlugin;
class QMenu;
class QHBoxLayout;
class QSplitter;
class QContextMenuEvent;

class ButtonInfo
{
public:
    ButtonInfo()
    {
    }
    ButtonInfo(const KSharedConfig::Ptr &configFile_,
               const QString &file_,
               const QUrl &url_,
               const QString &id,
               const QString &dispName_,
               const QString &iconName_)
        : configFile(configFile_), file(file_),
          pluginId(id), displayName(dispName_),
          iconName(iconName_), initURL(url_)
    {
    }

    KonqSidebarPlugin *plugin(QObject *parent);

    KSharedConfig::Ptr configFile;
    QString file;
    QPointer<QWidget> dock;
    KonqSidebarModule *module = nullptr;
    KonqSidebarPlugin *m_plugin = nullptr;

    QString pluginId;
    QString displayName;
    QString iconName;
    QUrl initURL;

    bool configOpen = false;
    bool canToggleShowHiddenFolders = false;
    bool showHiddenFolders = false;
};

class Sidebar_Widget: public QWidget
{
    Q_OBJECT
public:
    friend class ButtonInfo;
public:
    Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par,
                   const QString &currentProfile);
    ~Sidebar_Widget() override;
    bool openUrl(const QUrl &url);
    void stdAction(const char *handlestd);

    KParts::NavigationExtension *getExtension();
    BrowserExtension *getBrowserExtension();
    QSize sizeHint() const override;

public Q_SLOTS:
    void addWebSideBar(const QUrl &url, const QString &name);

protected:
    void customEvent(QEvent *ev) override;
    bool eventFilter(QObject *, QEvent *) override;
    void contextMenuEvent(QContextMenuEvent *ev) override;

protected Q_SLOTS:
    void showHidePage(int value);
    void createButtons();
    void updateButtons();
    void slotRestoreDeletedButtons();
    void slotRollback();
    void aboutToShowConfigMenu();
    void saveConfig();

    void slotMultipleViews();
    void slotShowTabsLeft();
    void slotShowConfigurationButton();

    void slotSetName();
    void slotSetURL();
    void slotSetIcon();
    void slotToggleShowHiddenFolders();
    void slotRemove();

    void slotUrlsDropped(const QList<QUrl> &urls);

Q_SIGNALS:
    void started(KIO::Job *);
    void completed();
    void fileSelection(const KFileItemList &items);
    void fileMouseOver(const KFileItem &item);
    void curViewUrlChanged(const QUrl &url);

    /* The following public slots are wrappers for browserextension signals */
public Q_SLOTS:
    void openUrlRequest(const QUrl &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs);
    /* @internal
     * ### KDE4 remove me
     */
    void submitFormRequest(const char *, const QString &, const QByteArray &, const QString &, const QString &, const QString &);
    void createNewWindow(const QUrl &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs,
                         const WindowArgs &windowArgs);

    void slotEnableAction(KonqSidebarModule *module, const char *name, bool enabled);

private:

    QUrl cleanupURL(const QString &url);
    QUrl cleanupURL(const QUrl &url);
    bool addButton(const QString &desktopFileName, int pos = -1);
    bool createView(ButtonInfo &buttonInfo);

    //Changes m_storedCurViewUrl and emits the curViewUrlChanged signal
    void setStoredCurViewUrl(const QUrl &url);
    KonqSidebarModule *loadModule(QWidget *par, const QString &desktopName,
                                  ButtonInfo &buttonInfo, const KSharedConfig::Ptr &config);
    void readConfig();
    void doLayout();
    void connectModule(KonqSidebarModule *mod);
    void collapseExpandSidebar();
    void doEnableActions();
    ButtonInfo &currentButtonInfo()
    {
        return m_buttons[m_currentButtonIndex];
    }

    /**
     * Create a module without the interactive "createNewModule" implemented
     * in the plugin.
     */
    bool createDirectModule(const QString &templ,
                            const QString &name,
                            const QUrl &url,
                            const QString &icon,
                            const QString &module,
                            const QString &treeModule = QString());
private Q_SLOTS:
    void aboutToShowAddMenu();
    void triggeredAddMenu(QAction *action);

    void slotPopupMenu(KonqSidebarModule *, const QPoint &global, const KFileItemList &items,
                       const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                       const BrowserArguments &browserArgs = BrowserArguments(),
                       KParts::NavigationExtension::PopupFlags flags = KParts::NavigationExtension::DefaultPopupItems,
                       const KParts::NavigationExtension::ActionGroupMap &actionGroups = KParts::NavigationExtension::ActionGroupMap());

    void slotStatResult(KJob *job);

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
    QMap<QAction *, KonqSidebarPlugin *> m_pluginForAction;

    QPointer<KonqSidebarModule> m_activeModule; // during RMB popups inside the module

    int m_currentButtonIndex; // during RMB popups (over tabs) only, see currentButtonInfo

    KConfigGroup *m_config;
    QTimer m_configTimer;

    QUrl m_storedCurViewUrl;
    QUrl m_origURL;

    int m_savedWidth;
    int m_latestViewed;

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

    bool m_urlBeforeInstanceFlag = false;

Q_SIGNALS:
    void panelHasBeenExpanded(bool);
};

#endif
