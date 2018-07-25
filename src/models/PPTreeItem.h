#pragma once

#include <QList>
#include <QVariant>

// adapted from https://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html

class PPTreeItem
{
  public:
    explicit PPTreeItem(QString key, PPTreeItem *parentItem = 0);
    ~PPTreeItem();

    void appendChild(PPTreeItem *child);

    PPTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    PPTreeItem *parentItem();

    inline void setValue(const QString& _value) {
      value = _value;
      leaf = true;
    }

    inline bool isLeaf() {
      return leaf;
    }

  private:
    QList<PPTreeItem*> m_childItems;
    QString key;
    QString value;
    bool leaf = false;
    PPTreeItem *m_parentItem;
};
