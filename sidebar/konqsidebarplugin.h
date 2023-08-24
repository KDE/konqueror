/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2001, 2002 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _konqsidebarplugin_h_
#define _konqsidebarplugin_h_

#include <kconfiggroup.h>
#include <QWidget>

#include <kparts/part.h>

//We don't use kf5compat.h to avoid linking with libkonq
//TODO KF6: when removing compatibility with KF5, remove #if
#include <QtGlobal>
#if QT_VERSION_MAJOR < 6
#include <KParts/BrowserExtension>
namespace KParts {
  typedef BrowserExtension NavigationExtension;
}
#else
#include <KParts/NavigationExtension>
#endif
#include <kio/job.h>
#include <kfileitem.h>

#ifndef KONQSIDEBARPLUGIN_EXPORT
# if defined(MAKE_KONQSIDEBARPLUGIN_LIB)
/* We are building this library */
#  define KONQSIDEBARPLUGIN_EXPORT Q_DECL_EXPORT
# else
/* We are using this library */
#  define KONQSIDEBARPLUGIN_EXPORT Q_DECL_IMPORT
# endif
#endif

class KonqSidebarModulePrivate;

/**
 * Base class for modules, implemented in plugins.
 * This class is exported, make sure you keep BINARY COMPATIBILITY!
 * (Alternatively, add a Version key to the plugins desktop file...)
 *
 * A plugin can instantiate multiple modules, for various configurations.
 * Example: the dirtree plugin can create a module for "/", a module for $HOME, etc.
 */
class KONQSIDEBARPLUGIN_EXPORT KonqSidebarModule : public QObject
{
    Q_OBJECT
public:
    KonqSidebarModule(QObject *parent,
                      const KConfigGroup &configGroup);
    ~KonqSidebarModule() override;

    virtual QWidget *getWidget() = 0;
    KConfigGroup configGroup();

    /**
     * Enable/disable a standard konqueror action (cut, copy, paste, print)
     * See KParts::NavigationExtension::enableAction
     */
    void enableCopy(bool enabled);
    void enableCut(bool enabled);
    void enablePaste(bool enabled);
    bool isCopyEnabled() const;
    bool isCutEnabled() const;
    bool isPasteEnabled() const;

    void showPopupMenu(const QPoint &global, const KFileItemList &items,
                       const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                       const KParts::BrowserArguments &browserArgs = KParts::BrowserArguments(),
                       KParts::NavigationExtension::PopupFlags flags = KParts::NavigationExtension::DefaultPopupItems,
                       const KParts::NavigationExtension::ActionGroupMap &actionGroups = KParts::NavigationExtension::ActionGroupMap());

protected:
    /**
     * Called by the sidebar's openUrl. Reimplement this in order to
     * follow the navigation happening in konqueror's current view.
     */
    virtual void handleURL(const QUrl &url)
    {
        Q_UNUSED(url);
    }
    virtual void handlePreview(const KFileItemList &items);
    virtual void handlePreviewOnMouseOver(const KFileItem &items); //not used yet

public Q_SLOTS:
    void openUrl(const QUrl &url);

    void openPreview(const KFileItemList &items);

    void openPreviewOnMouseOver(const KFileItem &item); // not used yet

    virtual void slotCurViewUrlChanged(const QUrl &url){Q_UNUSED(url);}

Q_SIGNALS:
    void started(KIO::Job *);
    void completed();
    void setIcon(const QString &icon);
    void setCaption(const QString &caption);

    /**
     * Ask konqueror to open @p url.
     */
    void openUrlRequest(const QUrl &url, const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                        const KParts::BrowserArguments &browserArgs = KParts::BrowserArguments());
    /**
     * Ask konqueror to create a new window (or tab, see BrowserArguments) for @p url.
     */
    void createNewWindow(const QUrl &url, const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                         const KParts::BrowserArguments &browserArgs = KParts::BrowserArguments(),
                         const KParts::WindowArgs & = KParts::WindowArgs());

    /**
     * Ask konqueror to show the standard popup menu for the given @p items.
     */
    void popupMenu(KonqSidebarModule *module,
                   const QPoint &global, const KFileItemList &items,
                   const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                   const KParts::BrowserArguments &browserArgs = KParts::BrowserArguments(),
                   KParts::NavigationExtension::PopupFlags flags = KParts::NavigationExtension::DefaultPopupItems,
                   const KParts::NavigationExtension::ActionGroupMap &actionGroups = KParts::NavigationExtension::ActionGroupMap());

    // TODO
    void submitFormRequest(const char *, const QString &, const QByteArray &, const QString &, const QString &, const QString &);

    void enableAction(KonqSidebarModule *module, const char *name, bool enabled);

private:
    KConfigGroup m_configGroup;
    KonqSidebarModulePrivate *const d;

};

/**
 * The plugin class is the "factory" for sidebar modules.
 * It can create a module (based on a given configuration),
 * it can also provide QActions for letting the user create new modules interactively.
 */
class KONQSIDEBARPLUGIN_EXPORT KonqSidebarPlugin : public QObject
{
    Q_OBJECT
public:
    KonqSidebarPlugin(QObject *parent, const QVariantList &args)
        : QObject(parent)
    {
        Q_UNUSED(args);
    }
    ~KonqSidebarPlugin() override {}

    /**
     * Create new module for the sidebar.
     * @param parent parent widget, for the plugin's widget
     * @param configGroup desktop group from the plugin's desktop file
     * @param desktopName filename of the plugin's desktop file - for compatibility only
     * @param unused for future extensions
     */
    virtual KonqSidebarModule *createModule(QWidget *parent,
                                            const KConfigGroup &configGroup,
                                            const QString &desktopname,
                                            const QVariant &unused) = 0;

    /**
     * Creates QActions for the "Add new" menu.
     * @param parent parent object for the actions
     * @param existingModules list of existing modules, to avoid creating "unique"
     * modules multiple times
     * @param unused for future extensions
     */
    virtual QList<QAction *> addNewActions(QObject *parent,
                                           const QList<KConfigGroup> &existingModules,
                                           const QVariant &unused)
    {
        Q_UNUSED(parent); Q_UNUSED(existingModules); Q_UNUSED(unused);
        return QList<QAction *>();
    }

    /**
     * Returns the template of the filename to used for new modules.
     * @param actionData data given to the QAction (for modules who provide
     * multiple "new" actions)
     * @param unused for future extensions
     */
    virtual QString templateNameForNewModule(const QVariant &actionData,
            const QVariant &unused) const
    {
        Q_UNUSED(actionData); Q_UNUSED(unused);
        return QString();
    }

    /**
     * Finally create a new module, added by the user.
     * @param actionData data given to the QAction (for modules who provide
     * multiple "new" actions)
     * @param configGroup desktop group of the new desktop file. The desktop file
     * is deleted if the method returns false (e.g. cancelled by user).
     * @param parentWidget in case the plugin shows dialogs in this method
     * @param unused for future extensions
     */
    virtual bool createNewModule(const QVariant &actionData, KConfigGroup &configGroup,
                                 QWidget *parentWidget,
                                 const QVariant &unused)
    {
        Q_UNUSED(actionData); Q_UNUSED(configGroup);
        Q_UNUSED(parentWidget); Q_UNUSED(unused);
        return false;
    }
};

#endif
