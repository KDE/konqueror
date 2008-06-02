/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2008 by George Goldberg <grundleborg@googlemail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef KONQ_FILEITEMCAPABILITIES_H
#define KONQ_FILEITEMCAPABILITIES_H

#include <libkonq_export.h>

#include <QtCore/QSharedDataPointer>

class KonqFileItemCapabilitiesPrivate;
class KFileItemList;

/**
 * @brief Provides information about the access capabilities of a group of 
 *        KFileItem objects.
 *
 * As soon as one file item does not support a specific capability, it is
 * marked as unsupported for all items.
 *
 * This class is implicitly shared, which means it can be used as a value and
 * copied around at almost no cost.
 */
class LIBKONQ_EXPORT KonqFileItemCapabilities
{
public:
    /**
     * @brief Constructor that takes a KFileItemList and sets the capabilities
     *        supported by all the FileItems as true.
     * @param items The list of items that are to have their supported
     *              capabilities checked.
     */
    KonqFileItemCapabilities(const KFileItemList& items);
    /**
     * @brief Copy constructor
     */
    KonqFileItemCapabilities(const KonqFileItemCapabilities&);
    /**
     * @brief Destructor
     */
    virtual ~KonqFileItemCapabilities();
    /**
     * @brief Check if reading capability is supported
     * @return true if all the FileItems support reading, otherwise false.
     */
    bool supportsReading() const;
    /**
     * @brief Check if deleting capability is supported
     * @return true if all the FileItems support deleting, otherwise false.
     */
    bool supportsDeleting() const;
    /**
     * @brief Check if writing capability is supported
     * @return true if all the FileItems support writing, otherwise false.
     */
    bool supportsWriting() const;
    /**
     * @brief Check if moving capability is supported
     * @return true if all the FileItems support moving, otherwise false.
     */
    bool supportsMoving() const;
    /**
     * @brief Check if files are loca
     * @return true if all the FileItems are local, othwise there is one or more
     *         remote file, so false.
     */
    bool isLocal() const;

private:
    /** @brief d-pointer */
   QSharedDataPointer<KonqFileItemCapabilitiesPrivate> d;
};

#endif
