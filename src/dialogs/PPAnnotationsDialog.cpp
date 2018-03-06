#include "PPAnnotationsDialog.h"
#include "ui_PPAnnotationsDialog.h"

PPAnnotationsDialog::PPAnnotationsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PPAnnotationsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Event filter for capturing Ctrl/Cmd+Return
    //ui->textEdit->installEventFilter(this);
}

PPAnnotationsDialog::~PPAnnotationsDialog() {}

void PPAnnotationsDialog::on_buttonBox_accepted()
{
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

void PPAnnotationsDialog::setAddress(AddressType addr)
{
    //ui->textEdit->document()->setPlainText(comment);
    this->addr = addr;
    PPAnnotation* oldAnnotation = PPCore()->getAnnotationAt(addr);
    if (oldAnnotation != nullptr) {
        annotation = std::unique_ptr<PPAnnotation>(new PPAnnotation(*oldAnnotation));
        // TODO use proper conversion
        ui->annotationTypeDropdown->setCurrentIndex((int)annotation->type);

        ui->dataTreeWidget->clear();
        for (auto it = annotation->data.begin(); it != annotation->data.end(); ++it)
        {
            QTreeWidgetItem *tempItem = new QTreeWidgetItem();
            std::string key = it.key();
            std::string value = it.value();
            tempItem->setText(0, QString::fromUtf8(key.c_str()));
            tempItem->setText(1, QString::fromUtf8(value.c_str()));
            //tempItem->setData(0, Qt::UserRole, QVariant::fromValue(xref));
            ui->dataTreeWidget->insertTopLevelItem(0, tempItem);
        }
    }
}

bool PPAnnotationsDialog::eventFilter(QObject */*obj*/, QEvent *event)
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
