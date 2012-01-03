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

#include "domtreecommands.h"

#include <dom/dom_doc.h>
#include <dom/dom_exception.h>

#include <klocale.h>

#include <qmap.h>

using namespace domtreeviewer;

static const char * const dom_error_msgs[] = {
	I18N_NOOP("No error"),
	I18N_NOOP("Index size exceeded"),
	I18N_NOOP("DOMString size exceeded"),
	I18N_NOOP("Hierarchy request error"),
	I18N_NOOP("Wrong document"),
	I18N_NOOP("Invalid character"),
	I18N_NOOP("No data allowed"),
	I18N_NOOP("No modification allowed"),
	I18N_NOOP("Not found"),
	I18N_NOOP("Not supported"),
	I18N_NOOP("Attribute in use"),
	I18N_NOOP("Invalid state"),
	I18N_NOOP("Syntax error"),
	I18N_NOOP("Invalid modification"),
	I18N_NOOP("Namespace error"),
	I18N_NOOP("Invalid access")
};

// == global functions ==============================================

QString domtreeviewer::domErrorMessage(int dom_err)
{
  if ((unsigned)dom_err >= sizeof dom_error_msgs/sizeof dom_error_msgs[0])
    return i18n("Unknown Exception %1", dom_err);
  else
    return i18n(dom_error_msgs[dom_err]);
}

// == ManipulationCommandSignalEmitter ==============================

static ManipulationCommandSignalEmitter *_mcse;

ManipulationCommandSignalEmitter::ManipulationCommandSignalEmitter()
{}
ManipulationCommandSignalEmitter::~ManipulationCommandSignalEmitter()
{}

namespace domtreeviewer {

ManipulationCommandSignalEmitter* ManipulationCommand::mcse()
{
  if (!_mcse) _mcse = new ManipulationCommandSignalEmitter;
  return _mcse;
}

}

// == ChangedNodeSet ================================================

namespace DOM {

inline static bool operator <(const DOM::Node &n1, const DOM::Node &n2)
{
  return (qptrdiff)n1.handle() - (qptrdiff)n2.handle() < 0;
}

}

namespace domtreeviewer {

// collection of nodes for which to emit the nodeChanged signal
class ChangedNodeSet : public QMap<DOM::Node, bool>
{
};

}

// == ManipulationCommand ===========================================

ManipulationCommand::ManipulationCommand() : _exception(0), changedNodes(0)
	, _reapplied(false) , allow_signals(true)
{
}

ManipulationCommand::~ManipulationCommand()
{
}

void ManipulationCommand::connect(const char *signal, QObject *recv, const char *slot)
{
  QObject::connect(mcse(), signal, recv, slot);
}

void ManipulationCommand::handleException(DOM::DOMException &ex)
{
  _exception = ex;
  QString msg = text() + ": " + domErrorMessage(ex.code);
  emit mcse()->error(ex.code, msg);
}

void ManipulationCommand::checkAndEmitSignals()
{
  if (allow_signals) {
    if (changedNodes) {
      ChangedNodeSet::Iterator end = changedNodes->end();
      for (ChangedNodeSet::Iterator it = changedNodes->begin(); it != end; ++it) {
        emit mcse()->nodeChanged(it.key());
      }
    }

    if (struc_changed) emit mcse()->structureChanged();
  }
  if (changedNodes) changedNodes->clear();
}

void ManipulationCommand::addChangedNode(const DOM::Node &node)
{
  if (!changedNodes) changedNodes = new ChangedNodeSet;
  changedNodes->insert(node, true);
}

void ManipulationCommand::redo()
{
  if (!isValid()) return;

  struc_changed = false;

  try {
    if (shouldReapply())
      reapply();
    else
      apply();

    checkAndEmitSignals();

  } catch(DOM::DOMException &ex) {
    handleException(ex);
  }
  _reapplied = true;
}

void ManipulationCommand::undo()
{
  if (!isValid()) return;

  struc_changed = false;

  try {
    unapply();
    checkAndEmitSignals();
  } catch(DOM::DOMException &ex) {
    handleException(ex);
  }
}

void ManipulationCommand::reapply()
{
  apply();
}

// == MultiCommand ===========================================

MultiCommand::MultiCommand(const QString &desc)
: _name(desc)
{
}

MultiCommand::~MultiCommand()
{
  qDeleteAll(cmds);
}

void MultiCommand::addCommand(ManipulationCommand *cmd)
{
  cmd->allow_signals = false;
  cmds.append(cmd);
}

