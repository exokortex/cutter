#pragma once

#include <QList>
#include <QVariant>
#include <memory>
#include "pp/annotations.h"

// adapted from https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html

class PPTreeItem
{
  public:
    enum Type {PPTI_ROOT, PPTI_ANNOTATION, PPTI_LEAF};
    enum ValueType { STRING, ADDRESS, ENUM_UPDATE_TYPE };

    explicit PPTreeItem(Type _type, PPTreeItem *parentItem, QString key,
                        const QString& value = "", ValueType valueType = STRING);
    ~PPTreeItem();

    void appendChild(PPTreeItem *child);

    PPTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    PPTreeItem *parentItem();

    void print(std::ostream& out);

    inline void setValue(const QString& _value) {
      value = _value;
    }

    inline ValueType getValueType() {
      return valueType;
    }

    inline bool isLeaf() {
      return type == PPTI_LEAF;
    }

    inline void setAnnotationPtr(std::shared_ptr<Annotation> _annotation) {
      annotation = _annotation;
    }

    inline std::shared_ptr<Annotation> getAnnotationPtr() {
      return annotation;
    }

  private:
    Type type;
    PPTreeItem *m_parentItem;

    QString key;
    QString value;
    ValueType valueType;

    QList<PPTreeItem*> m_childItems;
    std::shared_ptr<Annotation> annotation;
};
