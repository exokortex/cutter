#include <QCheckBox>
#include <QMenu>
#include <QAction>

#include "PPAnnotationsDialog.h"
#include "Cutter.h"
#include "plugins/ppCutter/core/PPCutterCore.h"
#include "ui_PPAnnotationsDialog.h"
#include <iostream>
#include "plugins/ppCutter/models/PPItemDelegate.h"

PPAnnotationsDialog::PPAnnotationsDialog(AddressType addr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PPAnnotationsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    setAddress(addr);
}

PPAnnotationsDialog::~PPAnnotationsDialog() {}

void PPAnnotationsDialog::on_buttonBox_accepted()
{
}

void PPAnnotationsDialog::on_buttonBox_rejected()
{
    close();
}

void PPAnnotationsDialog::setAddress(AddressType addr)
{
    ui->editorWidget->setAddress(addr);
    this->setWindowTitle(tr("Edit Annotation at %1").arg(RAddressString(addr)));
}
