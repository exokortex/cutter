#include "ppCore/PPCutterCore.h"
#include "PPAnnotationDataModel.h"

#include "llvm/Support/Casting.h"

#include <iostream>
#include <memory>

#include <QStringList>
#include <QFontDatabase>

#include "ppCore/PPCutterCore.h"
#include "PPTreeItem.h"

PPAnnotationDataModel::PPAnnotationDataModel(QObject *parent, std::set<std::shared_ptr<Annotation>> annotations)
        : QAbstractItemModel(parent), rootItem(treeFromAnnotations(annotations))
{
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
            item->setValue(value.toString());
            return true;
        default:
            assert(false);

    }
}

PPTreeItem* PPAnnotationDataModel::treeFromAnnotations(std::set<std::shared_ptr<Annotation>> annotations)
{
    PPTreeItem* root = new PPTreeItem(PPTreeItem::Type::PPTI_ROOT, nullptr, "Root");
    for (auto&& annotation : annotations)
    {
        treeFromAnnotation(root, annotation);
    }
    return root;
}

PPTreeItem* PPAnnotationDataModel::treeFromAnnotation(PPTreeItem *parent, std::shared_ptr<Annotation> annotation)
{
    QString name = QString::fromStdString(PPCore()->annotationTypeToString(annotation->getType()));
    PPTreeItem* annotationItem = new PPTreeItem(PPTreeItem::Type::PPTI_ANNOTATION, parent, name);
    annotationItem->setAnnotationPtr(annotation);

    switch (annotation->getType()) {
      case AT_LOAD_REF:
      {
          const LoadRefAnnotation* a = llvm::dyn_cast<LoadRefAnnotation>(annotation.get());
          QString typeString = "";
          switch (a->updateType) {
            case UpdateType::INVALID:
              typeString = "INVALID";
              break;
            case UpdateType::CONSTANT_LOAD:
              typeString = "CONSTANT_LOAD";
              break;
            case UpdateType::SIGNATURE_LOAD:
              typeString = "SIGNATURE_LOAD";
              break;
            case UpdateType::CONST_INJECTION:
              typeString = "CONST_INJECTION";
              break;
          }
          new PPTreeItem(PPTreeItem::Type::PPTI_LEAF, annotationItem, "type", typeString, PPTreeItem::ValueType::ENUM_UPDATE_TYPE);
          new PPTreeItem(PPTreeItem::Type::PPTI_LEAF, annotationItem, "addr", PPCutterCore::addrToString(a->addrInstAddress), PPTreeItem::ValueType::ADDRESS);
          new PPTreeItem(PPTreeItem::Type::PPTI_LEAF, annotationItem, "data", PPCutterCore::addrToString(a->dataInstAddress), PPTreeItem::ValueType::ADDRESS);
          new PPTreeItem(PPTreeItem::Type::PPTI_LEAF, annotationItem, "apply", PPCutterCore::addrToString(a->applyInstAddress), PPTreeItem::ValueType::ADDRESS);
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

void PPAnnotationDataModel::addAnnotation(std::shared_ptr<Annotation> annotation)
{
    treeFromAnnotation(rootItem, annotation);
    rootItem->print(std::cout);
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

            switch (annotation->getType())
            {
                case AT_LOAD_REF:
                {
                    LoadRefAnnotation* a = llvm::dyn_cast<LoadRefAnnotation>(annotation.get());
                    if (key == "type") {
                        if (value =="INVALID") {
                          a->updateType = UpdateType::INVALID;
                        } else if (value == "CONSTANT_LOAD") {
                          a->updateType = UpdateType::INVALID;
                        } else if (value == "SIGNATURE_LOAD") {
                          a->updateType = UpdateType::SIGNATURE_LOAD;
                        } else if (value == "CONST_INJECTION") {
                          a->updateType = UpdateType::CONST_INJECTION;
                        }
                    } else if (key == "addr") {
                        a->addrInstAddress = PPCutterCore::strToAddress(value);
                    } else if (key == "data") {
                        a->dataInstAddress = PPCutterCore::strToAddress(value);
                    } else if (key == "apply") {
                        a->applyInstAddress = PPCutterCore::strToAddress(value);
                    }
                    break;
                }
                default:
                    assert(false);
            }
        }
    }
}

//PPTreeItem* PPAnnotationDataModel::treeFromJson(QString key, PPTreeItem* parent, json json)
//{
//    std::cout << "treeFromJson " << key.toUtf8().constData() << std::endl;
//    PPTreeItem* item = new PPTreeItem(key, parent);
//    if (json.is_object()) {
//        for (json::iterator it = json.begin(); it != json.end(); ++it) {
//            item->appendChild(treeFromJson(QString::fromStdString(it.key()), item, it.value()));
//        }
//    } else if (json.is_string()){
//        std::cout << "treeFromJson value " << json << std::endl;
//        item->setValue(QString::fromStdString(json.get<std::string>()));
//    } else {
//        std::cout << "Warning: Json contains unexpected data: " << std::endl;
//        std::cout << json.is_null() << json.is_boolean()<< json.is_number() << json.is_object() << json.is_array() << json.is_string() << std::endl;
//    }
//    return item;
//}
//
//json PPAnnotationDataModel::jsonFromTree(PPTreeItem* parent)
//{
//    if (parent->isLeaf()) {
//        std::cout << "leaf " << parent->data(1).toString().toStdString() << std::endl;
//        return parent->data(1).toString().toStdString();
//    } else {
//        std::cout << "obj " << parent->data(0).toString().toStdString() << std::endl;
//        json res;
//        for (int i = 0; i < parent->childCount(); i++) {
//            std::cout << "child"<< std::endl;
//            PPTreeItem* child = parent->child(i);
//            res[child->data(0).toString().toStdString()] = jsonFromTree(child);
//        }
//        return res;
//    }
//}
