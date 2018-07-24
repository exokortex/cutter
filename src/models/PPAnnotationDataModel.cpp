#include "ppCore/PPCutterCore.h"
#include "PPAnnotationDataModel.h"

#include <iostream>
#include <QStringList>

#include "ppCore/PPCutterCore.h"

PPAnnotationDataModel::PPAnnotationDataModel(QObject *parent, json data)
        : QAbstractItemModel(parent), rootItem(treeFromJson("Annotations", nullptr, data))
{
    std::cout << "PPAnnotationDataModel::PPAnnotationDataModel: " << jsonFromTree(rootItem) << std::endl;
}

PPAnnotationDataModel::~PPAnnotationDataModel()
{
    delete rootItem;
}

void PPAnnotationDataModel::setJsonData(json data)
{
    // TODO inform the view about this before manipulating data
    delete rootItem;
    rootItem = treeFromJson("Annotations", nullptr, data);
    std::cout << "PPAnnotationDataModel::setJsonData: " << data << std::endl;
    std::cout << "PPAnnotationDataModel::setJsonData: " << jsonFromTree(rootItem) << std::endl;
    emit dataChanged(QModelIndex(), QModelIndex());
}

json PPAnnotationDataModel::getJsonData()
{
  return jsonFromTree(rootItem);
}

int PPAnnotationDataModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 2;
}

QVariant PPAnnotationDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    PPTreeItem *item = static_cast<PPTreeItem*>(index.internalPointer());

    if (!item)
        return QVariant();

    return item->data(index.column());
}

Qt::ItemFlags PPAnnotationDataModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

QVariant PPAnnotationDataModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case KeyColumn:
                return tr("Key");
            case ValueColumn:
                return tr("Value");
            default:
                return QVariant();
        }
    }

    return QVariant();
}

QModelIndex PPAnnotationDataModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    PPTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<PPTreeItem*>(parent.internalPointer());

    assert(parentItem);
    PPTreeItem *childItem = parentItem->child(row);
    assert(childItem != parentItem);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex PPAnnotationDataModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    PPTreeItem *childItem = static_cast<PPTreeItem*>(index.internalPointer());
    PPTreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int PPAnnotationDataModel::rowCount(const QModelIndex &index) const
{
    const PPTreeItem *item;
    if (index.column() > 0)
        return 0;

    if (index.isValid())
        item = static_cast<PPTreeItem*>(index.internalPointer());
    else
        item = rootItem;

    std::cout << index.isValid() << " @ " << item << std::endl;

    if (!item)
        return 0;

    std::cout << "rowCount " << item->childCount() << std::endl;
    return item->childCount();
}

bool PPAnnotationDataModel::setData(const QModelIndex &index, const QVariant &value,
             int role)
{
    if (!index.isValid())
        return false;

    PPTreeItem *item = static_cast<PPTreeItem*>(index.internalPointer());

    if (!item)
        return false;

    switch(index.column()) {
        case KeyColumn:
            return false;
        case ValueColumn:
            item->setValue(value.toString());
            return true;
        default:
            assert(false);

    }
}

PPTreeItem* PPAnnotationDataModel::treeFromJson(QString key, PPTreeItem* parent, json json)
{
    std::cout << "treeFromJson " << key.toUtf8().constData() << std::endl;
    PPTreeItem* item = new PPTreeItem(key, parent);
    if (json.is_object()) {
        for (json::iterator it = json.begin(); it != json.end(); ++it) {
            item->appendChild(treeFromJson(QString::fromStdString(it.key()), item, it.value()));
        }
    } else if (json.is_string()){
        std::cout << "treeFromJson value " << json << std::endl;
        item->setValue(QString::fromStdString(json.get<std::string>()));
    } else {
        std::cout << "Warning: Json contains unexpected data: " << std::endl;
        std::cout << json.is_null() << json.is_boolean()<< json.is_number() << json.is_object() << json.is_array() << json.is_string() << std::endl;
    }
    return item;
}

json PPAnnotationDataModel::jsonFromTree(PPTreeItem* parent)
{
    if (parent->isLeaf()) {
        std::cout << "leaf " << parent->data(1).toString().toStdString() << std::endl;
        return parent->data(1).toString().toStdString();
    } else {
        std::cout << "obj " << parent->data(0).toString().toStdString() << std::endl;
        json res;
        for (int i = 0; i < parent->childCount(); i++) {
            std::cout << "child"<< std::endl;
            PPTreeItem* child = parent->child(i);
            res[child->data(0).toString().toStdString()] = jsonFromTree(child);
        }
        return res;
    }
}
