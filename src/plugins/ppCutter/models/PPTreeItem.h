#pragma once

#include <QList>
#include <QVariant>
#include <memory>
#include "annotations/Annotation.h"
#include "annotations/CommentAnnotation.h"
#include "annotations/LoadRefAnnotation.h"

// adapted from https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html

class PPTreeItem
{
  public:
    enum Type { ROOT, ANNOTATION, LEAF};
    enum ValueType { STRING, ADDRESS, ENUM_UPDATE_TYPE, ENUM_INSTRUCTION_TYPE };

    explicit PPTreeItem(Type _type, PPTreeItem *parentItem, QString key,
                        const QVariant& value = "", ValueType valueType = STRING);
    ~PPTreeItem();

    void appendChild(PPTreeItem *child);
    void clearChildren();

    PPTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    PPTreeItem *getParentItem();
    bool removeChildren(int position, int count);
    void deleteData();

    void print(std::ostream& out);

    inline Type getType() {
      return type;
    }

    inline void setValue(const QVariant& _value) {
      value = _value;
    }

    inline ValueType getValueType() {
      return valueType;
    }

    inline bool isLeaf() {
      return type == LEAF;
    }

    inline void setAnnotationPtr(std::shared_ptr<Annotation> _annotation) {
      annotation = _annotation;
    }

    inline std::shared_ptr<Annotation> getAnnotationPtr() {
      return annotation;
    }

  private:
    Type type;
    PPTreeItem *parentItem;

    QString key;
    QVariant value;
    ValueType valueType;

    QList<PPTreeItem*> childItems;
    std::shared_ptr<Annotation> annotation;
};
