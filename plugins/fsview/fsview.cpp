/* This file is part of FSView.
    SPDX-FileCopyrightText: 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

/*
 * FSView specialization of TreeMap classes.
 */

#include "fsview.h"

#include <QDir>
#include <QTimer>
#include <QApplication>
#include <QDebug>

#include <KLocalizedString>
#include <kconfig.h>
#include <kmessagebox.h>

#include <kio/job.h>
#include <kauthorized.h>
#include <kurlauthorized.h>

#include "fsviewdebug.h"

// FSView

QMap<QString, MetricEntry> FSView::_dirMetric;

FSView::FSView(Inode *base, QWidget *parent)
    : TreeMapWidget(base, parent)
{
    setFieldType(0, i18n("Name"));
    setFieldType(1, i18n("Size"));
    setFieldType(2, i18n("File Count"));
    setFieldType(3, i18n("Directory Count"));
    setFieldType(4, i18n("Last Modified"));
    setFieldType(5, i18n("Owner"));
    setFieldType(6, i18n("Group"));
    setFieldType(7, i18n("Mime Type"));

    // defaults
    setVisibleWidth(4, true);
    setSplitMode(TreeMapItem::Rows);
    setFieldForced(0, true); // show directory names
    setFieldForced(1, true); // show directory sizes
    setSelectionMode(TreeMapWidget::Extended);

    _colorMode = Depth;
    _pathDepth = 0;
    _allowRefresh = true;

    _progressPhase = 0;
    _chunkData1 = 0;
    _chunkData2 = 0;
    _chunkData3 = 0;
    _chunkSize1 = 0;
    _chunkSize2 = 0;
    _chunkSize3 = 0;
    _progressSize = 0;
    _progress = 0;
    _dirsFinished = 0;
    _lastDir = nullptr;

    _config = new KConfig(QStringLiteral("fsviewrc"));

    // restore TreeMap visualization options of last execution
    KConfigGroup tmconfig(_config, "TreeMap");
    restoreOptions(&tmconfig);
    QString str = tmconfig.readEntry("ColorMode");
    if (!str.isEmpty()) {
        setColorMode(str);
    }

    if (_dirMetric.count() == 0) {
        // restore metric cache
        KConfigGroup cconfig(_config, "MetricCache");
        int ccount = cconfig.readEntry("Count", 0);
        int i, f, d;
        double s;
        QString str;
        for (i = 1; i <= ccount; i++) {
            str = QStringLiteral("Dir%1").arg(i);
            if (!cconfig.hasKey(str)) {
                continue;
            }
            str = cconfig.readPathEntry(str, QString());
            s = cconfig.readEntry(QStringLiteral("Size%1").arg(i), 0.0);
            f = cconfig.readEntry(QStringLiteral("Files%1").arg(i), 0);
            d = cconfig.readEntry(QStringLiteral("Dirs%1").arg(i), 0);
            if (s == 0.0 || f == 0 || d == 0) {
                continue;
            }
            setDirMetric(str, s, f, d);
        }
    }

    _sm.setListener(this);
}

FSView::~FSView()
{
    delete _config;
}

void FSView::stop()
{
    _sm.stopScan();
}

void FSView::setPath(const QString &p)
{
    Inode *b = (Inode *)base();
    if (!b) {
        return;
    }

    //qCDebug(FSVIEWLOG) << "FSView::setPath " << p;

    // stop any previous updating
    stop();

    QFileInfo fi(p);
    _path = fi.absoluteFilePath();
    if (!fi.isDir()) {
        _path = fi.absolutePath();
    }
    _path = QDir::cleanPath(_path);
    _pathDepth = _path.count('/');

    QUrl u = QUrl::fromLocalFile(_path);
    if (!KUrlAuthorized::authorizeUrlAction(QStringLiteral("list"), QUrl(), u)) {
        QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, u.toDisplayString());
        KMessageBox::error(this, msg);
    }

    ScanDir *d = _sm.setTop(_path);

    b->setPeer(d);

    setWindowTitle(QStringLiteral("%1 - FSView").arg(_path));
    requestUpdate(b);
}

QList<QUrl> FSView::selectedUrls()
{
    QList<QUrl> urls;

    for (TreeMapItem *i: selection()) {
        QUrl u = QUrl::fromLocalFile(((Inode *)i)->path());
        urls.append(u);
    }
    return urls;
}

