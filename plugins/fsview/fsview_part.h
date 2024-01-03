/* This file is part of FSView.
    SPDX-FileCopyrightText: 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

/*
 * The KPart embedding the FSView widget
 */

#ifndef FSVIEW_PART_H
#define FSVIEW_PART_H

#include <kparts_version.h>
#include <kparts/part.h>

#include <KIO/Job>

#include "fsview.h"
#include "browserextension.h"

class KActionMenu;

class FSViewPart;

class FSViewNavigationExtension : public BrowserExtension
{
    Q_OBJECT

public:
    explicit FSViewNavigationExtension(FSViewPart *viewPart);
    ~FSViewNavigationExtension() override;

public slots:
    void selected(TreeMapItem *);

    void itemSingleClicked(TreeMapItem *i);
    void itemDoubleClicked(TreeMapItem *i);

    void trash();
    void del();
    void editMimeType();

    void refresh();

    void copy()
    {
        copySelection(false);
    }
    void cut()
    {
        copySelection(true);
    }
private:
    void copySelection(bool move);

    FSView *_view;
};

class FSJob: public KIO::Job
{
    Q_OBJECT

public:
    explicit FSJob(FSView *);

    virtual void kill(bool quietly = true);

public slots:
    void progressSlot(int percent, int dirs, const QString &lastDir);

#if QT_VERSION_MAJOR < 6
protected slots:
    void slotInfoMessage(KJob * job, const QString & plain, const QString & rich=QString()) override;
#endif

private:
    FSView *_view;
};

class FSViewPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    Q_PROPERTY(bool supportsUndo READ supportsUndo)
public:
    FSViewPart(QWidget *parentWidget,
               QObject *parent,
               const KPluginMetaData& metaData,
               const QList<QVariant> &args);
    ~FSViewPart() override;

    bool supportsUndo() const
    {
        return false;
    }

    FSView *view() const
    {
        return _view;
    }

    /**
     * Return custom componentName for KXMLGUIClient, as for historical reasons the plugin id is not used
     */
    QString componentName() const override;

public slots:
    void updateActions();
    void contextMenu(TreeMapItem *, const QPoint &);
    void showInfo();
    void showHelp();
    void startedSlot();
    void completedSlot(int dirs);
    void slotShowVisMenu();
    void slotShowAreaMenu();
    void slotShowDepthMenu();
    void slotShowColorMenu();
    void slotProperties();

protected:
    /**
     * This must be implemented by each part
     */
    bool openFile() override;
    bool openUrl(const QUrl &url) override;
    bool closeUrl() override;

private:
    void setNonStandardActionEnabled(const char *actionName, bool enabled);
    KFileItemList selectedFileItems() const;

    FSView *_view;
    FSJob *_job;
    FSViewNavigationExtension *_ext;
    KActionMenu *_visMenu, *_areaMenu, *_depthMenu, *_colorMenu;
};

#endif // FSVIEW_PART_H
