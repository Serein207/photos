#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QPolygonF>
#include "ai/OCR.h"
#include "ai/TextLayoutAnalyzer.h"

class OcrResultModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum OcrRoles {
        TextRole = Qt::UserRole + 1,
        BoundingBoxRole,
        IsUrlRole,
        CharBoxesRole,
        LineIndexRole
    };

    explicit OcrResultModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    void setResults(const std::vector<OCRResult>& results);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        return m_results.count();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override {
        return {
            {TextRole, "text"},
            {BoundingBoxRole, "boundingBox"},
            {IsUrlRole, "isUrl"},
            {CharBoxesRole, "charBoxes"},
            {LineIndexRole, "lineIndex"}
        };
    }

    Q_INVOKABLE QVariantList allCharBoxes() const { return m_allCharBoxes; }

private:
    struct Item {
        QString text;
        QVariantList box;
        bool isUrl;
        QVariantList charBoxes;
        int lineIndex;
    };
    QList<Item> m_results;
    QVariantList m_allCharBoxes;
};
