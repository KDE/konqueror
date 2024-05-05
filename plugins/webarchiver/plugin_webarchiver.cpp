/* This file is part of Webarchiver

    SPDX-FileCopyrightText: 2001 Andreas Schlapbach <schlpbch@iam.unibe.ch>
    SPDX-FileCopyrightText: 2020 Jonathan Marten <jjm@keelhaul.me.uk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "plugin_webarchiver.h"

#include <QIcon>
#include <QUrl>
#include <QStandardPaths>
#include <QProcess>

#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kactioncollection.h>
#include <kmessagebox.h>

#include <kparts/readonlypart.h>

#include "webarchiverdebug.h"

K_PLUGIN_CLASS_WITH_JSON(PluginWebArchiver, "plugin_webarchiver.json")

PluginWebArchiver::PluginWebArchiver(QObject *parent, const QVariantList &args)
    : KonqParts::Plugin(parent)
{
    Q_UNUSED(args);
    QAction *a = actionCollection()->addAction(QStringLiteral("archivepage"));
    a->setText(i18n("Archive Web Page..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("webarchiver")));
    connect(a, &QAction::triggered, this, &PluginWebArchiver::slotSaveToArchive);
}


void PluginWebArchiver::slotSaveToArchive()
{
    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent());
    if (part==nullptr) return;

    const QUrl pageUrl = part->url();
    if (!pageUrl.isValid()) return;

    const QString helper = QStandardPaths::findExecutable("kcreatewebarchive");
    if (helper.isEmpty())
    {
        KMessageBox::error(part->widget(),
                           xi18nc("@info", "Cannot find the <command>kcreatewebarchive</command> executable,<nl/>check the plugin and helper installation."),
                           i18n("Cannot Create Web Archive"));
        return;
    }

    qCDebug(WEBARCHIVERPLUGIN_LOG) << "Executing" << helper;
    QProcess::startDetached(helper, (QStringList() << pageUrl.url()));
}


#include "plugin_webarchiver.moc"
