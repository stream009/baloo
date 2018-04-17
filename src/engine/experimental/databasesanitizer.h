/*
 * This file is part of the KDE Baloo project.
 * Copyright 2018  Michael Heidelbach <ottwolt@gmail.com>
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

#ifndef BALOODATABASESANITIZER_H
#define BALOODATABASESANITIZER_H

#include "transaction.h"

namespace Baloo 
{
class DatabaseSanitizerImpl;
/**
* \brief Provide methods to show database problems and
* sanitize them.
*/
class BALOO_ENGINE_EXPORT DatabaseSanitizer
{
public:
    DatabaseSanitizer(const Database& db, Transaction::TransactionType type);
    DatabaseSanitizer(Database* db, Transaction::TransactionType type);
    ~DatabaseSanitizer();
    
    /** 
    * Print database content to stdout
    * 
    * \p deviceIDs filter by device ids. Negative numbers list everything but...
    * with empty \p deviceIDs(default) everything is printed.
    * 
    * \p missingOnly Simulate purging operation. Only inaccessible items are printed.
    * 
    * \p urlFilter Filter result urls. Default is null = Print everything.
    */
    void printList(const QVector<qint64>& deviceIds, 
        const bool missingOnly, 
        const QSharedPointer<QRegularExpression>& urlFilter
    );
    /** 
    * Print info about known devices to stdout
    * 
    * \p deviceIDs filter by device ids. Negative numbers list everything but...
    * with empty \p deviceIDs(default) everything is printed.
    * 
    * \p missingOnly Only inaccessible items are printed.
    */
    void printDevices(const QVector<qint64>& deviceIds, const bool missingOnly = false);
   
private:
    DatabaseSanitizer(const DatabaseSanitizer& rhs) = delete;
    DatabaseSanitizerImpl* m_pimpl;
};

}
#endif // BALOODATABASESANITIZER_H