bool FSView::getDirMetric(const QString &k,
                          double &s, unsigned int &f, unsigned int &d)
{
    QMap<QString, MetricEntry>::iterator it;

    it = _dirMetric.find(k);
    if (it == _dirMetric.end()) {
        return false;
    }

    s = (*it).size;
    f = (*it).fileCount;
    d = (*it).dirCount;

    if (0) {
        qCDebug(FSVIEWLOG) << "getDirMetric " << k;
    }
    if (0) {
        qCDebug(FSVIEWLOG) << " - got size " << s << ", files " << f;
    }

    return true;
}

void FSView::setDirMetric(const QString &k,
                          double s, unsigned int f, unsigned int d)
{
    if (0) qCDebug(FSVIEWLOG) << "setDirMetric '" << k << "': size "
                              << s << ", files " << f << ", dirs " << d;
    _dirMetric.insert(k, MetricEntry(s, f, d));
}

void FSView::requestUpdate(Inode *i)
{
    if (0) qCDebug(FSVIEWLOG) << "FSView::requestUpdate(" << i->path()
                              << ")";

    ScanDir *peer = i->dirPeer();
    if (!peer) {
        return;
    }

    peer->clear();
    i->clear();

    if (!_sm.scanRunning()) {
        QTimer::singleShot(0, this, SLOT(doUpdate()));
        QTimer::singleShot(100, this, SLOT(doRedraw()));

        /* start new progress chunk */
        _progressPhase = 1;
        _chunkData1 += 3;
        _chunkData2 = _chunkData1 + 1;
        _chunkData3 = _chunkData1 + 2;
        _chunkSize1 = 0;
        _chunkSize2 = 0;
        _chunkSize3 = 0;
        peer->setData(_chunkData1);

        _progressSize = 0;
        _progress = 0;
        _dirsFinished = 0;
        _lastDir = nullptr;
        emit started();
    }

    _sm.startScan(peer);
}

void FSView::scanFinished(ScanDir *d)
{
    /* if finished directory was from last progress chunk, increment */
    int data = d->data();
    switch (_progressPhase) {
    case 1:
        if (data == _chunkData1) {
            _chunkSize1--;
        }
        break;
    case 2:
        if (data == _chunkData1) {
            _progress++;
        }
        if (data == _chunkData2) {
            _chunkSize2--;
        }
        break;
    case 3:
        if ((data == _chunkData1) ||
                (data == _chunkData2)) {
            _progress++;
        }
        if (data == _chunkData3) {
            _chunkSize3--;
        }
        break;
    case 4:
        if ((data == _chunkData1) ||
                (data == _chunkData2) ||
                (data == _chunkData3)) {
            _progress++;
        }
        break;
    default:
        break;
    }

    _lastDir = d;
    _dirsFinished++;

    if (0) qCDebug(FSVIEWLOG) << "FSFiew::scanFinished: " << d->path()
                              << ", Data " << data
                              << ", Progress " << _progress << "/"
                              << _progressSize;
}

void FSView::selected(TreeMapItem *i)
{
    setPath(((Inode *)i)->path());
}

void FSView::contextMenu(TreeMapItem *i, const QPoint &p)
{
    QMenu popup;

    QMenu *spopup = new QMenu(i18n("Go To"));
    QMenu *dpopup = new QMenu(i18n("Stop at Depth"));
    QMenu *apopup = new QMenu(i18n("Stop at Area"));
    QMenu *fpopup = new QMenu(i18n("Stop at Name"));

    // choosing from the selection menu will give a selectionChanged() signal
    addSelectionItems(spopup, 901, i);
    popup.addMenu(spopup);

    QAction *actionGoUp = popup.addAction(i18n("Go Up"));
    popup.addSeparator();
    QAction *actionStopRefresh = popup.addAction(i18n("Stop Refresh"));
    actionStopRefresh->setEnabled(_sm.scanRunning());
    QAction *actionRefresh = popup.addAction(i18n("Refresh"));
    actionRefresh->setEnabled(!_sm.scanRunning());

    QAction *actionRefreshSelected = nullptr;
    if (i) {
        actionRefreshSelected = popup.addAction(i18n("Refresh '%1'", i->text(0)));
    }
    popup.addSeparator();
    addDepthStopItems(dpopup, 1001, i);
    popup.addMenu(dpopup);
    addAreaStopItems(apopup, 1101, i);
    popup.addMenu(apopup);
    addFieldStopItems(fpopup, 1201, i);
    popup.addMenu(fpopup);

    popup.addSeparator();

    QMenu *cpopup = new QMenu(i18n("Color Mode"));
    addColorItems(cpopup, 1401);
    popup.addMenu(cpopup);
    QMenu *vpopup = new QMenu(i18n("Visualization"));
    addVisualizationItems(vpopup, 1301);
    popup.addMenu(vpopup);

    _allowRefresh = false;
    QAction *action = popup.exec(mapToGlobal(p));
    _allowRefresh = true;
    if (!action) {
        return;
    }

    if (action == actionGoUp) {
        Inode *i = (Inode *) base();
        if (i) {
            setPath(i->path() + QLatin1String("/.."));
        }
    } else if (action == actionStopRefresh) {
        stop();
    } else if (action == actionRefreshSelected) {
        //((Inode*)i)->refresh();
        requestUpdate((Inode *)i);
    } else if (action == actionRefresh) {
        Inode *i = (Inode *) base();
        if (i) {
            requestUpdate(i);
        }
    }
}

