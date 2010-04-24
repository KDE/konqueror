/* This file is part of the KDE project
   Copyright (C) 2000, 2010 David Faure <faure@kde.org>
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

#ifndef KBOOKMARKMODEL_COMMANDS_P_H
#define KBOOKMARKMODEL_COMMANDS_P_H

// Used internally by the "sort" command
class MoveCommand : public QUndoCommand, public IKEBCommand
{
public:
   MoveCommand(KBookmarkModel* model, const QString &from, const QString &to, const QString &name = QString(), QUndoCommand* parent = 0);
   QString finalAddress() const;
   virtual ~MoveCommand() {}
   virtual void redo();
   virtual void undo();
   virtual QString affectedBookmarks() const;
private:
   KBookmarkModel* m_model;
   QString m_from;
   QString m_to;
   CreateCommand * m_cc;
   DeleteCommand * m_dc;
};

#endif /* KBOOKMARKMODEL_COMMANDS_P_H */
