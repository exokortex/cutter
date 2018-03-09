#include "PPCutterCore.h"
#include "PPAnnotationDataModel.h"

#include <QStringList>

#include "PPCutterCore.h"

PPAnnotationDataModel::PPAnnotationDataModel(QObject *parent)
        : QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << "Key" << "Value";
    //rootItem = new PPAnnotation(rootData);
    //setupModelData(data.split(QString("\n")), rootItem);
}

PPAnnotationDataModel::~PPAnnotationDataModel() {}

int PPAnnotationDataModel::columnCount(const QModelIndex &parent) const
{
    return 2;
//    if (parent.isValid())
//        return static_cast<PPAnnotation*>(parent.internalPointer())->columnCount();
//    else
//        return rootItem->columnCount();
}

QVariant PPAnnotationDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    int idx = 0;
    for (auto it = annotation->data.begin(); it != annotation->data.end(); ++it)
    {
        if (idx == index.row()) {
            std::string key = it.key();
            std::string value = it.value();
            switch(index.column()) {
                case KeyColumn:
                    return QString::fromUtf8(key.c_str());
                case ValueColumn:
                    return QString::fromUtf8(value.c_str());
                default:
                    return "error";
            }
        }
        idx++;
    }

    PPAnnotation *item = static_cast<PPAnnotation*>(index.internalPointer());

    return "error 2";
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
    //if (!parent.isValid())
        return createIndex(row, column, (quintptr)0); // root function nodes have id = 0

    //return createIndex(row, column, (quintptr)(parent.row() + 1)); // sub-nodes have id = function index + 1
}

QModelIndex PPAnnotationDataModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 0)
        return QModelIndex();

    if (index.internalId() == 0) // root function node
        return QModelIndex();
    else // sub-node
        return this->index((int)(index.internalId() - 1), 0);
}

int PPAnnotationDataModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return annotation->data.size();

    return 0;
}

/*
void PPAnnotationDataModel::setupModelData(const QStringList &lines, PPAnnotation *parent)
{
    QList<PPAnnotation*> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].at(position) != ' ')
                break;
            position++;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QList<QVariant> columnData;
            for (int column = 0; column < columnStrings.count(); ++column)
                columnData << columnStrings[column];

            if (position > indentations.last()) {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            parents.last()->appendChild(new PPAnnotation(columnData, parents.last()));
        }

        ++number;
    }
}*/

bool PPAnnotationDataModel::setData(const QModelIndex &index, const QVariant &value,
             int role)
{
    int idx = 0;
    for (auto it = annotation->data.begin(); it != annotation->data.end(); ++it)
    {
        if (idx == index.row()) {
            std::string key = it.key();
            std::string jValue = it.value();
            switch(index.column()) {
                case KeyColumn:
                    return false;
                case ValueColumn:
                    annotation->data[key] = value.toString().toStdString();
                    PPCore()->registerAnnotationChange();
                    return true;
                default:
                    return "error";
            }
        }
        idx++;
    }
}

void PPAnnotationDataModel::setAnnotation(PPAnnotation *annotation)
{
    this->annotation = annotation;
}