/* This file is part of Webarchiver

    SPDX-FileCopyrightText: 2001 Andreas Schlapbach <schlpbch@iam.unibe.ch>
    SPDX-FileCopyrightText: 2020 Jonathan Marten <jjm@keelhaul.me.uk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this program; see the file COPYING.  If not, see
    <http://www.gnu.org/licenses/>.
*/

#include "plugin_webarchiver.h"

#include <qicon.h>
#include <qurl.h>
#include <qstandardpaths.h>
#include <qprocess.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kactioncollection.h>
#include <kmessagebox.h>

#include <kparts/readonlypart.h>

#include "webarchiverdebug.h"


K_PLUGIN_FACTORY(PluginWebArchiverFactory, registerPlugin<PluginWebArchiver>();)


PluginWebArchiver::PluginWebArchiver(QObject *parent, const QVariantList &args)
    : Plugin(parent)
{
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
