#include "contents_extractor.h"

#include "baloodebug.h"
#include "basicindexingjob.h"
#include "transaction.h"
#include "extractor/result.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

#include <KFileMetaData/Extractor>

namespace Baloo {

ContentsExtractor::
ContentsExtractor(Database &db, FileIndexerConfig &config)
    : m_db { db }
    , m_config { config }
{}

void ContentsExtractor::
index(const QVector<qint64> &ids)
{
    QStringList updatedPaths;
    for (const auto id: ids) {
        const auto &path = index(id);
        if (!path.isEmpty()) {
            updatedPaths.push_back(path);
        }
    }

    auto &&message =
        QDBusMessage::createSignal("/files", "org.kde", "changed");

    QVariantList vl;
    vl.push_back(updatedPaths);
    message.setArguments(vl);

    QDBusConnection::sessionBus().send(message);
}

QString ContentsExtractor::
index(const quint64 id)
{
    Transaction tr { &m_db, Transaction::ReadWrite };

    const auto &url = QFile::decodeName(tr.documentUrl(id));
    if (!QFile::exists(url)) {
        qCDebug(BALOO) << "File doesn't exist";
        tr.removeDocument(id);
        tr.commit();
        return {};
    }

    const auto &mimetype = m_mimeDb.mimeTypeForFile(url).name();

    const bool shouldIndex = m_config.shouldBeIndexed(url)
                          && m_config.shouldMimeTypeBeIndexed(mimetype);
    if (!shouldIndex) {
        // FIXME: This should never be happening!
        tr.removeDocument(id);
        tr.commit();
        qCDebug(BALOO) << url << "shouldn't be indexed. skipping.";
        return {};
    }

    //
    // HACK: We only want to index plain text files which end with a .txt
    //
    if (mimetype == QLatin1String("text/plain")) {
        if (!url.endsWith(QLatin1String(".txt"))) {
            qCDebug(BALOO) << "text/plain does not end with .txt. Ignoring";
            tr.removePhaseOne(id);
            tr.commit();
            return {};
        }
    }

    //
    // HACK: Also, we're ignoring ttext files which are greater tha 10 Mb as we
    // have trouble processing them
    //
    if (mimetype.startsWith(QStringLiteral("text/"))) {
        const QFileInfo fileInfo(url);
        if (fileInfo.size() >= 10 * 1024 * 1024) {
            qCDebug(BALOO) << url << "is bigger than 10MB. Ignoring";
            tr.removePhaseOne(id);
            tr.commit();
            return {};
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
        tr.removeDocument(id);
        if (!tr.hasDocument(doc.id())) {
            tr.addDocument(result.document());
        } else {
            tr.replaceDocument(result.document(), DocumentTerms | DocumentData);
        }
    } else {
        tr.replaceDocument(result.document(), DocumentTerms | DocumentData);
        qCDebug(BALOO) << url << "has indexed properly";
    }
    tr.removePhaseOne(doc.id());
    tr.commit();

    return url;
}

} // namespace Baloo
