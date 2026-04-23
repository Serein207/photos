#include "OcrCache.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QThread>
#include <QDebug>

OcrCache& OcrCache::instance() {
    static OcrCache inst;
    return inst;
}

OcrCache::OcrCache() {
    m_dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ocr_cache.db";
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    // Create the schema on the calling (UI) thread's connection.
    QSqlDatabase db = getDb();
    if (db.isOpen()) {
        QSqlQuery q(db);
        q.exec(R"(
            CREATE TABLE IF NOT EXISTS ocr_cache (
                hash        TEXT PRIMARY KEY,
                result      TEXT NOT NULL,
                accessed_at INTEGER NOT NULL
            )
        )");
    }
}

QSqlDatabase OcrCache::getDb() {
    // Each thread gets its own named connection to avoid Qt's cross-thread warning.
    QString connName = QString("ocr_cache_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16);

    if (QSqlDatabase::contains(connName))
        return QSqlDatabase::database(connName);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(m_dbPath);
    if (!db.open())
        qWarning() << "OcrCache: failed to open db on thread" << connName << db.lastError().text();
    else {
        // Ensure schema exists on this connection too (worker threads).
        QSqlQuery q(db);
        q.exec(R"(
            CREATE TABLE IF NOT EXISTS ocr_cache (
                hash        TEXT PRIMARY KEY,
                result      TEXT NOT NULL,
                accessed_at INTEGER NOT NULL
            )
        )");
    }
    return db;
}

static QJsonArray serializeResults(const std::vector<OCRResult>& results) {
    QJsonArray arr;
    for (const auto& r : results) {
        QJsonObject obj;
        obj["text"]   = QString::fromStdString(r.text);
        obj["is_url"] = r.is_url;
        QJsonArray box;
        for (const auto& pt : r.box) { QJsonArray p; p.append(pt.x); p.append(pt.y); box.append(p); }
        obj["box"] = box;
        QJsonArray chars;
        for (const auto& cb : r.chars) {
            QJsonObject c;
            c["ch"]    = QString(cb.ch);
            c["cx"]    = cb.rect.center.x;
            c["cy"]    = cb.rect.center.y;
            c["w"]     = cb.rect.size.width;
            c["h"]     = cb.rect.size.height;
            c["angle"] = cb.rect.angle;
            c["bh"]    = cb.boxHeight;
            chars.append(c);
        }
        obj["chars"] = chars;
        arr.append(obj);
    }
    return arr;
}

static std::vector<OCRResult> deserializeResults(const QJsonArray& arr) {
    std::vector<OCRResult> results;
    for (const auto& v : arr) {
        QJsonObject obj = v.toObject();
        OCRResult r;
        r.text   = obj["text"].toString().toStdString();
        r.is_url = obj["is_url"].toBool();
        for (const auto& pv : obj["box"].toArray()) {
            auto pa = pv.toArray();
            r.box.emplace_back(pa[0].toInt(), pa[1].toInt());
        }
        for (const auto& cv : obj["chars"].toArray()) {
            QJsonObject c = cv.toObject();
            CharBox cb;
            QString ch = c["ch"].toString();
            cb.ch = ch.isEmpty() ? QChar() : ch.at(0);
            cb.rect = cv::RotatedRect(
                cv::Point2f(c["cx"].toDouble(), c["cy"].toDouble()),
                cv::Size2f(c["w"].toDouble(),  c["h"].toDouble()),
                c["angle"].toDouble()
            );
            cb.boxHeight = c["bh"].toDouble();
            r.chars.push_back(cb);
        }
        results.push_back(r);
    }
    return results;
}

std::optional<std::vector<OCRResult>> OcrCache::lookup(const QString& hash) {
    QMutexLocker lock(&m_mutex);
    QSqlDatabase db = getDb();
    if (!db.isOpen()) return std::nullopt;

    QSqlQuery q(db);
    q.prepare("SELECT result FROM ocr_cache WHERE hash = ?");
    q.addBindValue(hash);
    if (!q.exec() || !q.next()) return std::nullopt;

    QString json = q.value(0).toString();

    QSqlQuery upd(db);
    upd.prepare("UPDATE ocr_cache SET accessed_at = ? WHERE hash = ?");
    upd.addBindValue(QDateTime::currentSecsSinceEpoch());
    upd.addBindValue(hash);
    upd.exec();

    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) return std::nullopt;
    return deserializeResults(doc.array());
}

void OcrCache::store(const QString& hash, const std::vector<OCRResult>& results) {
    QMutexLocker lock(&m_mutex);
    QSqlDatabase db = getDb();
    if (!db.isOpen()) return;

    QJsonDocument doc(serializeResults(results));
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO ocr_cache (hash, result, accessed_at) VALUES (?, ?, ?)");
    q.addBindValue(hash);
    q.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();

    evictIfNeeded(db);
}

void OcrCache::evictIfNeeded(QSqlDatabase& db) {
    // Caller must hold m_mutex.
    QSqlQuery count(db);
    count.exec("SELECT COUNT(*) FROM ocr_cache");
    if (!count.next() || count.value(0).toInt() <= m_maxEntries) return;
    QSqlQuery del(db);
    del.exec("DELETE FROM ocr_cache WHERE hash = "
             "(SELECT hash FROM ocr_cache ORDER BY accessed_at ASC LIMIT 1)");
}

qint64 OcrCache::cacheSize() const {
    return QFileInfo(m_dbPath).size();
}

int OcrCache::cacheEntries() {
    QMutexLocker lock(&m_mutex);
    QSqlDatabase db = getDb();
    if (!db.isOpen()) return 0;
    QSqlQuery q(db);
    q.exec("SELECT COUNT(*) FROM ocr_cache");
    return q.next() ? q.value(0).toInt() : 0;
}

void OcrCache::clearCache() {
    QMutexLocker lock(&m_mutex);
    QSqlDatabase db = getDb();
    if (!db.isOpen()) return;
    QSqlQuery q(db);
    q.exec("DELETE FROM ocr_cache");
    q.exec("VACUUM");
}

void OcrCache::setMaxEntries(int n) {
    QMutexLocker lock(&m_mutex);
    m_maxEntries = qMax(1, n);
    QSqlDatabase db = getDb();
    if (db.isOpen())
        evictIfNeeded(db);
}
