/* This file is part of the KDE project
 *
 * Copyright (C) 2005 Leo Savernik <l.savernik@aon.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef domtreecommands_H
#define domtreecommands_H

#include <dom/dom_element.h>
#include <dom/dom_exception.h>
#include <dom/dom_string.h>
#include <dom/dom_text.h>

#include <QUndoCommand>

#include <qlist.h>
#include <qobject.h>


namespace domtreeviewer {

class ManipulationCommandSignalEmitter;
class ChangedNodeSet;

/** returns a localized string for the given dom exception code */
QString domErrorMessage(int exception_code);

/**
 * Internal class for emitting signals.
 * @internal
 */
class ManipulationCommandSignalEmitter : public QObject
{
  Q_OBJECT
public:
  ManipulationCommandSignalEmitter();
  virtual ~ManipulationCommandSignalEmitter();

// public signals:
signals:
#if !defined(Q_MOC_RUN) && !defined(DOXYGEN_SHOULD_SKIP_THIS) && !defined(IN_IDE_PARSER)
public:
#endif
  /** emitted if the DOM structure has been changed */
  void structureChanged();
  /** emitted if a DOM node has been changed */
  void nodeChanged(const DOM::Node &changedNode);
  /** emitted if an error occurred
   * @param err_id DOM error id
   * @param msg error message
   */
  void error(int err_id, const QString &msg);
};

/**
 * Base class of all dom tree manipulation commands.
 * @author Leo Savernik
 */
class ManipulationCommand : public QUndoCommand
{
public:
  ManipulationCommand();
  virtual ~ManipulationCommand();

  /** returns whether this command is still valid and can be executed */
  bool isValid() const { return !_exception.code; }
  /** returns the last occurred DOM exception */
  DOM::DOMException exception() const { return _exception; }
  /** returns true when the next issue of execute will reapply the command */
  bool shouldReapply() const { return _reapplied; }
  /** returns true if the command may emit signals */
  bool allowSignals() const { return allow_signals; }

  /** connects the given signal to a slot */
  static void connect(const char *signal, QObject *recv, const char *slot);

  /** does grunt work and calls apply()/reapply() */
  virtual void redo();
  /** does grunt work and calls unapply() */
  virtual void undo();

protected:
  virtual void apply() = 0;
  virtual void reapply();
  virtual void unapply() = 0;

  void handleException(DOM::DOMException &);
  void checkAndEmitSignals();
  void addChangedNode(const DOM::Node &);

  static ManipulationCommandSignalEmitter *mcse();

protected:
  DOM::DOMException _exception;
  ChangedNodeSet *changedNodes;
  bool _reapplied:1;
  bool struc_changed:1;

private:
  bool allow_signals:1;

  friend class MultiCommand;
};

/**
 * Combines multiple commands into a single command.
 *
 * Does basically the same as KMacroCommand, but inherits from
 * ManipulationCommand, and supports rollback.
 */
class MultiCommand : public ManipulationCommand
{
public:
  MultiCommand(const QString &name);
  virtual ~MultiCommand();

  /** Adds a new command. Will take ownership of \c cmd */
  void addCommand(ManipulationCommand *cmd);

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

  void mergeChangedNodesFrom(ManipulationCommand *cmd);

protected:
  QList<ManipulationCommand*> cmds;
  QString _name;
};

/**
 * Adds an attribute to a node.
 * @author Leo Savernik
 */
class AddAttributeCommand : public ManipulationCommand
{
public:
  AddAttributeCommand(const DOM::Element &element, const QString &attrName, const QString &attrValue);
  virtual ~AddAttributeCommand();

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

protected:
  DOM::Element _element;
  DOM::DOMString attrName;
  DOM::DOMString attrValue;
};

/**
 * Manipulates an attribute's value.
 * @author Leo Savernik
 */
