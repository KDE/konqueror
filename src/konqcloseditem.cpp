/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqcloseditem.h"
#include "konqclosedwindowsmanager.h"
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <kconfig.h>
#include "konqdebug.h"
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

Q_GLOBAL_STATIC(KonqIcon, s_lightIconImage)

KonqClosedItem::KonqClosedItem(const QString &title, KConfig *config, const QString &group, quint64 serialNumber)
    : m_title(title), m_configGroup(config, group), m_serialNumber(serialNumber)
{
}

KonqClosedItem::~KonqClosedItem()
{
    //qCDebug(KONQUEROR_LOG) << "deleting group" << m_configGroup.name();
    m_configGroup.deleteGroup();
}

KonqClosedTabItem::KonqClosedTabItem(const QString &url, KConfig *config, const QString &title, int pos, quint64 serialNumber)
    :  KonqClosedItem(title, config, "Closed_Tab" + QString::number(reinterpret_cast<qint64>(this)), serialNumber),  m_url(url), m_pos(pos)
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

KonqClosedWindowItem::KonqClosedWindowItem(const QString &title, KConfig *config, quint64 serialNumber, int numTabs)
    :  KonqClosedItem(title, config, "Closed_Window" + QString::number(reinterpret_cast<qint64>(this)), serialNumber), m_numTabs(numTabs)
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
    int w = fm.boundingRect(countStr).width();
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

KonqClosedRemoteWindowItem::KonqClosedRemoteWindowItem(const QString &title, KConfig *config,
        const QString &groupName, const QString &configFileName, quint64 serialNumber,
        int numTabs, const QString &dbusService)
    : KonqClosedWindowItem(title, config, serialNumber, numTabs),
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
