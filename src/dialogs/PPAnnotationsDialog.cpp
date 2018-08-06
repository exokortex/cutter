#include <QCheckBox>
#include <QMenu>
#include <QAction>

#include "PPAnnotationsDialog.h"
#include "Cutter.h"
#include "ppCore/PPCutterCore.h"
#include "ui_PPAnnotationsDialog.h"
#include <iostream>
#include "models/PPItemDelegate.h"

PPAnnotationsDialog::PPAnnotationsDialog(AddressType addr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PPAnnotationsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    this->addr = addr;
    std::set<std::shared_ptr<Annotation>> annotations = PPCore()->getFile().getAnnotationsAt(addr);
    dataModel = new PPAnnotationDataModel(this, annotations);
    ui->dataTreeView->setModel(dataModel);

    PPItemDelegate* delegate = new PPItemDelegate(ui->dataTreeView);
    ui->dataTreeView->setItemDelegate(delegate);

    QMenu *addMenu = new QMenu();
    ui->addButton->setMenu(addMenu);
    ui->addButton->setPopupMode(QToolButton::InstantPopup);

    for (auto& type : PPCore()->getAnnotationTypes()) {
        QAction *testAction = new QAction(QString::fromStdString(type.second), this);
        addMenu->addAction(testAction);
        connect(testAction, &QAction::triggered, [&]()
        {
          addAnnotationTriggered(type.first);
        });
    }

    expandAll();

    this->setWindowTitle(tr("Edit Annotation at %1").arg(RAddressString(addr)));
}

void PPAnnotationsDialog::addAnnotationTriggered(AnnotationType type)
{
    std::shared_ptr<Annotation> annotation = PPCore()->getFile().createAnnotation(type, addr);
    dataModel->addAnnotation(annotation);
    ui->dataTreeView->reset();
    expandAll();
}

void PPAnnotationsDialog::expandAll()
{
    QModelIndexList indexes = dataModel->match(dataModel->index(0,0), Qt::DisplayRole, "*", -1, Qt::MatchWildcard|Qt::MatchRecursive);
    foreach (QModelIndex index, indexes)
    ui->dataTreeView->expand(index);
}

void PPAnnotationsDialog::checkBoxStateChanged()
{
//    QCheckBox* button = qobject_cast<QCheckBox*>(sender());
//
//    if (!button)
//        return;
//
////    json data = dataModel->getJsonData();
////    std::string typeStr = button->text().toStdString();
////
////    std::cout << typeStr;
////
////    if (button->isChecked()) {
////        if (PPCore()->getDefaultAnnotationData().find(typeStr) != PPCore()->getDefaultAnnotationData().end()) {
////            std::cout << PPCore()->getDefaultAnnotationData()[typeStr] << std::endl;
////            std::cout << data << std::endl;
////            std::cout << "TYPEXXX: " << data.is_null() << data.is_boolean()<< data.is_number() << data.is_object() << data.is_array() << data.is_string() << std::endl;
////            data[typeStr] = PPCore()->getDefaultAnnotationData()[typeStr];
////        } else {
////            data[typeStr] = {};
////        };
////    } else if (data.find(typeStr) != data.end()) {
////        data.erase(typeStr);
////    }
////
////    dataModel->setJsonData(data);
//    ui->dataTreeView->reset();
//
//    expandAll();
}

PPAnnotationsDialog::~PPAnnotationsDialog() {}

void PPAnnotationsDialog::on_buttonBox_accepted()
{
    dataModel->save();
}

void PPAnnotationsDialog::on_buttonBox_rejected()
{
    close();
}

//QString PPAnnotationsDialog::getComment()
//{
//    //QString ret = ui->textEdit->document()->toPlainText();
//    return "not implemented";//ret;
//}

bool PPAnnotationsDialog::eventFilter(QObject */*obj*/, QEvent */*event*/)
{
//    if(event -> type() == QEvent::KeyPress)
//    {
//        QKeyEvent *keyEvent = static_cast <QKeyEvent*> (event);
//
//        // Confirm comment by pressing Ctrl/Cmd+Return
//        if((keyEvent -> modifiers() & Qt::ControlModifier) &&
//          ((keyEvent -> key() == Qt::Key_Enter) || (keyEvent -> key() == Qt::Key_Return)))
//        {
//            this->accept();
//            return true;
//        }
//    }

    return false;
}
