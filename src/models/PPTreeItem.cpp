#include <QStringList>

#include "PPTreeItem.h"

PPTreeItem::PPTreeItem(Type _type, PPTreeItem *parent, QString _key, const QString& value, ValueType valueType) :
  type(_type), m_parentItem(parent), key(_key), value(value), valueType(valueType)
{
  if (parent != nullptr)
    parent->appendChild(this);
}

PPTreeItem::~PPTreeItem()
{
  qDeleteAll(m_childItems);
}

void PPTreeItem::appendChild(PPTreeItem *item)
{
  m_childItems.append(item);
}

PPTreeItem *PPTreeItem::child(int row)
{
  return m_childItems.value(row);
}

int PPTreeItem::childCount() const
{
  return m_childItems.count();
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

PPTreeItem *PPTreeItem::parentItem()
{
  return m_parentItem;
}

int PPTreeItem::row() const
{
  if (m_parentItem)
    return m_parentItem->m_childItems.indexOf(const_cast<PPTreeItem*>(this));

  return 0;
}

void PPTreeItem::print(std::ostream& out)
{
  out << " T_" << type << "{";
  for (PPTreeItem* child : m_childItems) {
    child->print(out);
  }
  out << "} ";
}
