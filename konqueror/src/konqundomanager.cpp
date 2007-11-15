#include "konqundomanager.h"
#include <konq_fileundomanager.h>

KonqUndoManager::KonqUndoManager(QObject* parent)
    : QObject(parent)
{
    KonqFileUndoManager::incRef();
    connect( KonqFileUndoManager::self(), SIGNAL(undoAvailable(bool)),
             this, SLOT(slotFileUndoAvailable(bool)) );
    connect( KonqFileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
             this, SLOT(slotFileUndoTextChanged(QString)) );
}

KonqUndoManager::~KonqUndoManager()
{
    KonqFileUndoManager::decRef();
}

void KonqUndoManager::slotFileUndoAvailable(bool)
{
    emit undoAvailable(this->undoAvailable());
}

bool KonqUndoManager::undoAvailable() const
{
    // TODO "or tab closing"
    return KonqFileUndoManager::self()->undoAvailable();
}

void KonqUndoManager::undo()
{
    // TODO tab closing handling
    KonqFileUndoManager::self()->undo();
}

void KonqUndoManager::slotFileUndoTextChanged(const QString& text)
{
    // I guess we can always just forward that one?
    emit undoTextChanged(text);
}