void FSView::saveMetric(KConfigGroup *g)
{
    QMap<QString, MetricEntry>::iterator it;
    int c = 1;
    for (it = _dirMetric.begin(); it != _dirMetric.end(); ++it) {
        g->writePathEntry(QStringLiteral("Dir%1").arg(c), it.key());
        g->writeEntry(QStringLiteral("Size%1").arg(c), (*it).size);
        g->writeEntry(QStringLiteral("Files%1").arg(c), (*it).fileCount);
        g->writeEntry(QStringLiteral("Dirs%1").arg(c), (*it).dirCount);
        c++;
    }
    g->writeEntry("Count", c - 1);
}

void FSView::setColorMode(FSView::ColorMode cm)
{
    if (_colorMode == cm) {
        return;
    }

    _colorMode = cm;
    redraw();
}

bool FSView::setColorMode(const QString &mode)
{
    if (mode == QLatin1String("None")) {
        setColorMode(None);
    } else if (mode == QLatin1String("Depth")) {
        setColorMode(Depth);
    } else if (mode == QLatin1String("Name")) {
        setColorMode(Name);
    } else if (mode == QLatin1String("Owner")) {
        setColorMode(Owner);
    } else if (mode == QLatin1String("Group")) {
        setColorMode(Group);
    } else if (mode == QLatin1String("Mime")) {
        setColorMode(Mime);
    } else {
        return false;
    }

    return true;
}

QString FSView::colorModeString() const
{
    QString mode;
    switch (_colorMode) {
    case None:  mode = QStringLiteral("None"); break;
    case Depth: mode = QStringLiteral("Depth"); break;
    case Name:  mode = QStringLiteral("Name"); break;
    case Owner: mode = QStringLiteral("Owner"); break;
    case Group: mode = QStringLiteral("Group"); break;
    case Mime:  mode = QStringLiteral("Mime"); break;
    default:    mode = QStringLiteral("Unknown"); break;
    }
    return mode;
}

void FSView::addColorItems(QMenu *popup, int id)
{
    _colorID = id;

    connect(popup, &QMenu::triggered, this, &FSView::colorActivated);

    addPopupItem(popup, i18n("None"),      colorMode() == None,  id++);
    addPopupItem(popup, i18n("Depth"),     colorMode() == Depth, id++);
    addPopupItem(popup, i18n("Name"),      colorMode() == Name,  id++);
    addPopupItem(popup, i18n("Owner"),     colorMode() == Owner, id++);
    addPopupItem(popup, i18n("Group"),     colorMode() == Group, id++);
    addPopupItem(popup, i18n("Mime Type"), colorMode() == Mime,  id++);
}

void FSView::colorActivated(QAction *a)
{
    const int id = a->data().toInt();
    if (id == _colorID) {
        setColorMode(None);
    } else if (id == _colorID + 1) {
        setColorMode(Depth);
    } else if (id == _colorID + 2) {
        setColorMode(Name);
    } else if (id == _colorID + 3) {
        setColorMode(Owner);
    } else if (id == _colorID + 4) {
        setColorMode(Group);
    } else if (id == _colorID + 5) {
        setColorMode(Mime);
    }
}

