#include "OcrResultModel.h"
#include <algorithm>

void OcrResultModel::setResults(const std::vector<OCRResult>& results) {
    beginResetModel();
    m_results.clear();
    m_allCharBoxes.clear();

    auto ordered = TextLayoutAnalyzer::analyze(results);

    for (const auto& o : ordered) {
        const auto& r = results[o.originalIndex];
        QVariantList polyList;
        for (const auto& pt : r.box)
            polyList << QPointF(pt.x, pt.y);

        QVariantList charBoxList;
        for (const auto& cb : r.chars) {
            if (cb.ch.isNull() || cb.ch.isSpace()) continue;
            QVariantMap m;
            m["ch"] = QString(cb.ch);
            m["cx"] = cb.rect.center.x;
            m["cy"] = cb.rect.center.y;
            m["w"] = cb.rect.size.width;
            m["h"] = cb.rect.size.height;
            m["angle"] = cb.rect.angle;
            m["bh"] = cb.boxHeight;
            charBoxList << m;
        }

        m_results.append({
            QString::fromStdString(r.text),
            polyList,
            r.is_url,
            charBoxList,
            o.lineIndex
        });
    }

    int globalIdx = 0;
    for (const auto& item : m_results) {
        for (const auto& cb : item.charBoxes) {
            QVariantMap m = cb.toMap();
            m["globalIndex"] = globalIdx++;
            m["lineIndex"] = item.lineIndex;
            m_allCharBoxes << m;
        }
    }

    std::sort(m_allCharBoxes.begin(), m_allCharBoxes.end(), [](const QVariant& a, const QVariant& b) {
        auto ma = a.toMap(), mb = b.toMap();
        int la = ma["lineIndex"].toInt(), lb = mb["lineIndex"].toInt();
        if (la != lb) return la < lb;
        return ma["cx"].toFloat() < mb["cx"].toFloat();
    });
    for (int i = 0; i < m_allCharBoxes.size(); ++i) {
        QVariantMap m = m_allCharBoxes[i].toMap();
        m["globalIndex"] = i;
        m_allCharBoxes[i] = m;
    }

    endResetModel();
}

QVariant OcrResultModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_results.count()) return QVariant();

    const auto& item = m_results[index.row()];
    switch (role) {
        case TextRole: return item.text;
        case BoundingBoxRole: return QVariant::fromValue(item.box);
        case IsUrlRole: return item.isUrl;
        case CharBoxesRole: return item.charBoxes;
        case LineIndexRole: return item.lineIndex;
        default: return QVariant();
    }
}
