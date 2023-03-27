/* This file is part of FSView.
    SPDX-FileCopyrightText: 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

/*
 * Classes for breadth-first search in local filesystem
 */

#ifndef KONQ_PLUGIN_SCAN_H
#define KONQ_PLUGIN_SCAN_H

#include <QFile>
#include <QVector>
#include <kio/global.h>

class ScanDir;
class ScanFile;

class ScanItem
{
public:
    ScanItem(const QString &p, ScanDir *d)
    {
        absPath = p;
        dir = d;
    }

    QString absPath;
    ScanDir *dir;
};

typedef QList<ScanItem *> ScanItemList;

/**
 * Listener for events from directory scanning.
 *
 * You can register a listener for the ScanManager to get
 * all scan events and a listener for every ScanDir for
 * directory specific scan events.
 *
 * sizeChanged is called when a scan of a subdirectory
 * finished.
 */
class ScanListener
{
public:
    virtual ~ScanListener() {}
    virtual void scanStarted(ScanDir *) {}
    virtual void sizeChanged(ScanDir *) {}
    virtual void scanFinished(ScanDir *) {}
    // destroyed events are not delivered to listeners of ScanManager
    virtual void destroyed(ScanDir *) {}
    virtual void destroyed(ScanFile *) {}
};

/**
 * ScanManager
 *
 * Start/Stop/Restart Scans. Example:
 *
 *   ScanManager m("/opt");
 *   m.startScan();
 *   while(m.scan());
 */
class ScanManager
{
public:
    ScanManager();
    ScanManager(const QString &path);
    ~ScanManager();

    /** Set the top path for scanning
     * The ScanDir object created gets attribute data.
     */
    ScanDir *setTop(const QString &path, int data = 0);
    ScanDir *top()
    {
        return _topDir;
    }

    bool scanRunning();
    int scanLength() const
    {
        return _list.count();
    }

    /**
     * Starts the scan. Stop previous scan if running.
     * For the actual scan to happen, you have to call
     * scan() periodically.
     *
     * If from !=0, restart scan at given position; from must
     * be from the previous scan of this manager.
     */
    void startScan(ScanDir *from = nullptr);

    /** Stop a current running scan.
     * Make all directories to finish their scan.
     */
    void stopScan();

    /**
     * Scan first directory from todo list.
     * Directories added to the todo list are attributed with data.
     * Returns the number of new subdirectories created for scanning.
     */
    int scan(int data);

    /* set listener to get a callbacks from this ScanDir */
    void setListener(ScanListener *);
    ScanListener *listener()
    {
        return _listener;
    }

private:
    ScanItemList _list;
    ScanDir *_topDir;
    ScanListener *_listener;
};

class ScanFile
{
public:
    ScanFile();
    ScanFile(const QString &n, KIO::fileoffset_t s);
    ~ScanFile();

    const QString &name()
    {
        return _name;
    }
    KIO::fileoffset_t size()
    {
        return _size;
    }

    /* set listener to get callbacks from this ScanDir */
    void setListener(ScanListener *l)
    {
        _listener = l;
    }
    ScanListener *listener()
    {
        return _listener;
    }

private:
    QString _name;
    KIO::fileoffset_t _size;
    ScanListener *_listener;
};

typedef QVector<ScanFile> ScanFileVector;
typedef QVector<ScanDir> ScanDirVector;

/**
 * A directory to scan.
 * You can attribute a directory to scan with a
 * integer data attribute.
 */
class ScanDir
{
public:
    ScanDir();
    ScanDir(const QString &n, ScanManager *m,
            ScanDir *p = nullptr, int data = 0);
    ~ScanDir();

    /* Get items of this directory
     * and append subdirectories to todo list.
     *
     * Directories added to the todo list are attributed with data.
     * Returns the number of new subdirectories created for scanning.
     */
    int scan(ScanItem *si, ScanItemList &list, int data);

    /* clear scan objects below */
    void clear();

    /*
     * Setup for child rescan
     */
    void setupChildRescan();

    /* Absolute path. Warning: Slow, loops to top parent. */
    QString path();

    /* get integer data attribute */
    int data()
    {
        return _data;
    }
    void setData(int d)
    {
        _data = d;
    }

    ScanFileVector &files()
    {
        return _files;
    }
    ScanDirVector &dirs()
    {
        return _dirs;
    }
    const QString &name()
    {
        return _name;
    }
    KIO::fileoffset_t size()
    {
        update();
        return _size;
    }
    unsigned int fileCount()
    {
        update();
        return _fileCount;
    }
    unsigned int dirCount()
    {
        update();
        return _dirCount;
    }
    ScanDir *parent()
    {
        return _parent;
    }
    bool scanStarted()
    {
        return (_dirsFinished >= 0);
    }
    bool scanFinished()
    {
        return (_dirsFinished == _dirs.count());
    }
    bool scanRunning()
    {
        return scanStarted() && !scanFinished();
    }

    /* set listener to get a callbacks from this ScanDir */
    void setListener(ScanListener *);
    ScanListener *listener()
    {
        return _listener;
    }
    ScanManager *manager()
    {
        return _manager;
    }

    /* force current scan to be finished */
    void finish();

private:
    void update();
    bool isForbiddenDir(QString &);

    /* this propagates file count and size to upper dirs */
    void subScanFinished();
    void callScanStarted();
    void callSizeChanged();
    void callScanFinished();

    ScanFileVector _files;
    ScanDirVector _dirs;

    QString _name;
    bool _dirty; /* needs a call to update() */
    KIO::fileoffset_t _size, _fileSize;
    unsigned int _fileCount, _dirCount;
    int _dirsFinished, _data;
    ScanDir *_parent;
    ScanListener *_listener;
    ScanManager *_manager;
};

#endif // KONQ_PLUGIN_SCAN_H
