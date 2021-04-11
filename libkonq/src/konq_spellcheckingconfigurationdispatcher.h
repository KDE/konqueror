/*
 * This file is part of the KDE project.
 *
 * Copyright 2021  Stefano Crocco <posta@stefanocrocco.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KONQ_SPELLCHECKINGCONFIGURATIONDISPATCHER_H
#define KONQ_SPELLCHECKINGCONFIGURATIONDISPATCHER_H

#include <QObject>

#include <libkonq_export.h>

class LIBKONQ_EXPORT KonqSpellCheckingConfigurationDispatcher : public QObject
{
    Q_OBJECT

public:
    static KonqSpellCheckingConfigurationDispatcher* self();

signals:
    void spellCheckingConfigurationChanged(bool enabled);

};

#endif // KONQ_SPELLCHECKINGCONFIGURATIONDISPATCHER_H
