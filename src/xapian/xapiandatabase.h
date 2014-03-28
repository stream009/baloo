/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef BALOO_XAPIANDATABASE_H
#define BALOO_XAPIANDATABASE_H

#include <xapian.h>
#include "xapian_export.h"

#include <QString>
#include <QObject>
#include <QPair>
#include <QVector>

namespace Baloo {

class XapianDocument;

class BALOO_XAPIAN_EXPORT XapianDatabase : public QObject
{
    Q_OBJECT
public:
    XapianDatabase(const QString& path);

    void replaceDocument(uint id, const Xapian::Document& doc);
    void deleteDocument(uint id);

    /**
     * Commit all the pending changes. This may not commit
     * at this instance as the db might be locked by another process
     * It emits the committed signal on completion
     */
    void commit();

    XapianDocument document(uint id);

    /**
     * A pointer to the actual db. Only use this when doing queries
     */
    Xapian::Database* db() {
        if (m_db) {
            m_db->reopen();
        }
        return m_db;
    }

Q_SIGNALS:
    void committed();

private:
    Xapian::Database* m_db;

    typedef QPair<Xapian::docid, Xapian::Document> DocIdPair;
    QVector<DocIdPair> m_docsToAdd;
    QVector<uint> m_docsToRemove;

    std::string m_path;

    void retryCommit();
};

}

#endif // BALOO_XAPIANDATABASE_H