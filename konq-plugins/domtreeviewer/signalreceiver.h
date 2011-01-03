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

#ifndef KONQ_PLUGIN_SIGNALRECEIVER_H
#define KONQ_PLUGIN_SIGNALRECEIVER_H

#include <qobject.h>

/**
 * \brief Class for receiving signals.
 *
 * This is a convenience class for receiving signals when it is not worth
 * the effort, or overly complicated to introduce a dedicated slot.
 *
 * Use as follows:
 * \code
 * SignalReceiver sr;
 * sr.connect(some_obj, SIGNAL(someSignal()), SLOT(slot()));
 * <do something with some_obj> ...
 * if (sr.receivedSignal()) { // yes, signal was received
 * }
 * \endcode
 *
 * It is not possible to discriminate between different signals. Hence,
 * use different signal receiver instances for different signals.
 * @autor Leo Savernik
 */
class SignalReceiver : public QObject
{
  Q_OBJECT

public:  
  explicit SignalReceiver(QObject *parent = 0);
  virtual ~SignalReceiver();
  
  /** returns true if any signal has been received */
  bool signalReceived() const { return rcvd; }
  
  /** returns true if any signal has been received */
  bool operator ()() const { return rcvd; }

public slots:
  /** connect a signal to this slot to receive it */
  void slot();

private:
  bool rcvd;
};

#endif // KONQ_PLUGIN_SIGNALRECEIVER_H
