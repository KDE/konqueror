/* This file is part of FSView.
    SPDX-FileCopyrightText: 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

/*
 * FSView specialization of TreeMapItem class.
 */

#ifndef INODE_H
#define INODE_H

#include <QMap>
#include <QFileInfo>
#include <QString>
#include <QPixmap>

#include <QMimeType>

#include "treemap.h"
#include "scan.h"

/**
 * A specialized version of a TreeMapItem
 * for representation of an Directory or File.
 *
 * These are dynamically created on drawing.
 * The real breadth-first scanning of the filesystem
 * uses ScanDir:scan.
 */
class Inode: public TreeMapItem, public ScanListener
{
public:
    Inode();
    Inode(ScanDir *, Inode *);
    Inode(ScanFile *, Inode *);
    ~Inode() override;
    void init(const QString &);

    void setPeer(ScanDir *);

    TreeMapItemList *children() override;

    double value() const override;
    double size() const;
    unsigned int fileCount() const;
    unsigned int dirCount() const;
    QString path() const;
    QString text(int i) const override;
    QPixmap pixmap(int i) const override;
    QColor backColor() const override;
    QMimeType mimeType() const;

    const QFileInfo &fileInfo() const
    {
        return _info;
    }
    ScanDir *dirPeer()
    {
        return _dirPeer;
    }
    ScanFile *filePeer()
    {
        return _filePeer;
    }
    bool isDir()
    {
        return (_dirPeer != nullptr);
    }

    void sizeChanged(ScanDir *) override;
    void scanFinished(ScanDir *) override;
    void destroyed(ScanDir *) override;
    void destroyed(ScanFile *) override;

private:
    void setMetrics(double, unsigned int);

    QFileInfo _info;
    ScanDir *_dirPeer;
    ScanFile *_filePeer;

    double _sizeEstimation;
    unsigned int _fileCountEstimation, _dirCountEstimation;

    bool _resortNeeded;

    // Cached values, calculated lazy.
    // This means a change even in const methods, thus has to be "mutable"
    mutable bool _mimeSet, _mimePixmapSet;
    mutable QMimeType _mimeType;
    mutable QPixmap _mimePixmap;
};

#endif
