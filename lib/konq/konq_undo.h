/* -*- c-basic-offset:2 -*- */
/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

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
#ifndef __konq_undo_h__
#define __konq_undo_h__

#include <QObject>
#include <kurl.h>

#include <libkonq_export.h>

namespace KIO
{
  class Job;
}
class KonqBasicOperation;
class KonqCommand;
class KonqUndoJob;
class KJob;

class LIBKONQ_EXPORT KonqUndoManager : public QObject
{
  Q_OBJECT
  friend class KonqUndoJob;
public:
  KonqUndoManager();
  virtual ~KonqUndoManager();

  static void incRef();
  static void decRef();
  static KonqUndoManager *self();

  enum CommandType { COPY, MOVE, LINK, MKDIR, TRASH };

  /**
   * Record this job while it's happening and add a command for it so that the user can undo it.
   * @param op the type of job - which is also the type of command that will be created for it
   * @param src list of source urls
   * @param dst destination url
   * @param job the job to record
   */
  void recordJob( CommandType op, const KUrl::List &src, const KUrl &dst, KIO::Job *job );

  bool undoAvailable() const;
  QString undoText() const;

public Q_SLOTS:
  void undo();

  // public for KonqUndoManagerAdaptor
  QByteArray get() const;

Q_SIGNALS:
  // The four signals below are emitted to dbus
  void push( const QByteArray &command );
  void pop();
  void lock();
  void unlock();

  void undoAvailable( bool avail );
  void undoTextChanged( const QString &text );

protected:
  /**
   * @internal
   */
  void stopUndo( bool step );

private Q_SLOTS:
  // Those four are connected to DBUS signals
  void slotPush( QByteArray command ); // const ref doesn't work due to QDataStream
  void slotPop();
  void slotLock();
  void slotUnlock();

private Q_SLOTS:
  void slotResult( KJob *job );

private:
  enum UndoState { MAKINGDIRS, MOVINGFILES, REMOVINGDIRS, REMOVINGFILES };

  // called by KonqCommandRecorder
  friend class KonqCommandRecorder;
  void addCommand( const KonqCommand &cmd );

  void pushCommand( const KonqCommand& cmd );
  void undoStep();

  void undoMakingDirectories();
  void undoMovingFiles();
  void undoRemovingFiles();
  void undoRemovingDirectories();

  void broadcastPush( const KonqCommand &cmd );
  void broadcastPop();
  void broadcastLock();
  void broadcastUnlock();

  void addDirToUpdate( const KUrl& url );
  bool initializeFromKDesky();

  class KonqUndoManagerPrivate;
  KonqUndoManagerPrivate *d;
  static KonqUndoManager *s_self;
  static unsigned long s_refCnt;
};

QDataStream &operator<<( QDataStream &stream, const KonqBasicOperation &op );
QDataStream &operator>>( QDataStream &stream, KonqBasicOperation &op );

QDataStream &operator<<( QDataStream &stream, const KonqCommand &cmd );
QDataStream &operator>>( QDataStream &stream, KonqCommand &cmd );

#endif
