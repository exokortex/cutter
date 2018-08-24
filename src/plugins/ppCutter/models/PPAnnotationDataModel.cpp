#include "plugins/ppCutter/core/PPCutterCore.h"
#include "PPAnnotationDataModel.h"

#include "llvm/Support/Casting.h"

#include <iostream>
#include <memory>

#include <QStringList>
#include <QFontDatabase>

#include "plugins/ppCutter/core/PPCutterCore.h"
#include "PPTreeItem.h"

PPAnnotationDataModel::PPAnnotationDataModel(QObject *parent)
        : QAbstractItemModel(parent)
{
    rootItem = new PPTreeItem(PPTreeItem::Type::ROOT, nullptr, "Root");
}

PPAnnotationDataModel::~PPAnnotationDataModel()
{
    delete rootItem;
}

//void PPAnnotationDataModel::setJsonData(json data)
//{
//    // TODO inform the view about this before manipulating data
//    delete rootItem;
//    rootItem = treeFromJson("Annotations", nullptr, data);
//    std::cout << "PPAnnotationDataModel::setJsonData: " << data << std::endl;
//    std::cout << "PPAnnotationDataModel::setJsonData: " << jsonFromTree(rootItem) << std::endl;
//    emit dataChanged(QModelIndex(), QModelIndex());
//}

//json PPAnnotationDataModel::getJsonData()
//{
//  return jsonFromTree(rootItem);
//}

int PPAnnotationDataModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 2;
}

QVariant PPAnnotationDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::FontRole) {
        return QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::UserRole)
        return QVariant();

    PPTreeItem *item = getItem(index);

    if (role == Qt::UserRole) {
      return item->getValueType();
    } else {
      return item->data(index.column());
    }
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

PPTreeItem* PPAnnotationDataModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        PPTreeItem *item = static_cast<PPTreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
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
    PPTreeItem *parentItem = childItem->getParentItem();

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

    //std::cout << index.isValid() << " @ " << item << std::endl;

    if (!item)
        return 0;

    //std::cout << "rowCount " << item->childCount() << std::endl;
    return item->childCount();
}

bool PPAnnotationDataModel::setData(const QModelIndex &index, const QVariant &value,
             int /*role*/)
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
            item->setValue(value);
            save();
            return true;
        default:
            assert(false);

    }
}

PPTreeItem* PPAnnotationDataModel::treeFromAnnotation(PPTreeItem *parent, std::shared_ptr<Annotation> annotation)
{
    QString name = QString::fromStdString(PPCore()->toString(annotation->getType()));
    PPTreeItem* annotationItem = new PPTreeItem(PPTreeItem::Type::ANNOTATION, parent, name);
    annotationItem->setAnnotationPtr(annotation);

    new PPTreeItem(PPTreeItem::Type::LEAF, annotationItem, "address", PPCutterCore::addrToString(annotation->address), PPTreeItem::ValueType::ADDRESS);

    switch (annotation->getType()) {
        case Annotation::Type::COMMENT:
        {
            CommentAnnotation* a = llvm::dyn_cast<CommentAnnotation>(annotation.get());
            new PPTreeItem(PPTreeItem::Type::LEAF, annotationItem, "comment", QString::fromStdString(a->comment), PPTreeItem::ValueType::STRING);
            break;
        }
        case Annotation::Type::LOAD_REF:
        {
            const LoadRefAnnotation* a = llvm::dyn_cast<LoadRefAnnotation>(annotation.get());
            new PPTreeItem(PPTreeItem::Type::LEAF, annotationItem, "updateType", PPCutterCore::toString(a->updateType), PPTreeItem::ValueType::ENUM_UPDATE_TYPE);
            new PPTreeItem(PPTreeItem::Type::LEAF, annotationItem, "addrLoad", PPCutterCore::addrToString(a->addrLoad), PPTreeItem::ValueType::ADDRESS);
            new PPTreeItem(PPTreeItem::Type::LEAF, annotationItem, "dataLoad", PPCutterCore::addrToString(a->dataLoad), PPTreeItem::ValueType::ADDRESS);
            break;
        }
        default:
        {
            assert(false);
            break;
        }
    }
    return annotationItem;
}

void PPAnnotationDataModel::setAnnotations(std::set<std::shared_ptr<Annotation>> annotations)
{
    beginResetModel();
    rootItem->clearChildren();
    for (auto&& annotation : annotations) {
        treeFromAnnotation(rootItem, annotation);
    }
    endResetModel();
}

void PPAnnotationDataModel::addAnnotation(std::shared_ptr<Annotation> annotation)
{
    treeFromAnnotation(rootItem, annotation);
}

void PPAnnotationDataModel::save()
{
    for (int childIdx = 0; childIdx < rootItem->childCount(); childIdx++) {
        PPTreeItem* annotationItem = rootItem->child(childIdx);
        std::shared_ptr<Annotation> annotation = annotationItem->getAnnotationPtr();
        for (int dataIdx = 0; dataIdx < annotationItem->childCount(); dataIdx++) {
            PPTreeItem* valueItem = annotationItem->child(dataIdx);
            QString key = valueItem->data(0).toString();
            QString value = valueItem->data(1).toString();

            if (key == "address") {
                annotation->address = PPCutterCore::strToAddress(value);
                continue;
            }

            switch (annotation->getType())
            {
                case Annotation::Type::COMMENT:
                {
                    CommentAnnotation* a = llvm::dyn_cast<CommentAnnotation>(annotation.get());
                    if (key == "comment") {
                        a->comment = value.toStdString();
                    }
                    break;
                }
                case Annotation::Type::LOAD_REF:
                {
                    LoadRefAnnotation* a = llvm::dyn_cast<LoadRefAnnotation>(annotation.get());
                    if (key == "updateType") {
                        a->updateType = PPCutterCore::updateTypeFromString(value.toStdString());
                    } else if (key == "addrLoad") {
                        a->addrLoad = PPCutterCore::strToAddress(value);
                    } else if (key == "dataLoad") {
                        a->dataLoad = PPCutterCore::strToAddress(value);
                    }
                    break;
                }
                default:
                    assert(false);
            }
        }
    }

    PPCore()->registerAnnotationChange();
}

bool PPAnnotationDataModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    PPTreeItem *parentItem = getItem(parent);
    if (parentItem->getType() != PPTreeItem::Type::ROOT) {
        return removeRows(parent.row(), 1, parent.parent());
    }

    if (position < 0 || position + rows > parentItem->childCount())
        return false;

    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    for (int pos = position; pos < position + rows; pos++)
        PPCore()->getFile().deleteAnnotation(parentItem->child(pos)->getAnnotationPtr());
    PPCore()->getFile().createIndex();
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}
