/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at),                 *
 *   Cvetoslav Ludmiloff and Patrice Tremblay                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "dolphinsettings.h"

#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include "dolphin_columnmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"

DolphinSettings& DolphinSettings::instance()
{
    static DolphinSettings* instance = 0;
    if (instance == 0) {
        instance = new DolphinSettings();
    }
    return *instance;
}

KBookmark DolphinSettings::bookmark(int index) const
{
    return bookmarkManager()->findByAddress(QString('/') + QString::number(index));
}

KBookmarkManager* DolphinSettings::bookmarkManager() const
{
    QString basePath = KGlobal::mainComponent().componentName();
    basePath.append("/bookmarks.xml");
    const QString file = KStandardDirs::locateLocal("data", basePath);

    return KBookmarkManager::managerForFile(file, "dolphin", false);
}

void DolphinSettings::save()
{
    m_generalSettings->writeConfig();
    m_iconsModeSettings->writeConfig();
    m_detailsModeSettings->writeConfig();
    m_columnModeSettings->writeConfig();

    QString basePath = KGlobal::mainComponent().componentName();
    basePath.append("/bookmarks.xml");
    const QString file = KStandardDirs::locateLocal( "data", basePath);

    KBookmarkManager* manager = KBookmarkManager::managerForFile(file, "dolphin", false);
    manager->save(false);
}

DolphinSettings::DolphinSettings()
{
    m_generalSettings = new GeneralSettings();
    m_iconsModeSettings = new IconsModeSettings();
    m_detailsModeSettings = new DetailsModeSettings();
    m_columnModeSettings = new ColumnModeSettings();
}

DolphinSettings::~DolphinSettings()
{
    delete m_generalSettings;
    m_generalSettings = 0;

    delete m_iconsModeSettings;
    m_iconsModeSettings = 0;

    delete m_detailsModeSettings;
    m_detailsModeSettings = 0;

    delete m_columnModeSettings;
    m_columnModeSettings = 0;
}
