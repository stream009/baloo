/*
 * This file is part of the KDE Baloo Project
 * Copyright (C) 2013-2015  Vishesh Handa <vhanda@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "app.h"
#include "basicindexingjob.h"
#include "result.h"
#include "idutils.h"
#include "transaction.h"
#include "baloodebug.h"
#include "global.h"

#include <QCoreApplication>

#include <QTimer>
#include <QFileInfo>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QFile>

#include <KFileMetaData/Extractor>
#include <KFileMetaData/PropertyInfo>

#include <unistd.h> //for STDIN_FILENO
#include <iostream>

using namespace Baloo;

App::App(QObject* parent)
    : QObject(parent)
    , m_notifyNewData(STDIN_FILENO, QSocketNotifier::Read)
    , m_io(STDIN_FILENO, STDOUT_FILENO)
    , m_tr(0)
{
    qCDebug(BALOO) << ">>";
    connect(&m_notifyNewData, &QSocketNotifier::activated, this, &App::slotNewInput);
    qCDebug(BALOO) << "<<";
}

void App::slotNewInput()
{
    qCDebug(BALOO) << ">>";
    Database *db = globalDatabaseInstance();
    if (!db->open(Database::OpenDatabase)) {
        qCritical() << "Failed to open the database";
        exit(1);
    }

    Q_ASSERT(m_tr == 0);

    m_tr = new Transaction(db, Transaction::ReadWrite);
    // FIXME: The transaction is open for way too long. We should just open it for when we're
    //        committing the data not during the extraction.
    m_io.newBatch();
    QTimer::singleShot(0, this, &App::processNextFile);

    /**
     * A Single Batch seems to be triggering the SocketNotifier more than once
     * so we disable it till the batch is done.
     */
    m_notifyNewData.setEnabled(false);
    qCDebug(BALOO) << "<<";
}

void App::processNextFile()
{
    qCDebug(BALOO) << ">>";
    if (!m_io.atEnd()) {
        int delay = m_idleMonitor.isIdle() ? 0 : 10;

        quint64 id = m_io.nextId();

        QString url = QFile::decodeName(m_tr->documentUrl(id));
        qCDebug(BALOO) << url;
        if (!QFile::exists(url)) {
            qCDebug(BALOO) << "File doesn't exist";
            m_tr->removeDocument(id);
            QTimer::singleShot(0, this, &App::processNextFile);
            m_io.batchIndexed();
            return;
        }

        m_io.indexingUrl(url);
        index(m_tr, url, id);
        m_updatedFiles << url;

        QTimer::singleShot(delay, this, &App::processNextFile);

    } else {
        m_tr->commit();
        delete m_tr;
        m_tr = 0;

        /*
        * TODO we're already sending out each file as we start we can simply send out a done
        * signal isntead of sending out the list of files, that will need changes in whatever
        * uses this signal, Dolphin I think?
        */
        QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/files"),
                                                        QLatin1String("org.kde"),
                                                        QLatin1String("changed"));

        QVariantList vl;
        vl.reserve(1);
        vl << QVariant(m_updatedFiles);
        m_updatedFiles.clear();
        message.setArguments(vl);

        QDBusConnection::sessionBus().send(message);

        // Enable the SocketNotifier for the next batch
        m_notifyNewData.setEnabled(true);
        m_io.batchIndexed();
        qCDebug(BALOO) << "Batch finished";
    }
    qCDebug(BALOO) << "<<";
}

void App::index(Transaction* tr, const QString& url, quint64 id)
{
    qCDebug(BALOO) << ">>";
    QString mimetype = m_mimeDb.mimeTypeForFile(url).name();

    bool shouldIndex = m_config.shouldBeIndexed(url) && m_config.shouldMimeTypeBeIndexed(mimetype);
    if (!shouldIndex) {
        // FIXME: This should never be happening!
        tr->removeDocument(id);
        qCDebug(BALOO) << url << "shouldn't be indexed. skipping.";
        return;
    }

    //
    // HACK: We only want to index plain text files which end with a .txt
    //
    if (mimetype == QLatin1String("text/plain")) {
        if (!url.endsWith(QLatin1String(".txt"))) {
            qCDebug(BALOO) << "text/plain does not end with .txt. Ignoring";
            tr->removePhaseOne(id);
            return;
        }
    }

    //
    // HACK: Also, we're ignoring ttext files which are greater tha 10 Mb as we
    // have trouble processing them
    //
    if (mimetype.startsWith(QStringLiteral("text/"))) {
        QFileInfo fileInfo(url);
        if (fileInfo.size() >= 10 * 1024 * 1024) {
            qCDebug(BALOO) << url << "is bigger than 10MB. Ignoring";
            tr->removePhaseOne(id);
            return;
        }
    }

    // We always run the basic indexing again. This is mostly so that the proper
    // mimetype is set and we get proper type information.
    // The mimetype fetched in the BasicIQ is fast but not accurate
    BasicIndexingJob basicIndexer(url, mimetype, BasicIndexingJob::NoLevel);
    basicIndexer.index();

    Baloo::Document doc = basicIndexer.document();

    Result result(url, mimetype, KFileMetaData::ExtractionResult::ExtractEverything);
    result.setDocument(doc);

    QList<KFileMetaData::Extractor*> exList = m_extractorCollection.fetchExtractors(mimetype);

    if (exList.empty()) {
        qCDebug(BALOO) << url << mimetype << "doesn't have corresponding extractor.";
    }
    Q_FOREACH (KFileMetaData::Extractor* ex, exList) {
        ex->extract(&result);
    }

    result.finish();
    if (doc.id() != id) {
        qWarning() << url << "id seems to have changed. Perhaps baloo was not running, and this file was deleted + re-created";
        tr->removeDocument(id);
        if (!tr->hasDocument(doc.id())) {
            tr->addDocument(result.document());
        } else {
            tr->replaceDocument(result.document(), DocumentTerms | DocumentData);
        }
    } else {
        tr->replaceDocument(result.document(), DocumentTerms | DocumentData);
        qCDebug(BALOO) << url << "has indexed properly";
    }
    tr->removePhaseOne(doc.id());
    qCDebug(BALOO) << "<<";
}