void MultiCommand::apply()
{
  // apply in forward order
  QList<ManipulationCommand*>::ConstIterator it = cmds.constBegin(), itEnd = cmds.constEnd();
  for (; it != itEnd; ++it) {
    try {
      if (shouldReapply()) (*it)->reapply();
      else (*it)->apply();

      struc_changed |= (*it)->struc_changed;
      mergeChangedNodesFrom(*it);

    } catch (DOM::DOMException &) {
      // rollback
      for (--it; *it; --it) {
        try {
	  (*it)->unapply();
	} catch(DOM::DOMException &) {
	  // ignore
	}
      }
      throw;
    }

  }
}

void MultiCommand::unapply()
{
  // unapply in reverse order
  QListIterator<ManipulationCommand*> it(cmds);
  it.toBack();
  while (it.hasPrevious()) {
    ManipulationCommand* current = it.previous();
    try {
      current->unapply();

      struc_changed |= current->struc_changed;
      mergeChangedNodesFrom(current);

    } catch (DOM::DOMException &) {
      // rollback
      while (it.hasNext()) {
        ManipulationCommand* newcurrent = it.next();
        try {
	  newcurrent->reapply();
	} catch(DOM::DOMException &) {
	  // ignore
	}
      }
      throw;
    }

  }
}

void MultiCommand::mergeChangedNodesFrom(ManipulationCommand *cmd)
{
  if (!cmd->changedNodes) return;

  ChangedNodeSet::ConstIterator end = cmd->changedNodes->constEnd();
  for (ChangedNodeSet::ConstIterator it = cmd->changedNodes->constBegin(); it != end; ++it) {
    addChangedNode(it.key());
  }

  cmd->changedNodes->clear();
}

QString MultiCommand::name() const
{
  return _name;
}

// == AddAttributeCommand ===========================================

AddAttributeCommand::AddAttributeCommand(const DOM::Element &element, const QString &attrName, const QString &attrValue)
: _element(element), attrName(attrName), attrValue(attrValue)
{
  if (attrValue.isEmpty()) this->attrValue = "<dummy>";
}

AddAttributeCommand::~AddAttributeCommand()
{
}

void AddAttributeCommand::apply()
{
  _element.setAttribute(attrName, attrValue);
  addChangedNode(_element);
}

void AddAttributeCommand::unapply()
{
  _element.removeAttribute(attrName);
  addChangedNode(_element);
}

QString AddAttributeCommand::name() const
{
  return i18n("Add attribute");
}

// == ChangeAttributeValueCommand ====================================

ChangeAttributeValueCommand::ChangeAttributeValueCommand(
const DOM::Element &element, const QString &attr, const QString &value)
: _element(element), _attr(attr), new_value(value)
{
}

ChangeAttributeValueCommand::~ChangeAttributeValueCommand()
{
}

void ChangeAttributeValueCommand::apply()
{
  if (!shouldReapply()) old_value = _element.getAttribute(_attr);
  _element.setAttribute(_attr, new_value);
  addChangedNode(_element);
}

void ChangeAttributeValueCommand::unapply()
{
  _element.setAttribute(_attr, old_value);
  addChangedNode(_element);
}

QString ChangeAttributeValueCommand::name() const
{
  return i18n("Change attribute value");
}

// == RemoveAttributeCommand ========================================

RemoveAttributeCommand::RemoveAttributeCommand(const DOM::Element &element, const QString &attrName)
: _element(element), attrName(attrName)
{
}

RemoveAttributeCommand::~RemoveAttributeCommand()
{
}

void RemoveAttributeCommand::apply()
{
// kDebug(90180) << _element.nodeName().string() << ": " << attrName.string();
  if (!shouldReapply())
    oldAttrValue = _element.getAttribute(attrName);
  _element.removeAttribute(attrName);
  addChangedNode(_element);
}

void RemoveAttributeCommand::unapply()
{
  _element.setAttribute(attrName, oldAttrValue);
  addChangedNode(_element);
}

QString RemoveAttributeCommand::name() const
{
  return i18n("Remove attribute");
}

// == RenameAttributeCommand ========================================

RenameAttributeCommand::RenameAttributeCommand(const DOM::Element &element, const QString &attrOldName, const QString &attrNewName)
: _element(element), attrOldName(attrOldName), attrNewName(attrNewName)
{
}

RenameAttributeCommand::~RenameAttributeCommand()
{
}

void RenameAttributeCommand::apply()
{
  if (!shouldReapply())
    attrValue = _element.getAttribute(attrOldName);
  _element.removeAttribute(attrOldName);
  _element.setAttribute(attrNewName, attrValue);
  addChangedNode(_element);
}

