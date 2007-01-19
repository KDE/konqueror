/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2006 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_UNDO_P_H
#define KONQ_UNDO_P_H

#include <qstack.h>

struct KonqBasicOperation
{
  typedef QStack<KonqBasicOperation> Stack;

  KonqBasicOperation()
  { m_valid = false; }

  bool m_valid;
  bool m_directory;
  bool m_renamed;
  bool m_link;
  KUrl m_src;
  KUrl m_dst;
  QString m_target;
};

struct KonqCommand
{
  typedef QStack<KonqCommand> Stack;

  KonqCommand()
  { m_valid = false; }
  //KonqCommand( Type type, KonqBasicOperation::Stack& opStack, const KUrl::List& src, const KUrl& dest )
  //  : m_type( type ), m_opStack( opStack ), m_src( src ), m_dest( dest ) {}

  bool m_valid;

  KonqUndoManager::CommandType m_type;
  KonqBasicOperation::Stack m_opStack;
  KUrl::List m_src;
  KUrl m_dst;
};

class KonqCommandRecorder : public QObject
{
  Q_OBJECT
public:
  KonqCommandRecorder( KonqUndoManager::CommandType op, const KUrl::List &src, const KUrl &dst, KIO::Job *job );
  virtual ~KonqCommandRecorder();

private Q_SLOTS:
  void slotResult( KJob *job );

  void slotCopyingDone( KIO::Job *, const KUrl &from, const KUrl &to, bool directory, bool renamed );
  void slotCopyingLinkDone( KIO::Job *, const KUrl &from, const QString &target, const KUrl &to );

private:
  KonqCommand m_cmd;
};


#endif /* KONQ_UNDO_P_H */

