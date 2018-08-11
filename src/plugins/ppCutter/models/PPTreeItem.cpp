#include <QStringList>

#include "PPTreeItem.h"

PPTreeItem::PPTreeItem(Type _type, PPTreeItem *parent, QString _key, const QVariant& value, ValueType valueType) :
  type(_type), parentItem(parent), key(_key), value(value), valueType(valueType)
{
  if (parent != nullptr)
    parent->appendChild(this);
}

PPTreeItem::~PPTreeItem()
{
  qDeleteAll(childItems);
}

void PPTreeItem::appendChild(PPTreeItem *item)
{
  childItems.append(item);
}

void PPTreeItem::clearChildren()
{
  childItems.clear();
}

PPTreeItem *PPTreeItem::child(int row)
{
  return childItems.value(row);
}

int PPTreeItem::childCount() const
{
  return childItems.count();
}

int PPTreeItem::columnCount() const
{
  return 2;
}

QVariant PPTreeItem::data(int column) const
{
  if (column == 0)
    return key;
  else
    return value;
}

PPTreeItem *PPTreeItem::getParentItem()
{
  return parentItem;
}

int PPTreeItem::row() const
{
  if (parentItem)
    return parentItem->childItems.indexOf(const_cast<PPTreeItem*>(this));

  return 0;
}

bool PPTreeItem::removeChildren(int position, int count)
{
  if (position < 0 || position + count > childItems.size())
    return false;

  for (int row = 0; row < count; ++row) {
    delete childItems.takeAt(position);
  }

  return true;
}

void PPTreeItem::print(std::ostream& out)
{
  out << " T_" << type << "{";
  for (PPTreeItem* child : childItems) {
    child->print(out);
  }
  out << "} ";
}
