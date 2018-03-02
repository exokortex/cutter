#include <QTreeWidget>
#include <QMenu>
#include <QResizeEvent>

#include "PPAnnotationsWidget.h"
#include "ui_PPAnnotationsWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

#include "PPCutterCore.h"
#include <pp/disassemblerstate.h>

PPAnnotationsWidget::PPAnnotationsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PPAnnotationsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->commentsTreeWidget->sortByColumn(2, Qt::AscendingOrder);

    QTabBar *tabs = ui->tabWidget->tabBar();
    tabs->setVisible(false);

    // Use a custom context menu on the dock title bar
    //this->title_bar = this->titleBarWidget();
    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));

    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshTree()));
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshTree()));

    // Hide the buttons frame
    ui->frame->hide();
}

PPAnnotationsWidget::~PPAnnotationsWidget() {}

void PPAnnotationsWidget::on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int)
{
    // Get offset and name of item double clicked
    PPAnnotation annotation = item->data(0, Qt::UserRole).value<PPAnnotation>();
    CutterCore::getInstance()->seek(annotation.offset);
}

void PPAnnotationsWidget::on_nestedCmtsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int)
{
    // don't react on top-level items
    if (item->parent() == nullptr)
    {
        return;
    }

    // Get offset and name of item double clicked
    PPAnnotation annotation = item->data(0, Qt::UserRole).value<PPAnnotation>();
    CutterCore::getInstance()->seek(annotation.offset);

}

void PPAnnotationsWidget::on_toolButton_clicked()
{
    ui->tabWidget->setCurrentIndex(0);
}

void PPAnnotationsWidget::on_toolButton_2_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
}

void PPAnnotationsWidget::showTitleContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (ui->tabWidget->currentIndex() == 0)
    {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    }
    else
    {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(this->mapToGlobal(pt));
    delete menu;
}

void PPAnnotationsWidget::on_actionHorizontal_triggered()
{
    ui->tabWidget->setCurrentIndex(0);
}

void PPAnnotationsWidget::on_actionVertical_triggered()
{
    ui->tabWidget->setCurrentIndex(1);
}



void PPAnnotationsWidget::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible())
    {
        if (event->size().width() >= event->size().height())
        {
            // Set horizontal view (list)
            on_actionHorizontal_triggered();
        }
        else
        {
            // Set vertical view (Tree)
            on_actionVertical_triggered();
        }
    }
    QDockWidget::resizeEvent(event);
}

/*
 *
QMap<QString, QList<QList<QString>>> CutterCore::getNestedComments()
{
    QMap<QString, QList<QList<QString>>> ret;
    QString comments = cmd("CC~CCu");

    for (QString line : comments.split("\n"))
    {
        QStringList fields = line.split("CCu");
        if (fields.length() == 2)
        {
            QList<QString> tmp = QList<QString>();
            tmp << fields[1].split("\"")[1].trimmed();
            tmp << fields[0].trimmed();
            QString fcn_name = this->cmdFunctionAt(fields[0].trimmed());
            ret[fcn_name].append(tmp);
        }
    }
    return ret;
}
 */

void PPAnnotationsWidget::refreshTree()
{
    ui->nestedCmtsTreeWidget->clear();
    //QMap<QString, QList<CommentDescription>> nestedComments;

    for (const PPAnnotation& annotation : PPCore()->getFile().annotations)
    {
        std::cout << "adding annotation..." << std::endl;
        std::string fname = "";
        const ::Function *f =  PPCore()->getState().getFunction(annotation.offset);
        if (f == nullptr)
            continue;
        for (auto& entrypoint : PPCore()->getState().getFunction(annotation.offset)->getEntryPoints())
            fname += entrypoint.name;
        QString fcn_name = QString::fromUtf8(fname.c_str());
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, RAddressString(annotation.offset));
        item->setText(1, fcn_name);
        item->setText(2, QString::fromUtf8(PPCutterCore::annotationTypeToString(annotation.type).c_str()));
        item->setText(3, QString::fromUtf8(annotation.data.dump().c_str()));
        item->setText(4, QString::fromUtf8(annotation.comment.c_str()));
        item->setData(0, Qt::UserRole, QVariant::fromValue(annotation));
        ui->commentsTreeWidget->addTopLevelItem(item);

        //nestedComments[fcn_name].append(comment);
    }
    std::cout << "refreshTree() done" << std::endl;
    qhelpers::adjustColumns(ui->commentsTreeWidget, 0);

    // Add nested comments
    //ui->nestedCmtsTreeWidget->clear();
    //for (auto functionName : nestedComments.keys())
    //{
    //    QTreeWidgetItem *item = new QTreeWidgetItem(ui->nestedCmtsTreeWidget);
    //    item->setText(0, functionName);
    //    for (CommentDescription comment : nestedComments.value(functionName))
    //    {
    //        QTreeWidgetItem *it = new QTreeWidgetItem();
    //        it->setText(0, RAddressString(comment.offset));
    //        it->setText(1, comment.name);
    //        it->setData(0, Qt::UserRole, QVariant::fromValue(comment));
    //        item->addChild(it);
    //    }
    //    ui->nestedCmtsTreeWidget->addTopLevelItem(item);
    //}
    //qhelpers::adjustColumns(ui->nestedCmtsTreeWidget, 0);
}