void FSView::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape && !_pressed && (selection().size() > 0)) {
        // For consistency with Dolphin, deselect all on Escape if we're not dragging.
        TreeMapItem *changed = selection().commonParent();
        if (changed) {
            clearSelection(changed);
        }
    } else {
        TreeMapWidget::keyPressEvent(e);
    }
}

void FSView::saveFSOptions()
{
    KConfigGroup tmconfig(_config, "TreeMap");
    saveOptions(&tmconfig);
    tmconfig.writeEntry("ColorMode", colorModeString());

    KConfigGroup gconfig(_config, "General");
    gconfig.writeEntry("Path", _path);

    KConfigGroup cconfig(_config, "MetricCache");
    saveMetric(&cconfig);
}

void FSView::quit()
{
    saveFSOptions();
    qApp->quit();
}

void FSView::doRedraw()
{
    // we update progress every 1/4 second, and redraw every second
    static int redrawCounter = 0;

    bool redo = _sm.scanRunning();
    if (!redo) {
        redrawCounter = 0;
    }

    if ((_progress > 0) && (_progressSize > 0) && _lastDir) {
        int percent = _progress * 100 / _progressSize;
        if (0) qCDebug(FSVIEWLOG) << "FSView::progress "
                                  << _progress << "/" << _progressSize
                                  << "= " << percent << "%, "
                                  << _dirsFinished << " dirs read, in "
                                  << _lastDir->path();
        emit progress(percent, _dirsFinished, _lastDir->path());
    }

    if (_allowRefresh && ((redrawCounter % 4) == 0)) {
        if (0) {
            qCDebug(FSVIEWLOG) << "doRedraw " << _sm.scanLength();
        }
        redraw();
    } else {
        redo = true;
    }

    if (redo) {
        QTimer::singleShot(500, this, SLOT(doRedraw()));
        redrawCounter++;
    }
}

void FSView::doUpdate()
{
    for (int i = 0; i < 5; i++) {
        switch (_progressPhase) {
        case 1:
            _chunkSize1 += _sm.scan(_chunkData1);
            if (_chunkSize1 > 100) {
                _progressPhase = 2;

                /* Go to maximally 33% by scaling with 3 */
                _progressSize = 3 * _chunkSize1;

                if (1) {
                    qCDebug(FSVIEWLOG) << "Phase 2: CSize " << _chunkSize1;
                }
            }
            break;

        case 2:
            /* progress phase 2 */
            _chunkSize2 += _sm.scan(_chunkData2);
            /* switch to Phase 3 if we reach 80 % of Phase 2 */
            if (_progress * 3 > _progressSize * 8 / 10) {
                _progressPhase = 3;

                /* Goal: Keep percentage equal from phase 2 to 3 */
                double percent = (double)_progress / _progressSize;
                /* We scale by factor 2/3 afterwards */
                percent = percent * 3 / 2;

                int todo = _chunkSize2 + (_progressSize / 3 - _progress);
                _progressSize = (int)((double)todo / (1.0 - percent));
                _progress = _progressSize - todo;

                /* Go to maximally 66% by scaling with 1.5 */
                _progressSize = _progressSize * 3 / 2;

                if (1) qCDebug(FSVIEWLOG) << "Phase 3: CSize " << _chunkSize2
                                          << ", Todo " << todo
                                          << ", Progress " << _progress
                                          << "/" << _progressSize;
            }
            break;

        case 3:
            /* progress phase 3 */
            _chunkSize3 += _sm.scan(_chunkData3);
            /* switch to Phase 4 if we reach 80 % of Phase 3 */
            if (_progress * 3 / 2 > _progressSize * 8 / 10) {
                _progressPhase = 4;

                /* Goal: Keep percentage equal from phase 2 to 3 */
                double percent = (double)_progress / _progressSize;
                int todo = _chunkSize3 + (_progressSize * 2 / 3 - _progress);
                _progressSize = (int)((double)todo / (1.0 - percent) + .5);
                _progress = _progressSize - todo;

                if (1) qCDebug(FSVIEWLOG) << "Phase 4: CSize " << _chunkSize3
                                          << ", Todo " << todo
                                          << ", Progress " << _progress
                                          << "/" << _progressSize;
            }

        default:
            _sm.scan(-1);
            break;
        }
    }

    if (_sm.scanRunning()) {
        QTimer::singleShot(0, this, SLOT(doUpdate()));
    } else {
        emit completed(_dirsFinished);
    }
}

