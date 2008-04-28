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
#ifndef KONQ_FILEUNDOMANAGER_H
#define KONQ_FILEUNDOMANAGER_H

#include <QtCore/QObject>
#include <kurl.h>

#include <libkonq_export.h>

namespace KIO
{
  class Job;
}
class KonqFileUndoManagerPrivate;
class KonqCommand;
class KonqUndoJob;
class KDateTime;

/**
 * KonqFileUndoManager: makes it possible to undo kio jobs.
 * This class is a singleton, use self() to access its only instance.
 */
class LIBKONQ_EXPORT KonqFileUndoManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @return the KonqFileUndoManager instance
     */
    static KonqFileUndoManager *self();

    /**
     * Interface for the gui handling of KonqFileUndoManager.
     * This includes three events currently:
     * - error when undoing a job
     * - confirm deletion before undoing a copy job
     * - confirm deletion when the copied file has been modified afterwards
     *
     * By default UiInterface shows message boxes in all three cases;
     * applications can reimplement this interface to provide different user interfaces.
     */
    class LIBKONQ_EXPORT UiInterface
    {
    public:
        UiInterface();
        virtual ~UiInterface();

        /**
         * Sets whether to show progress info when running the KIO jobs for undoing.
         */
        void setShowProgressInfo(bool b);
        /**
         * @returns whether progress info dialogs are shown while undoing.
         */
        bool showProgressInfo() const;

        /**
         * Sets the parent widget to use for message boxes.
         */
        void setParentWidget(QWidget* parentWidget);

        /**
         * @return the parent widget passed to the last call to undo(parentWidget), or 0.
         */
        QWidget* parentWidget() const;

        /**
         * Called when an undo job errors; default implementation displays a message box.
         */
        virtual void jobError(KIO::Job* job);

        /**
         * Called when we are about to remove those files.
         * Return true if we should proceed with deleting them.
         */
        virtual bool confirmDeletion(const KUrl::List& files);

        /**
         * Called when dest was modified since it was copied from src.
         * Note that this is called after confirmDeletion.
         * Return true if we should proceed with deleting dest.
         */
        virtual bool copiedFileWasModified(const KUrl& src, const KUrl& dest, const KDateTime& srcTime, const KDateTime& destTime);

        /**
         * \internal, for future extensions
         */
        virtual void virtual_hook(int id, void* data);

    private:
        class UiInterfacePrivate;
        UiInterfacePrivate *d;
    };

    /**
     * Set a new UiInterface implementation.
     * This deletes the previous one.
     * @param ui the UiInterface instance, which becomes owned by the undo manager.
     */
    void setUiInterface(UiInterface* ui);

    /**
     * @return the UiInterface instance passed to setUiInterface.
     * This is useful for calling setParentWidget on it. Never delete it!
     */
    UiInterface* uiInterface() const;

    enum CommandType { COPY, MOVE, RENAME, LINK, MKDIR, TRASH };

    /**
     * Record this job while it's happening and add a command for it so that the user can undo it.
     * @param op the type of job - which is also the type of command that will be created for it
     * @param src list of source urls
     * @param dst destination url
     * @param job the job to record
     */
    void recordJob(CommandType op, const KUrl::List &src, const KUrl &dst, KIO::Job *job);

    /**
     * @return true if undo is possible. Usually used for enabling/disabling the undo action.
     */
    bool undoAvailable() const;
    /**
     * @return the current text for the undo action.
     */
    QString undoText() const;

    /**
     * These two functions are useful when wrapping KonqFileUndoManager and adding custom commands.
     * Each command has a unique ID. You can get a new serial number for a custom command
     * with newCommandSerialNumber(), and then when you want to undo, check if the command
     * KonqFileUndoManager would undo is newer or older than your custom command.
     */
    quint64 newCommandSerialNumber();
    quint64 currentCommandSerialNumber() const;

    /// @deprecated
    static KDE_DEPRECATED void incRef() {}
    /// @deprecated
    static KDE_DEPRECATED void decRef() {}

public Q_SLOTS:
    /**
     * Undoes the last command
     * Remember to call uiInterface()->setParentWidget(parentWidget) first,
     * if you have multiple mainwindows.
     */
    void undo();

Q_SIGNALS:
    /// Emitted when the value of undoAvailable() changes
    void undoAvailable(bool avail);

    /// Emitted when the value of undoText() changes
    void undoTextChanged(const QString &text);

    /// Emitted when an undo job finishes. Used for unit testing.
    void undoJobFinished();

private:
    KonqFileUndoManager();
    virtual ~KonqFileUndoManager();
    friend class KonqFileUndoManagerSingleton;

    friend class KonqUndoJob;
    friend class KonqCommandRecorder;

    friend class KonqFileUndoManagerPrivate;
    KonqFileUndoManagerPrivate *d;
};

#endif
