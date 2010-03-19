// kate: space-indent on; indent-width 3; replace-tabs on;
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
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

#ifndef __commands_h
#define __commands_h

#include <QtGui/QUndoCommand>
#include <kbookmark.h>
#include <QtCore/QMap>


// Interface adds the affectedBookmarks method
// Any class should on call add those bookmarks which are
// affected by executing or unexecuting the command
// Or a common parent of the affected bookmarks
// see KBookmarkManager::notifyChange(KBookmarkGroup)
class IKEBCommand
{
public:
   IKEBCommand() {}
   virtual ~IKEBCommand() {}
   virtual QString affectedBookmarks() const = 0;
   // If only QUndoCommand had setData(QVariant), we would save a lot of casting...
};

class KEBMacroCommand : public QUndoCommand, public IKEBCommand
{
public:
   KEBMacroCommand(const QString &name, QUndoCommand* parent = 0)
      : QUndoCommand(name, parent) {}
   virtual ~KEBMacroCommand() {}
   virtual QString affectedBookmarks() const;
};

class DeleteManyCommand : public KEBMacroCommand
{
public:
   DeleteManyCommand(const QString &name, const QList<KBookmark> & bookmarks);
   virtual ~DeleteManyCommand() {}
};

class CreateCommand : public QUndoCommand, public IKEBCommand
{
public:
   // separator
   CreateCommand(const QString &address, QUndoCommand* parent = 0);

   // bookmark
   CreateCommand(const QString &address,
                 const QString &text, const QString &iconPath,
                 const KUrl &url, QUndoCommand* parent = 0);

   // folder
   CreateCommand(const QString &address,
                 const QString &text, const QString &iconPath,
                 bool open, QUndoCommand* parent = 0);

   // clone existing bookmark
   CreateCommand(const QString &address,
                 const KBookmark &original, const QString &name = QString(), QUndoCommand* parent = 0);

   QString finalAddress() const;

   virtual ~CreateCommand() {}
   virtual void redo();
   virtual void undo();
   virtual QString affectedBookmarks() const;
private:
   QString m_to;
   QString m_text;
   QString m_iconPath;
   KUrl m_url;
   bool m_group:1;
   bool m_separator:1;
   bool m_open:1;
   KBookmark m_originalBookmark;
   QDomDocument m_originalBookmarkDocRef; // so that it lives at least as long as m_originalBookmark
};

class EditCommand : public QUndoCommand, public IKEBCommand
{
public:
   EditCommand(const QString & address, int col, const QString & newValue, QUndoCommand* parent = 0);
   virtual ~EditCommand() {}
   virtual void redo();
   virtual void undo();
   virtual QString affectedBookmarks() const { return KBookmark::parentAddress(mAddress); }
   static QString getNodeText(const KBookmark& bk, const QStringList &nodehier);
   static QString setNodeText(const KBookmark& bk, const QStringList &nodehier,
                                     const QString& newValue);
   void modify(const QString &newValue);
private:
   QString mAddress;
   int mCol;
   QString mNewValue;
   QString mOldValue;
};

class DeleteCommand : public QUndoCommand, public IKEBCommand
{
public:
    explicit DeleteCommand(const QString &from, bool contentOnly = false, QUndoCommand* parent = 0)
      : QUndoCommand(parent), m_from(from), m_cmd(0), m_subCmd(0), m_contentOnly(contentOnly)
   {
      // NOTE - DeleteCommand needs no text, it is always embedded in a macrocommand
   }
   virtual ~DeleteCommand() { delete m_cmd; delete m_subCmd; }
   virtual void redo();
   virtual void undo();
   virtual QString affectedBookmarks() const;
   static KEBMacroCommand* deleteAll(const KBookmarkGroup &parentGroup);
private:
   QString m_from;
   QUndoCommand *m_cmd;
   KEBMacroCommand *m_subCmd;
   bool m_contentOnly;
};

class MoveCommand : public QUndoCommand, public IKEBCommand
{
public:
   MoveCommand(const QString &from, const QString &to, const QString &name = QString(), QUndoCommand* parent = 0);
   QString finalAddress() const;
   virtual ~MoveCommand() {}
   virtual void redo();
   virtual void undo();
   virtual QString affectedBookmarks() const;
private:
   QString m_from;
   QString m_to;
   CreateCommand * m_cc;
   DeleteCommand * m_dc;
};

class SortItem;

class SortCommand : public KEBMacroCommand
{
public:
   SortCommand(const QString &name, const QString &groupAddress, QUndoCommand* parent = 0)
      : KEBMacroCommand(name, parent), m_groupAddress(groupAddress)
   {}
   virtual ~SortCommand()
   {}
   virtual void redo();
   virtual void undo();
   virtual QString affectedBookmarks() const;
   // internal
   void moveAfter(const SortItem &moveMe, const SortItem &afterMe);
private:
   QString m_groupAddress;
};

class KEBListViewItem;

class CmdGen {
public:
   static KEBMacroCommand* setAsToolbar(const KBookmark &bk);
   static KEBMacroCommand* deleteItems(const QString &commandName, const QMap<KEBListViewItem *, bool> & items);
   static KEBMacroCommand* insertMimeSource(const QString &cmdName, const QMimeData *data, const QString &addr);
   static KEBMacroCommand* itemsMoved(const QList<KBookmark> & items, const QString &newAddress, bool copy);
private:
   CmdGen() {}
};

#endif
