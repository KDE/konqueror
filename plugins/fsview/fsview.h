/* This file is part of FSView.
    SPDX-FileCopyrightText: 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

/*
 * FSView specialization of TreeMap classes.
 */

#ifndef FSVIEW_H
#define FSVIEW_H

#include <QMap>
#include <QFileInfo>
#include <QString>

#include <kconfiggroup.h>

#include "treemap.h"
#include "inode.h"
#include "scan.h"

class QMenu;
class KConfig;

/* Cached Metric info config */
class MetricEntry
{
public:
    MetricEntry()
    {
        size = 0.0;
        fileCount = 0;
        dirCount = 0;
    }
    MetricEntry(double s, unsigned int f, unsigned int d)
    {
        size = s;
        fileCount = f;
        dirCount = d;
    }

    double size;
    unsigned int fileCount, dirCount;
};

/**
 * The root object for the treemap.
 *
 * Does context menu handling and
 * asynchronous file size update
 */
class FSView : public TreeMapWidget, public ScanListener
{
    Q_OBJECT

public:
    enum ColorMode { None = 0, Depth, Name, Owner, Group, Mime };

    explicit FSView(Inode *, QWidget *parent = nullptr);
    ~FSView() override;

    KConfig *config()
    {
        return _config;
    }

    void setPath(const QString &);
    QString path()
    {
        return _path;
    }
    int pathDepth()
    {
        return _pathDepth;
    }

    void setColorMode(FSView::ColorMode cm);
    FSView::ColorMode colorMode() const
    {
        return _colorMode;
    }
    // returns true if string was recognized
    bool setColorMode(const QString &);
    QString colorModeString() const;

    void requestUpdate(Inode *);

    /* Implementation of listener interface of ScanManager.
     * Used to calculate progress info */
    void scanFinished(ScanDir *) override;

    void stop();

    static bool getDirMetric(const QString &, double &, unsigned int &, unsigned int &);
    static void setDirMetric(const QString &, double, unsigned int, unsigned int);
    void saveMetric(KConfigGroup *);
    void saveFSOptions();

    // for color mode
    void addColorItems(QMenu *, int);

    QList<QUrl> selectedUrls();

public slots:
    void selected(TreeMapItem *);
    void contextMenu(TreeMapItem *, const QPoint &);
    void quit();
    void doUpdate();
    void doRedraw();
    void colorActivated(QAction *);

signals:
    void started();
    void progress(int percent, int dirs, const QString &lastDir);
    void completed(int dirs);

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    KConfig *_config;
    ScanManager _sm;

    // when a contextMenu is shown, we don't allow async. refreshing
    bool _allowRefresh;
    // a cache for directory sizes with long lasting updates
    static QMap<QString, MetricEntry> _dirMetric;

    // current root path
    int _pathDepth;
    QString _path;

    // for progress info
    int _progressPhase;
    int _chunkData1, _chunkData2, _chunkData3;
    int _chunkSize1, _chunkSize2, _chunkSize3;
    int _progress, _progressSize, _dirsFinished;
    ScanDir *_lastDir;

    ColorMode _colorMode;
    int _colorID;
};

#endif // FSVIEW_H

