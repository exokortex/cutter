#ifndef PPANNOTATIONSWIDGET_H
#define PPANNOTATIONSWIDGET_H

#include <memory>

#include "widgets/CutterDockWidget.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class PPAnnotationsWidget;
}

class PPAnnotationsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit PPAnnotationsWidget(MainWindow *main, QAction *action = nullptr);
    ~PPAnnotationsWidget();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_nestedCmtsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_toolButton_clicked();
    void on_toolButton_2_clicked();

    void on_actionAddAnnotation_triggered();
    void on_actionHorizontal_triggered();
    void on_actionVertical_triggered();

    void showTitleContextMenu(const QPoint &pt);

    void refreshTree();

private:
    std::unique_ptr<Ui::PPAnnotationsWidget> ui;
    MainWindow      *main;
};

#endif // PPANNOTATIONSWIDGET_H
