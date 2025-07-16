/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 1999 Torben Weis <weis@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqfactory.h"
#include <konq_kpart_plugin.h>
#include "konqdebug.h"
#include "konqembedsettings.h"
#include "konqmainwindow.h"
#include "konqutils.h"

// std
#include <assert.h>

// Qt
#include <QWidget>
#include <QFile>
#include <QCoreApplication>

// KDE
#include <KLocalizedString>
#include <KParts/ReadOnlyPart>
#include <KMessageBox>
#include <KApplicationTrader>
#include <KParts/PartLoader>

KonqViewFactory::KonqViewFactory(const KPluginMetaData &data, KPluginFactory *factory)
    : m_metaData(data), m_factory(factory), m_args()
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
    KParts::ReadOnlyPart *part = m_factory->create<KParts::ReadOnlyPart>(parentWidget, parent, m_args);

    if (!part) {
        qCWarning(KONQUEROR_LOG) << "No KParts::ReadOnlyPart created from" << m_metaData.name();
    } else {
        KonqParts::Plugin::loadPlugins(part, part, part->componentName());
        QFrame *frame = qobject_cast<QFrame *>(part->widget());
        if (frame) {
            frame->setFrameStyle(QFrame::NoFrame);
        }
    }
    return part;
}

static KonqViewFactory tryLoadingService(const KPluginMetaData &data)
{
    if (auto factoryResult = KPluginFactory::loadFactory(data)) {
        return KonqViewFactory(data, factoryResult.plugin);
    } else {
        KMessageBox::error(nullptr,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2",
                                data.name(), factoryResult.errorString));
        return KonqViewFactory();
    }
}

//TODO port away from query: check whether this output type arguments can be replaced by something else
KonqViewFactory KonqFactory::createView(const Konq::ViewType &type,
                                        const QString &serviceName,
                                        KPluginMetaData *serviceImpl,
                                        QVector<KPluginMetaData> *partServiceOffers,
                                        KService::List *appServiceOffers,
                                        bool forceAutoEmbed)
{
    //TODO port away from query: check whether service name can be empty
    qCDebug(KONQUEROR_LOG) << "Trying to create view for" << type << serviceName;

    // We need to get those in any case
    QVector<KPluginMetaData> offers;
    KService::List appOffers;

    // Query the plugins
    getOffers(type, &offers, &appOffers);

    if (partServiceOffers) {
        (*partServiceOffers) = offers;
    }
    if (appServiceOffers) {
        (*appServiceOffers) = appOffers;
    }

    const QString mimetype = type.isMimetype() ? type.mimetype().value() : QString();

    // We ask ourselves whether to do it or not only if no service was specified.
    // If it was (from the View menu or from RMB + Embedding service), just do it.
    forceAutoEmbed = forceAutoEmbed || !serviceName.isEmpty();
    // Or if we have no associated app anyway, then embed.
    forceAutoEmbed = forceAutoEmbed || (appOffers.isEmpty() && !offers.isEmpty());
    // Or if the associated app is konqueror itself, then embed.
    if (!appOffers.isEmpty()) {
        forceAutoEmbed = forceAutoEmbed || KonqMainWindow::isMimeTypeAssociatedWithSelf(mimetype, appOffers.first());
    }

    if (! forceAutoEmbed) {
        if (! KonqFMSettings::settings()->shouldEmbed(mimetype)) {
            qCDebug(KONQUEROR_LOG) << "KonqFMSettings says: don't embed this servicetype";
            return KonqViewFactory();
        }
    }

    KPluginMetaData service;

    // Look for this service
    if (!serviceName.isEmpty()) {
        service = findPartById(serviceName);
    }

    KonqViewFactory viewFactory;

    if (service.isValid()) {
        qCDebug(KONQUEROR_LOG) << "Trying to open lib for requested service " << service.name();
        viewFactory = tryLoadingService(service);
        // If this fails, then return an error.
        // When looking for konq_sidebartng or konq_aboutpage, we don't want to end up
        // with khtml or another Browser/View part in case of an error...
    } else {
        for (const KPluginMetaData &md : offers) {
            // Allowed as default? Assume it is, unless explicitly stated otherwise
            if (md.value(QStringLiteral("X-KDE-BrowserView-AllowAsDefault"), true)) {
                viewFactory = tryLoadingService(md);
                if (!viewFactory.isNull()) {
                    service = md;
                    break;
                }
            } else {
                qCDebug(KONQUEROR_LOG) << "Not allowed as default " << md.name();
            }
        }
    }

    if (serviceImpl) {
        (*serviceImpl) = service;
    }

    if (viewFactory.isNull()) {
        if (offers.isEmpty()) {
            qCWarning(KONQUEROR_LOG) << "no part was associated with" << type;
        } else {
            qCWarning(KONQUEROR_LOG) << "no part could be loaded";    // full error was shown to user already
        }
        return viewFactory;
    }

    QVariantList args;
    for (const QString &str : service.value(QStringLiteral("X-KDE-BrowserView-Args"), QStringList())) {
        args << QVariant(str);
    }

    if (Konq::partCapabilities(service) & KParts::PartCapability::BrowserView) {
        args << QLatin1String("Browser/View");
    }

    viewFactory.setArgs(args);
    return viewFactory;
}

void KonqFactory::getOffers(const Konq::ViewType &type, QVector<KPluginMetaData> *partServiceOffers, KService::List *appServiceOffers)
{
    if (type.isCapability()) {
        if (partServiceOffers) {
            //TODO port away from query: check whether it's still necessary to exclude kfmclient* from this vector (they aren't parts, so I think they shouldn't be included here)
            auto filter = [type](const KPluginMetaData &md){return Konq::partCapabilities(md) & type.capability().value();};
            *partServiceOffers = findParts(filter);
            //Some parts, in particular konsolepart (at least version 22.12.3), aren't installed if kf5/parts but in the plugins directory, so we need to look for them separately
            //TODO: remove this line when (if) all parts are installed in kf5/parts
            partServiceOffers->append(KPluginMetaData::findPlugins(QString(), filter));
        }
        return;
    }

    if (partServiceOffers ) {
        const QVector<KPluginMetaData> offers = KParts::PartLoader::partsForMimeType(type.mimetype().value());
        *partServiceOffers = offers;
    }
    if (appServiceOffers) {
        *appServiceOffers = KApplicationTrader::queryByMimeType(type.mimetype().value(), [](const KService::Ptr &s){return !s->desktopEntryName().startsWith("kfmclient");});
    }
}
