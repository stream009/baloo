/*
 * Copyright (C) 2015  Vishesh Handa <vhanda@kde.org>
 * Copyright (C) 2015  Pinak Ahuja <pinak.ahuja@gmail.com>
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

#include "extractorprocess.h"

#include "baloodebug.h"

#include <cassert>

#include <QStandardPaths>
#include <QDebug>

using namespace Baloo;

ExtractorProcess::ExtractorProcess(QObject* parent)
    : QObject(parent)
    //, m_extractorPath(QStandardPaths::findExecutable(QLatin1String("baloo_file_extractor")))
    , m_extractorPath("extractor/baloo_file_extractor")
    , m_extractorProcess(this)
    , m_batchSize(0)
    , m_indexedFiles(0)
    , m_extractorIdle(true)
{
    qCDebug(BALOO) << "Launching extractor process...";
    connect(&m_extractorProcess, &QProcess::readyRead, this, &ExtractorProcess::slotIndexingFile);
    m_extractorProcess.start(m_extractorPath, QStringList(), QIODevice::Unbuffered | QIODevice::ReadWrite);
    m_extractorProcess.waitForStarted();

    qCDebug(BALOO) << "extractor process started. PID:" << m_extractorProcess.processId();
    connect(&m_extractorProcess,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [](int exitCode, QProcess::ExitStatus exitStatus) {
            qCDebug(BALOO) << QString {
                "extractor prcess has finished (code: %1, status: %2)"
            }.arg(exitCode).arg(exitStatus);
        }
    );

    m_extractorProcess.setReadChannel(QProcess::StandardOutput);
    qCDebug(BALOO) << "...done";
}

ExtractorProcess::~ExtractorProcess()
{
    m_extractorProcess.close();
}

void ExtractorProcess::index(const QVector<quint64>& fileIds)
{
    Q_ASSERT(m_extractorProcess.state() == QProcess::Running);
    Q_ASSERT(m_extractorIdle);
    Q_ASSERT(!fileIds.isEmpty());

    QByteArray batchData;

    m_batchSize = fileIds.size();
    batchData.append(reinterpret_cast<char*>(&m_batchSize), sizeof(quint32));
    for (quint64 id : fileIds) {
        batchData.append(reinterpret_cast<char*>(&id), sizeof(quint64));
    }

    m_extractorIdle = false;
    m_indexedFiles = 0;
    qCDebug(BALOO) << "sending batch data";
    m_extractorProcess.write(batchData.data(), batchData.size());
}

void ExtractorProcess::slotIndexingFile()
{
    while (m_extractorProcess.canReadLine()) {
        QString filePath = m_extractorProcess.readLine().trimmed();
        if (m_indexedFiles < m_batchSize) {
            Q_EMIT indexingFile(filePath);
        }
        ++m_indexedFiles;
    }

    if (m_indexedFiles == m_batchSize + 1) {
        Q_EMIT done();
        m_extractorIdle = true;
    }
}
