/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kshellcmdplugin.h"
#include "kshellcmddialog.h"
#include <kparts/part.h>
#include <kactioncollection.h>
#include <kinputdialog.h>
#include <KLocalizedString>
#include <kmessagebox.h>
#include <kshell.h>
#include <kpluginfactory.h>
#include <kauthorized.h>
#include <kio/netaccess.h>
#include <kparts/fileinfoextension.h>
#include <KParts/ReadOnlyPart>
#include <QAction>

KShellCmdPlugin::KShellCmdPlugin(QObject *parent, const QVariantList &)
    : KParts::Plugin(parent)
{
    if (!KAuthorized::authorize(QStringLiteral("shell_access"))) {
        return;
    }

    QAction *action = actionCollection()->addAction(QStringLiteral("executeshellcommand"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
    action->setText(i18n("&Execute Shell Command..."));
    connect(action, &QAction::triggered, this, &KShellCmdPlugin::slotExecuteShellCommand);
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_E));
}

void KShellCmdPlugin::slotExecuteShellCommand()
{
    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent());
    if (!part)  {
        KMessageBox::sorry(nullptr, i18n("KShellCmdPlugin::slotExecuteShellCommand: Program error, please report a bug."));
        return;
    }

    QUrl url = KIO::NetAccess::mostLocalUrl(part->url(), nullptr);
    if (!url.isLocalFile()) {
        KMessageBox::sorry(part->widget(), i18n("Executing shell commands works only on local directories."));
        return;
    }

    QString path;
    KParts::FileInfoExtension *ext = KParts::FileInfoExtension::childObject(part);

    if (ext && ext->hasSelection() && (ext->supportedQueryModes() & KParts::FileInfoExtension::SelectedItems)) {
        KFileItemList list = ext->queryFor(KParts::FileInfoExtension::SelectedItems);
        QStringList fileNames;
        Q_FOREACH (const KFileItem &item, list) {
            fileNames << item.name();
        }
        path = KShell::joinArgs(fileNames);
    }

    if (path.isEmpty()) {
        path = KShell::quoteArg(url.toLocalFile());
    }

    bool ok;
    QString cmd = KInputDialog::getText(i18nc("@title:window", "Execute Shell Command"),
                                        i18n("Execute shell command in current directory:"),
                                        path, &ok, part->widget());
    if (ok) {
        QString exeCmd;
        exeCmd = QStringLiteral("cd ");
        exeCmd += KShell::quoteArg(part->url().path());
        exeCmd += QLatin1String("; ");
        exeCmd += cmd;

        KShellCommandDialog *dlg = new KShellCommandDialog(i18n("Output from command: \"%1\"", cmd), exeCmd, part->widget(), true);
        dlg->resize(500, 300);
        dlg->executeCommand();
        delete dlg;
    }
}

K_PLUGIN_FACTORY(KonqShellCmdPluginFactory, registerPlugin<KShellCmdPlugin>();)

#include "kshellcmdplugin.moc"

