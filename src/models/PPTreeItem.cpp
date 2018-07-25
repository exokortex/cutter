#include <QStringList>

#include "PPTreeItem.h"

PPTreeItem::PPTreeItem(QString _key, PPTreeItem *parent) :
  key(_key), m_parentItem(parent)
{
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
