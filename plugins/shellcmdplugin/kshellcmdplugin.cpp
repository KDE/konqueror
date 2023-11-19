/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kshellcmdplugin.h"
#include "kshellcmddialog.h"
#include <kparts/part.h>
#include <kactioncollection.h>
#include <KLocalizedString>
#include <kmessagebox.h>
#include <kshell.h>
#include <kpluginfactory.h>
#include <kauthorized.h>
#include <kparts/fileinfoextension.h>
#include <KParts/ReadOnlyPart>
#include <kio/statjob.h>

#include <QAction>
#include <QInputDialog>

KShellCmdPlugin::KShellCmdPlugin(QObject *parent, const QVariantList &)
    : KonqParts::Plugin(parent)
{
    if (!KAuthorized::authorize(QStringLiteral("shell_access"))) {
        return;
    }

    QAction *action = actionCollection()->addAction(QStringLiteral("executeshellcommand"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
    action->setText(i18n("&Execute Shell Command..."));
    connect(action, &QAction::triggered, this, &KShellCmdPlugin::slotExecuteShellCommand);
    actionCollection()->setDefaultShortcut(action, QKeySequence(Qt::CTRL | Qt::Key_E));
}

void KShellCmdPlugin::slotExecuteShellCommand()
{
    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent());
    if (!part)  {
        KMessageBox::error(nullptr, i18n("KShellCmdPlugin::slotExecuteShellCommand: Program error, please report a bug."));
        return;
    }

    QUrl url;
    KIO::StatJob *statJob = KIO::mostLocalUrl(part->url());
    if (statJob->exec()) {
        url = statJob->mostLocalUrl();
    }
    if (!url.isLocalFile()) {
        KMessageBox::error(part->widget(), i18n("Executing shell commands works only on local directories."));
        return;
    }

    QString path;
    KParts::FileInfoExtension *ext = KParts::FileInfoExtension::childObject(part);

    if (ext && ext->hasSelection() && (ext->supportedQueryModes() & KParts::FileInfoExtension::SelectedItems)) {
        KFileItemList list = ext->queryFor(KParts::FileInfoExtension::SelectedItems);
        QStringList fileNames;
        for (const KFileItem &item: list) {
            fileNames << item.name();
        }
        path = KShell::joinArgs(fileNames);
    }

    if (path.isEmpty()) {
        path = KShell::quoteArg(url.toLocalFile());
    }

    bool ok;
    QString cmd = QInputDialog::getText(part->widget(),
                                        i18nc("@title:window", "Execute Shell Command"),
                                        i18n("Execute shell command in current directory:"),
                                        QLineEdit::Normal,
                                        path,
                                        &ok);
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

K_PLUGIN_CLASS_WITH_JSON(KShellCmdPlugin, "kshellcmdplugin.json")

#include "kshellcmdplugin.moc"

