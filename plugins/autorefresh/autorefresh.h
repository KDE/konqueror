// -*- c++ -*-

/*
    SPDX-FileCopyrightText: 2003 Richard J. Moore <rich@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __plugin_autorefresh_h
#define __plugin_autorefresh_h

#include <QVariant>
#include <konq_kpart_plugin.h>

class QTimer;
class KSelectAction;

/**
 * A plugin is the way to add actions to an existing @ref KParts application,
 * or to a @ref Part.
 *
 * The XML of those plugins looks exactly like of the shell or parts,
 * with one small difference: The document tag should have an additional
 * attribute, named "library", and contain the name of the library implementing
 * the plugin.
 *
 * If you want this plugin to be used by a part, you need to
 * install the rc file under the directory
 * "data" (KDEDIR/share/apps usually)+"/instancename/kpartplugins/"
 * where instancename is the name of the part's instance.
 **/
class AutoRefresh : public KonqParts::Plugin
{
    Q_OBJECT
public:

    /**
     * Construct a new KParts plugin.
     */
    explicit AutoRefresh(QObject *parent = nullptr, const QVariantList &args = QVariantList());

    /**
     * Destructor.
     */
    ~AutoRefresh() override;

public slots:
    void slotRefresh();
    void slotIntervalChanged();

private:
    KSelectAction *refresher;
    QTimer *timer;
};

#endif
