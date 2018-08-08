#include "AnnotationsEditorWidget.h"
#include "ui_AnnotationsEditorWidget.h"
#include "Cutter.h"
#include "plugins/ppCutter/core/PPCutterCore.h"
#include "plugins/ppCutter/models/PPItemDelegate.h"

#include <QMenu>

AnnotationsEditorWidget::AnnotationsEditorWidget(QWidget *parent, AddressType addr) :
    QScrollArea(parent),
    ui(std::make_unique<Ui::AnnotationsEditorWidget>())
{
    ui->setupUi(this);

    // Item delegate for custom edit style
    PPItemDelegate* delegate = new PPItemDelegate(ui->annotationsTreeView);
    ui->annotationsTreeView->setItemDelegate(delegate);

    // create menu for add button
    QMenu *addMenu = new QMenu();
    ui->addButton->setMenu(addMenu);
    ui->addButton->setPopupMode(QToolButton::InstantPopup);

    // fill menu for add button
    for (auto& type : PPCore()->getAnnotationTypes()) {
        QAction *testAction = new QAction(QString::fromStdString(type.second), this);
        addMenu->addAction(testAction);
        connect(testAction, &QAction::triggered, [&]()
        {
          addAnnotationTriggered(type.first);
        });
    }

    dataModel = new PPAnnotationDataModel(this);
    ui->annotationsTreeView->setModel(dataModel);

    setAddress(addr);
}

AnnotationsEditorWidget::~AnnotationsEditorWidget()
{
}

void AnnotationsEditorWidget::setAddress(AddressType addr)
{
    this->addr = addr;

    if (!PPCore()->isReady())
        return;

    dataModel->setAnnotations(PPCore()->getFile().getAnnotationsAt(addr));
    expandAll();
}

void AnnotationsEditorWidget::expandAll()
{
    QModelIndexList indexes = dataModel->match(dataModel->index(0,0), Qt::DisplayRole, "*", -1, Qt::MatchWildcard|Qt::MatchRecursive);
    foreach (QModelIndex index, indexes)
    ui->annotationsTreeView->expand(index);
}


void AnnotationsEditorWidget::addAnnotationTriggered(AnnotationType type)
{
    std::shared_ptr<Annotation> annotation = PPCore()->getFile().createAnnotation(type, addr);
    dataModel->addAnnotation(annotation);
    ui->annotationsTreeView->reset();
    expandAll();
}
