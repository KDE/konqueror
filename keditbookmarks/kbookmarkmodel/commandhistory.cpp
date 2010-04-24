/* This file is part of the KDE project
   Copyright (C) 2000, 2009 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "commandhistory.h"
#include "commands.h"

#include <kactioncollection.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <kundostack.h>

#include <QAction>
#include <QUndoCommand>

class CommandHistory::Private
{
public:
    Private() : m_manager(0), m_commandHistory() {}
    KBookmarkManager* m_manager;
    KUndoStack m_commandHistory;
};

CommandHistory::CommandHistory(QObject* parent)
    : QObject(parent), d(new CommandHistory::Private)
{
}

CommandHistory::~CommandHistory()
{
    delete d;
}

void CommandHistory::setBookmarkManager(KBookmarkManager* manager)
{
    clearHistory(); // we can't keep old commands pointing to the wrong model/manager...
    d->m_manager = manager;
}

void CommandHistory::createActions(KActionCollection *actionCollection)
{
    // TODO use QUndoView?
    QAction* undoAction = d->m_commandHistory.createUndoAction(actionCollection);
    connect(undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    QAction* redoAction = d->m_commandHistory.createRedoAction(actionCollection);
    connect(redoAction, SIGNAL(triggered()), this, SLOT(redo()));
}

void CommandHistory::undo()
{
    const int idx = d->m_commandHistory.index();
    const QUndoCommand* cmd = d->m_commandHistory.command(idx-1);
    if (cmd) {
        d->m_commandHistory.undo();
        commandExecuted(cmd);
    }
}

void CommandHistory::redo()
{
    const int idx = d->m_commandHistory.index();
    const QUndoCommand* cmd = d->m_commandHistory.command(idx);
    if (cmd) {
        d->m_commandHistory.redo();
        commandExecuted(cmd);
    }
}

void CommandHistory::commandExecuted(const QUndoCommand *k)
{
    const IKEBCommand * cmd = dynamic_cast<const IKEBCommand *>(k);
    Q_ASSERT(cmd);

    KBookmark bk = d->m_manager->findByAddress(cmd->affectedBookmarks());
    Q_ASSERT(bk.isGroup());

    emit notifyCommandExecuted(bk.toGroup());
}

void CommandHistory::notifyDocSaved()
{
    d->m_commandHistory.setClean();
}

void CommandHistory::addCommand(QUndoCommand *cmd)
{
    if (!cmd)
        return;
    d->m_commandHistory.push(cmd); // calls cmd->redo()
    CommandHistory::commandExecuted(cmd);
}

void CommandHistory::clearHistory()
{
    if (d->m_commandHistory.count() > 0) {
        d->m_commandHistory.clear();
        emit notifyCommandExecuted(d->m_manager->root()); // not really, but we still want to update the GUI
    }
}

KBookmarkManager* CommandHistory::bookmarkManager()
{
    return d->m_manager;
}

#include "commandhistory.moc"
