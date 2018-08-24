#include "PPItemDelegate.h"
#include <QComboBox>
#include "PPTreeItem.h"

PPItemDelegate::PPItemDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
{
}

PPItemDelegate::~PPItemDelegate()
{
}

QWidget* PPItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (index.data(Qt::UserRole) == PPTreeItem::ValueType::ENUM_UPDATE_TYPE) {
    QComboBox* cb = new QComboBox(parent);
    cb->addItem(QString("INVALID"));
    cb->addItem(QString("CONSTANT_LOAD"));
    cb->addItem(QString("SIGNATURE_LOAD"));
    cb->addItem(QString("CONST_INJECTION"));
    return cb;
  } else if (index.data(Qt::UserRole) == PPTreeItem::ValueType::ENUM_INSTRUCTION_TYPE) {
    QComboBox* cb = new QComboBox(parent);
    cb->addItem(QString("UNKNOWN"));
    cb->addItem(QString("SEQUENTIAL"));
    cb->addItem(QString("DIRECT_CALL"));
    cb->addItem(QString("INDIRECT_CALL"));
    cb->addItem(QString("RETURN"));
    cb->addItem(QString("TRAP"));
    cb->addItem(QString("JUMP"));
    cb->addItem(QString("DIRECT_BRANCH"));
    cb->addItem(QString("INDIRECT_BRANCH"));
    return cb;
  }

  // create default editor
  return QStyledItemDelegate::createEditor(parent, option, index);
}

void PPItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  if (QComboBox* cb = qobject_cast<QComboBox*>(editor)) {
    // get the index of the text in the combobox that matches the current value of the itenm
    QString currentText = index.data(Qt::EditRole).toString();
    int cbIndex = cb->findText(currentText);
    // if it is valid, adjust the combobox
    if (cbIndex >= 0)
      cb->setCurrentIndex(cbIndex);
  } else {
    QStyledItemDelegate::setEditorData(editor, index);
  }
}

void PPItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  if (QComboBox* cb = qobject_cast<QComboBox*>(editor))
    // save the current text of the combo box as the current value of the item
    model->setData(index, cb->currentText(), Qt::EditRole);
  else
    QStyledItemDelegate::setModelData(editor, model, index);
}
