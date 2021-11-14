/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 1999 Torben Weis <weis@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "konqfactory.h"
#include <konq_kpart_plugin.h>

// std
#include <assert.h>

// Qt
#include <QWidget>
#include <QFile>
#include <QCoreApplication>

// KDE
#include "konqdebug.h"
#include <KLocalizedString>
#include <KParts/ReadOnlyPart>
#include <KPluginInfo>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kservicetypetrader.h>

// Local
#include "konqsettings.h"
#include "konqmainwindow.h"

KonqViewFactory::KonqViewFactory(const QString &libName, KPluginFactory *factory)
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
        KonqParts::Plugin::loadPlugins(part, part, part->componentName());
        QFrame *frame = qobject_cast<QFrame *>(part->widget());
        if (frame) {
            frame->setFrameStyle(QFrame::NoFrame);
        }
    }
    return part;
}

static KonqViewFactory tryLoadingService(KService::Ptr service)
{
    if (auto factoryResult = KPluginFactory::loadFactory(KPluginInfo(service).toMetaData())) {
        return KonqViewFactory(service->library(), factoryResult.plugin);
    } else {
        KMessageBox::error(nullptr,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2",
                                service->name(), factoryResult.errorString));
        return KonqViewFactory();
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
        auto it = std::find_if(offers.constBegin(), offers.constEnd(), [serviceName](KService::Ptr s){return s->desktopEntryName() == serviceName;});
        qCDebug(KONQUEROR_LOG) << "Found requested service" << serviceName;
        service = *it;
    }

    KonqViewFactory viewFactory;

    if (service) {
        qCDebug(KONQUEROR_LOG) << "Trying to open lib for requested service " << service->desktopEntryName();
        viewFactory = tryLoadingService(service);
        // If this fails, then return an error.
        // When looking for konq_sidebartng or konq_aboutpage, we don't want to end up
        // with khtml or another Browser/View part in case of an error...
    } else {
        for (KService::Ptr offer : offers) {
            // Allowed as default ?
            QVariant prop = offer ->property(QStringLiteral("X-KDE-BrowserView-AllowAsDefault"));
            qCDebug(KONQUEROR_LOG) << offer ->desktopEntryName() << " : X-KDE-BrowserView-AllowAsDefault is valid : " << prop.isValid();
            if (!prop.isValid() || prop.toBool()) {   // defaults to true
                //qCDebug(KONQUEROR_LOG) << "Trying to open lib for service " << service->name();
                viewFactory = tryLoadingService(offer);
                if (!viewFactory.isNull()) {
                    service = offer;
                    break;
                }
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
