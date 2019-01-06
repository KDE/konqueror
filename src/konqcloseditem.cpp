/* This file is part of the KDE project
   Copyright 2007 David Faure <faure@kde.org>
   Copyright 2007 Eduardo Robles Elvira <edulix@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqcloseditem.h"
#include "konqclosedwindowsmanager.h"
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <kconfig.h>
#include "konqdebug.h"
#include <kglobal.h>
#include <QIcon>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <konqpixmapprovider.h>
#include <kcolorscheme.h>

#include <QFontDatabase>

class KonqIcon
{
public:
    KonqIcon()
        : image(QIcon::fromTheme(QStringLiteral("konqueror")).pixmap(16).toImage())
    {
        KIconEffect::deSaturate(image, 0.60f);
    }

    QImage image;
};

K_GLOBAL_STATIC(KonqIcon, s_lightIconImage)

KonqClosedItem::KonqClosedItem(const QString &title, const QString &group, quint64 serialNumber)
    : m_title(title), m_configGroup(KonqClosedWindowsManager::self()->memoryStore(), group), m_serialNumber(serialNumber)
{
}

KonqClosedItem::~KonqClosedItem()
{
    //qCDebug(KONQUEROR_LOG) << "deleting group" << m_configGroup.name();
    m_configGroup.deleteGroup();
}

KonqClosedTabItem::KonqClosedTabItem(const QString &url, const QString &title, int pos, quint64 serialNumber)
    :  KonqClosedItem(title, "Closed_Tab" + QString::number(reinterpret_cast<qint64>(this)), serialNumber),  m_url(url), m_pos(pos)
{
    qCDebug(KONQUEROR_LOG) << m_configGroup.name();
}

KonqClosedTabItem::~KonqClosedTabItem()
{
    m_configGroup.deleteGroup();
    qCDebug(KONQUEROR_LOG) << "deleted group" << m_configGroup.name();
}

QPixmap KonqClosedTabItem::icon() const
{
    return KonqPixmapProvider::self()->pixmapFor(m_url, KIconLoader::SizeSmall);
}

KonqClosedWindowItem::KonqClosedWindowItem(const QString &title, quint64 serialNumber, int numTabs)
    :  KonqClosedItem(title, "Closed_Window" + QString::number(reinterpret_cast<qint64>(this)), serialNumber), m_numTabs(numTabs)
{
    qCDebug(KONQUEROR_LOG) << m_configGroup.name();
}

KonqClosedWindowItem::~KonqClosedWindowItem()
{
}

QPixmap KonqClosedWindowItem::icon() const
{
    QImage overlayImg = s_lightIconImage->image.copy();
    int oldWidth = overlayImg.width();
    QString countStr = QString::number(m_numTabs);

    QFont f = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    f.setBold(true);

    float pointSize = f.pointSizeF();
    QFontMetrics fm(f);
    int w = fm.width(countStr);
    if (w > (oldWidth)) {
        pointSize *= float(oldWidth) / float(w);
        f.setPointSizeF(pointSize);
    }

    // overlay
    QPainter p(&overlayImg);
    p.setFont(f);
    KColorScheme scheme(QPalette::Active, KColorScheme::Window);
    p.setPen(scheme.foreground(KColorScheme::LinkText).color());
    p.drawText(overlayImg.rect(), Qt::AlignCenter, countStr);

    return QPixmap::fromImage(overlayImg);
}

int KonqClosedWindowItem::numTabs() const
{
    return m_numTabs;
}

KonqClosedRemoteWindowItem::KonqClosedRemoteWindowItem(const QString &title,
        const QString &groupName, const QString &configFileName, quint64 serialNumber,
        int numTabs, const QString &dbusService)
    : KonqClosedWindowItem(title, serialNumber, numTabs),
      m_remoteGroupName(groupName), m_remoteConfigFileName(configFileName),
      m_dbusService(dbusService), m_remoteConfigGroup(nullptr), m_remoteConfig(nullptr)
{
    qCDebug(KONQUEROR_LOG);
}

KonqClosedRemoteWindowItem::~KonqClosedRemoteWindowItem()
{
    delete m_remoteConfigGroup;
    delete m_remoteConfig;
}

void KonqClosedRemoteWindowItem::readConfig() const
{
    // only do this once
    if (m_remoteConfig || m_remoteConfigGroup) {
        return;
    }

    m_remoteConfig = new KConfig(m_remoteConfigFileName, KConfig::SimpleConfig);
    m_remoteConfigGroup = new KConfigGroup(m_remoteConfig, m_remoteGroupName);
    qCDebug(KONQUEROR_LOG);
}

const KConfigGroup &KonqClosedRemoteWindowItem::configGroup() const
{
    readConfig();
    return *m_remoteConfigGroup;
}

KConfigGroup &KonqClosedRemoteWindowItem::configGroup()
{
    readConfig();
    return *m_remoteConfigGroup;
}

bool KonqClosedRemoteWindowItem::equalsTo(const QString &groupName,
        const QString &configFileName) const
{
    return (m_remoteGroupName == groupName &&
            m_remoteConfigFileName == configFileName);
}