class ChangeAttributeValueCommand : public ManipulationCommand
{
public:
  ChangeAttributeValueCommand(const DOM::Element &element, const QString &attr, const QString &value);
  virtual ~ChangeAttributeValueCommand();

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

protected:
  DOM::Element _element;
  DOM::DOMString _attr;
  DOM::DOMString old_value;
  DOM::DOMString new_value;
};

/**
 * Removes an attribute from a node.
 * @author Leo Savernik
 */
class RemoveAttributeCommand : public ManipulationCommand
{
public:
  RemoveAttributeCommand(const DOM::Element &element, const QString &attrName);
  virtual ~RemoveAttributeCommand();

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

protected:
  DOM::Element _element;
  DOM::DOMString attrName;
  DOM::DOMString oldAttrValue;
};

/**
 * Renames an attribute.
 * @author Leo Savernik
 */
class RenameAttributeCommand : public ManipulationCommand
{
public:
  RenameAttributeCommand(const DOM::Element &element, const QString &attrOldName, const QString &attrNewName);
  virtual ~RenameAttributeCommand();

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

protected:
  DOM::Element _element;
  DOM::DOMString attrOldName;
  DOM::DOMString attrNewName;
  DOM::DOMString attrValue;
};

/**
 * Changes the value of a CData-node.
 * @author Leo Savernik
 */
class ChangeCDataCommand : public ManipulationCommand
{
public:
  ChangeCDataCommand(const DOM::CharacterData &, const QString &value);
  virtual ~ChangeCDataCommand();

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

protected:
  DOM::CharacterData cdata;
  DOM::DOMString value;
  DOM::DOMString oldValue;
  bool has_newlines;
};

/**
 * Handles insertion and deletion primitives of nodes.
 * @author Leo Savernik
 */
class ManipulateNodeCommand : public ManipulationCommand
{
public:
  /**
   * Prepare command, where \c node is to be contained in \c parent, just
   * before \c after. If \c after is 0, it is appended at the end.
   */
  ManipulateNodeCommand(const DOM::Node &node, const DOM::Node &parent, const DOM::Node &after);
  virtual ~ManipulateNodeCommand();

protected:
  void insert();
  void remove();

protected:
  DOM::Node _node;
  DOM::Node _parent;
  DOM::Node _after;
};

/**
 * Inserts a node into the tree.
 *
 * The handed in node may be a full tree, even a document fragment.
 *
 * @author Leo Savernik
 */
class InsertNodeCommand : public ManipulateNodeCommand
{
public:
  /**
   * Prepare insertion command, inserting \c node into \c parent, just
   * before \c after. If \c after is 0, append it to the list of children.
   */
  InsertNodeCommand(const DOM::Node &node, const DOM::Node &parent, const DOM::Node &after);
  virtual ~InsertNodeCommand();

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

protected:
};

/**
 * Removes a node from the tree.
 *
 * The handed in node may be a full tree, even a document fragment.
 *
 * @author Leo Savernik
 */
class RemoveNodeCommand : public ManipulateNodeCommand
{
public:
  /**
   * Prepare insertion command, inserting \c node into \c parent, just
   * before \c after. If \c after is 0, append it to the list of children.
   */
  RemoveNodeCommand(const DOM::Node &node, const DOM::Node &parent, const DOM::Node &after);
  virtual ~RemoveNodeCommand();

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

protected:
};

/**
 * Moves a node.
 * @author Leo Savernik
 */
class MoveNodeCommand : public ManipulationCommand
{
public:
  /**
   * Move \c node from current position into \c parent, just before \c after.
   * Appends if \c after is 0.
   */
  MoveNodeCommand(const DOM::Node &node, const DOM::Node &parent, const DOM::Node &after);
  virtual ~MoveNodeCommand();

  virtual QString name() const;

protected:
  virtual void apply();
  virtual void unapply();

protected:
  DOM::Node _node;
  DOM::Node old_parent, old_after;
  DOM::Node new_parent, new_after;
};

} // namespace domtreeviewer

#endif // domtreewindow_H