void RenameAttributeCommand::unapply()
{
  _element.removeAttribute(attrNewName);
  _element.setAttribute(attrOldName, attrValue);
  addChangedNode(_element);
}

QString RenameAttributeCommand::name() const
{
  return i18n("Rename attribute");
}

// == ChangeCDataCommand ========================================

ChangeCDataCommand::ChangeCDataCommand(const DOM::CharacterData &cdata, const QString &value)
: cdata(cdata), value(value), has_newlines(false)
{
}

ChangeCDataCommand::~ChangeCDataCommand()
{
}

void ChangeCDataCommand::apply()
{
  if (!shouldReapply()) {
    oldValue = cdata.data();
    has_newlines =
    	QString::fromRawData(value.unicode(), value.length()).contains('\n')
	|| QString::fromRawData(oldValue.unicode(), oldValue.length()).contains('\n');
  }
  cdata.setData(value);
  addChangedNode(cdata);
  struc_changed = has_newlines;
}

void ChangeCDataCommand::unapply()
{
  cdata.setData(oldValue);
  addChangedNode(cdata);
  struc_changed = has_newlines;
}

QString ChangeCDataCommand::name() const
{
  return i18n("Change textual content");
}

// == ManipulateNodeCommand ===========================================

ManipulateNodeCommand::ManipulateNodeCommand(const DOM::Node &node, const DOM::Node &parent, const DOM::Node &after)
: _node(node), _parent(parent), _after(after)
{
}

ManipulateNodeCommand::~ManipulateNodeCommand()
{
}

void ManipulateNodeCommand::insert()
{
  _parent.insertBefore(_node, _after);
}

void ManipulateNodeCommand::remove()
{
  DOM::DocumentFragment frag = _node;

  if (frag.isNull()) {	// do a normal remove
    _node = _parent.removeChild(_node);

  } else {		// remove fragment nodes and recreate fragment
    DOM::DocumentFragment newfrag = _parent.ownerDocument().createDocumentFragment();

    for (DOM::Node i = frag.firstChild(); !i.isNull(); i = i.nextSibling()) {
      newfrag.appendChild(_parent.removeChild(i));
    }

    _node = newfrag;
  }
}

// == InsertNodeCommand ===========================================

InsertNodeCommand::InsertNodeCommand(const DOM::Node &node, const DOM::Node &parent, const DOM::Node &after)
: ManipulateNodeCommand(node, parent, after)
{
}

InsertNodeCommand::~InsertNodeCommand()
{
}

void InsertNodeCommand::apply()
{
  insert();
  struc_changed = true;
}

void InsertNodeCommand::unapply()
{
  remove();
  struc_changed = true;
}

QString InsertNodeCommand::name() const
{
  return i18n("Insert node");
}

// == RemoveNodeCommand ===========================================

RemoveNodeCommand::RemoveNodeCommand(const DOM::Node &node, const DOM::Node &parent, const DOM::Node &after)
: ManipulateNodeCommand(node, parent, after)
{
}

RemoveNodeCommand::~RemoveNodeCommand()
{
}

void RemoveNodeCommand::apply()
{
  remove();
  struc_changed = true;
}

void RemoveNodeCommand::unapply()
{
  insert();
  struc_changed = true;
}

QString RemoveNodeCommand::name() const
{
  return i18n("Remove node");
}

// == MoveNodeCommand ===========================================

MoveNodeCommand::MoveNodeCommand(const DOM::Node &node, const DOM::Node &parent, const DOM::Node &after)
: _node(node), new_parent(parent), new_after(after)
{
  old_parent = node.parentNode();
  old_after = node.nextSibling();
}

MoveNodeCommand::~MoveNodeCommand()
{
}

void MoveNodeCommand::apply()
{
  old_parent.removeChild(_node);
  try {
    new_parent.insertBefore(_node, new_after);
  } catch (DOM::DOMException &) {
    try {	// rollback
      old_parent.insertBefore(_node, old_after);
    } catch (DOM::DOMException &) {}
    throw;
  }
  struc_changed = true;
}

void MoveNodeCommand::unapply()
{
  new_parent.removeChild(_node);
  try {
    old_parent.insertBefore(_node, old_after);
  } catch (DOM::DOMException &) {
    try {	// rollback
      new_parent.insertBefore(_node, new_after);
    } catch (DOM::DOMException &) {}
    throw;
  }
  struc_changed = true;
}

QString MoveNodeCommand::name() const
{
  return i18n("Move node");
}

#include "domtreecommands.moc"

#undef MCSE
