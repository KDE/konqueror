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
#include <kdebug.h>
#include "commands.h"
#include "globalbookmarkmanager.h"
#include "toplevel.h" // for KEBApp
#include <kactioncollection.h>
#include <QUndoCommand>

CmdHistory* CmdHistory::s_self = 0;

CmdHistory::CmdHistory(KActionCollection *actionCollection)
    : m_commandHistory()
{
    Q_ASSERT(!s_self);
    s_self = this; // this is hacky

    // TODO use QUndoView?
    QAction* undoAction = m_commandHistory.createUndoAction(actionCollection);
    connect(undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    QAction* redoAction = m_commandHistory.createRedoAction(actionCollection);
    connect(redoAction, SIGNAL(triggered()), this, SLOT(redo()));
}

CmdHistory* CmdHistory::self()
{
    Q_ASSERT(s_self);
    return s_self;
}

void CmdHistory::undo()
{
    const int idx = m_commandHistory.index();
    const QUndoCommand* cmd = m_commandHistory.command(idx-1);
    if (cmd) {
        m_commandHistory.undo();
        commandExecuted(cmd);
    }
}

void CmdHistory::redo()
{
    const int idx = m_commandHistory.index();
    const QUndoCommand* cmd = m_commandHistory.command(idx);
    if (cmd) {
        m_commandHistory.redo();
        commandExecuted(cmd);
    }
}

void CmdHistory::commandExecuted(const QUndoCommand *k)
{
    KEBApp::self()->notifyCommandExecuted();

    const IKEBCommand * cmd = dynamic_cast<const IKEBCommand *>(k);
    Q_ASSERT(cmd);

    KBookmark bk = GlobalBookmarkManager::bookmarkAt(cmd->affectedBookmarks());
    Q_ASSERT(bk.isGroup());
    GlobalBookmarkManager::self()->notifyManagers(bk.toGroup());
}

void CmdHistory::notifyDocSaved()
{
    m_commandHistory.setClean();
}

void CmdHistory::addCommand(QUndoCommand *cmd)
{
    if (!cmd)
        return;
    m_commandHistory.push(cmd); // calls cmd->redo()
    CmdHistory::commandExecuted(cmd);
}

void CmdHistory::addInFlightCommand(QUndoCommand *cmd)
{
    if(!cmd)
        return;
    m_commandHistory.push(cmd); // TODO: HOW TO NOT CALL REDO?
}

void CmdHistory::clearHistory()
{
    m_commandHistory.clear();
}

#include "commandhistory.moc"
