/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 David Faure <faure@kde.org>
    Copyright (C) 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

// Own
#include "konqfactory.h"

// std
#include <assert.h>

// Qt
#include <QWidget>
#include <QFile>
#include <QCoreApplication>

// KDE
#include <k4aboutdata.h>
#include "konqdebug.h"
#include <KLocalizedString>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kservicetypetrader.h>
#include <kdeversion.h>
#include <KParts/ReadOnlyPart>

// Local
#include "konqsettings.h"
#include "konqmainwindow.h"

KonqViewFactory::KonqViewFactory(const QString &libName, KLibFactory *factory)
    : m_libName(libName), m_factory(factory),
      m_args()
{
}

void KonqViewFactory::setArgs(const QVariantList &args)
{
    m_args = args;
}

KParts::ReadOnlyPart *KonqViewFactory::create(QWidget *parentWidget, QObject *parent)
{
    if (!m_factory) {
        return nullptr;
    }

    KParts::ReadOnlyPart *part = m_factory->create<KParts::ReadOnlyPart>(parentWidget, parent, QString(), m_args);

    if (!part) {
        qCWarning(KONQUEROR_LOG) << "No KParts::ReadOnlyPart created from" << m_libName;
    } else {
        QFrame *frame = qobject_cast<QFrame *>(part->widget());
        if (frame) {
            frame->setFrameStyle(QFrame::NoFrame);
        }
    }
    return part;
}

static KonqViewFactory tryLoadingService(KService::Ptr service)
{
    KPluginLoader pluginLoader(*service);
    pluginLoader.setLoadHints(QLibrary::ExportExternalSymbolsHint); // #110947
    KPluginFactory *factory = pluginLoader.factory();
    if (!factory) {
        KMessageBox::error(nullptr,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2",
                                service->name(), pluginLoader.errorString()));
        return KonqViewFactory();
    } else {
        return KonqViewFactory(service->library(), factory);
    }
}

KonqViewFactory KonqFactory::createView(const QString &serviceType,
                                        const QString &serviceName,
                                        KService::Ptr *serviceImpl,
                                        KService::List *partServiceOffers,
                                        KService::List *appServiceOffers,
                                        bool forceAutoEmbed)
{
    qCDebug(KONQUEROR_LOG) << "Trying to create view for" << serviceType << serviceName;

    // We need to get those in any case
    KService::List offers, appOffers;

    // Query the trader
    getOffers(serviceType, &offers, &appOffers);

    if (partServiceOffers) {
        (*partServiceOffers) = offers;
    }
    if (appServiceOffers) {
        (*appServiceOffers) = appOffers;
    }

    // We ask ourselves whether to do it or not only if no service was specified.
    // If it was (from the View menu or from RMB + Embedding service), just do it.
    forceAutoEmbed = forceAutoEmbed || !serviceName.isEmpty();
    // Or if we have no associated app anyway, then embed.
    forceAutoEmbed = forceAutoEmbed || (appOffers.isEmpty() && !offers.isEmpty());
    // Or if the associated app is konqueror itself, then embed.
    if (!appOffers.isEmpty()) {
        forceAutoEmbed = forceAutoEmbed || KonqMainWindow::isMimeTypeAssociatedWithSelf(serviceType, appOffers.first());
    }

    if (! forceAutoEmbed) {
        if (! KonqFMSettings::settings()->shouldEmbed(serviceType)) {
            qCDebug(KONQUEROR_LOG) << "KonqFMSettings says: don't embed this servicetype";
            return KonqViewFactory();
        }
    }

    KService::Ptr service;

    // Look for this service
    if (!serviceName.isEmpty()) {
        KService::List::const_iterator it = offers.constBegin();
        for (; it != offers.constEnd() && !service; ++it) {
            if ((*it)->desktopEntryName() == serviceName) {
                qCDebug(KONQUEROR_LOG) << "Found requested service" << serviceName;
                service = *it;
            }
        }
    }

    KonqViewFactory viewFactory;
    if (service) {
        qCDebug(KONQUEROR_LOG) << "Trying to open lib for requested service " << service->desktopEntryName();
        viewFactory = tryLoadingService(service);
        // If this fails, then return an error.
        // When looking for konq_sidebartng or konq_aboutpage, we don't want to end up
        // with khtml or another Browser/View part in case of an error...
    } else {
        KService::List::Iterator it = offers.begin();
        for (; viewFactory.isNull() /* exit as soon as we get one */ && it != offers.end(); ++it) {
            service = (*it);
            // Allowed as default ?
            QVariant prop = service->property(QStringLiteral("X-KDE-BrowserView-AllowAsDefault"));
            qCDebug(KONQUEROR_LOG) << service->desktopEntryName() << " : X-KDE-BrowserView-AllowAsDefault is valid : " << prop.isValid();
            if (!prop.isValid() || prop.toBool()) {   // defaults to true
                //qCDebug(KONQUEROR_LOG) << "Trying to open lib for service " << service->name();
                viewFactory = tryLoadingService(service);
                // If this works, we exit the loop.
            } else {
                qCDebug(KONQUEROR_LOG) << "Not allowed as default " << service->desktopEntryName();
            }
        }
    }

    if (serviceImpl) {
        (*serviceImpl) = service;
    }

    if (viewFactory.isNull()) {
        if (offers.isEmpty()) {
            qCWarning(KONQUEROR_LOG) << "no part was associated with" << serviceType;
        } else {
            qCWarning(KONQUEROR_LOG) << "no part could be loaded";    // full error was shown to user already
        }
        return viewFactory;
    }

    QVariantList args;
    const QVariant prop = service->property(QStringLiteral("X-KDE-BrowserView-Args"));
    if (prop.isValid()) {
        Q_FOREACH (const QString &str, prop.toString().split(' ')) {
            args << QVariant(str);
        }
    }

    if (service->serviceTypes().contains(QStringLiteral("Browser/View"))) {
        args << QLatin1String("Browser/View");
    }

    viewFactory.setArgs(args);
    return viewFactory;
}

void KonqFactory::getOffers(const QString &serviceType,
                            KService::List *partServiceOffers,
                            KService::List *appServiceOffers)
{
#ifdef __GNUC__
#warning Temporary hack -- must separate mimetypes and servicetypes better
#endif
    if (partServiceOffers && serviceType.length() > 0 && serviceType[0].isUpper()) {
        *partServiceOffers = KServiceTypeTrader::self()->query(serviceType,
                             QStringLiteral("DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'"));
        return;

    }
    if (appServiceOffers) {
        *appServiceOffers = KMimeTypeTrader::self()->query(serviceType, QStringLiteral("Application"),
                            QStringLiteral("DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'"));
    }

    if (partServiceOffers) {
        *partServiceOffers = KMimeTypeTrader::self()->query(serviceType, QStringLiteral("KParts/ReadOnlyPart"));
    }
}
