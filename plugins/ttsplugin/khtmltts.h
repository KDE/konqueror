/***************************************************************************
  Copyright (C) 2002 by George Russell <george.russell@clara.net>
  Copyright (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
  Copyright (C) 2015 by Jeremy Whiting <jpwhiting@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KHTMLTTS_H
#define KHTMLTTS_H

#include <kparts/plugin.h>

/**
 * KHTML KParts Plugin
 */
class KHTMLPluginTTS : public KParts::Plugin
{
    Q_OBJECT
public:

    /**
     * Construct a new KParts plugin.
     */
    KHTMLPluginTTS(QObject *parent, const QVariantList &);

    /**
     * Destructor.
     */
    ~KHTMLPluginTTS() override;
public Q_SLOTS:
    void slotReadOut();
};

#endif
