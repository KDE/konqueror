#ifndef KONQUNDOMANAGER_H
#define KONQUNDOMANAGER_H

#include <QObject>

/**
 * Note that there is one KonqUndoManager per mainwindow.
 * It integrates KonqFileUndoManager (undoing file operations)
 * and undoing the closing of tabs.
 */
class KonqUndoManager : public QObject
{
    Q_OBJECT
public:
    explicit KonqUndoManager(QObject* parent);
    ~KonqUndoManager();

    bool undoAvailable() const;

public Q_SLOTS:
    void undo();

Q_SIGNALS:
    void undoAvailable(bool canUndo);
    void undoTextChanged(const QString& text);

private Q_SLOTS:
    void slotFileUndoAvailable(bool);
    void slotFileUndoTextChanged(const QString& text);
};

#endif /* KONQUNDOMANAGER_H */

