#pragma once

#include <QString>
#include <QMutex>
#include <QSqlDatabase>
#include <optional>
#include "ai/OCR.h"

class OcrCache {
public:
    static OcrCache& instance();

    std::optional<std::vector<OCRResult>> lookup(const QString& hash);
    void store(const QString& hash, const std::vector<OCRResult>& results);

    // Returns total byte size of the cache database file.
    qint64 cacheSize() const;
    // Returns number of cached entries.
    int cacheEntries();
    // Removes all cached entries.
    void clearCache();
    // Get/set the maximum number of entries (default 100).
    int maxEntries() const { return m_maxEntries; }
    void setMaxEntries(int n);

private:
    OcrCache();
    QSqlDatabase getDb();
    void evictIfNeeded(QSqlDatabase& db);

    QMutex m_mutex;
    QString m_dbPath;
    int m_maxEntries = 100;
};
