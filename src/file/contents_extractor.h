#ifndef CONTENTS_EXTRACTOR_HPP
#define CONTENTS_EXTRACTOR_HPP

#include "database.h"
#include "fileindexerconfig.h"

#include <QMimeDatabase>

#include <KFileMetaData/ExtractorCollection>

class QString;

namespace Baloo {

class ContentsExtractor
{
public:
    ContentsExtractor(Database&, FileIndexerConfig&);

    void index(const QVector<quint64> &ids);

private:
    QString index(const quint64 id);

private:
    Database &m_db;
    FileIndexerConfig &m_config;
    QMimeDatabase m_mimeDb;
    KFileMetaData::ExtractorCollection m_extractorCollection;
};

} // namespace Baloo

#endif // CONTENTS_EXTRACTOR_HPP
