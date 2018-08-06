#ifndef PPCUTTER_PPANNOTATIONDATAMODEL_H
#define PPCUTTER_PPANNOTATIONDATAMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "PPTreeItem.h"

class PPAnnotationDataModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit PPAnnotationDataModel(QObject *parent, std::set<std::shared_ptr<Annotation>> annotations);
    ~PPAnnotationDataModel();

    enum Column { KeyColumn = 0, ValueColumn};

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    void addAnnotation(std::shared_ptr<Annotation> annotation);

    PPTreeItem* getItem(const QModelIndex &index) const;

    void save();

private:
    PPTreeItem* rootItem;

    static PPTreeItem* treeFromAnnotations(std::set<std::shared_ptr<Annotation>> annotations);
    static PPTreeItem* treeFromAnnotation(PPTreeItem* parent, std::shared_ptr<Annotation> annotation);
};

#endif //PPCUTTER_PPANNOTATIONDATAMODEL_H
