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
        copy = cut = paste = trash = del = rename =false;
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
    bool copy;
    bool cut;
    bool paste;
    bool trash;
    bool del;
    bool rename;
    //KonqSidebarIface *m_part;
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

    void popupMenu( const QPoint &global, const KFileItemList &items );
    void popupMenu( const QPoint &global, const KUrl &url,
                    const QString &mimeType, mode_t mode = (mode_t)-1 );
    void enableAction( const char * name, bool enabled );

private:

    bool addButton(const QString &desktopFileName, int pos = -1);
    bool createView(ButtonInfo &buttonInfo);
    KonqSidebarModule *loadModule(QWidget *par, const QString &desktopName,
                                  ButtonInfo& buttonInfo, const KSharedConfig::Ptr& config);
    void readConfig();
    void doLayout();
    void connectModule(QObject *mod);
    void collapseExpandSidebar();
    bool doEnableActions();
    ButtonInfo& currentButtonInfo() { return m_buttons[m_currentButtonIndex]; }

protected Q_SLOTS:
    void aboutToShowAddMenu();
    void triggeredAddMenu(QAction* action);
private:
    KParts::ReadOnlyPart *m_partParent;
    QSplitter *m_area;

    KMultiTabBar *m_buttonBar;
    QVector<ButtonInfo> m_buttons;
    QHBoxLayout *m_layout;
    QAction *m_showTabLeft;
    QMenu *m_menu;

    QMenu *m_addMenu;
    QActionGroup m_addMenuActionGroup;
    QMap<QAction*, KonqSidebarPlugin*> m_pluginForAction;

    //QPointer<ButtonInfo> m_activeModule; // TODO REMOVE?
    int m_currentButtonIndex; // during RMB popups only

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
