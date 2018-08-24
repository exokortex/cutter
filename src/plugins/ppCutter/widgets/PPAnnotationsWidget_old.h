#ifndef AnnotationsWidget_H
#define AnnotationsWidget_H

#include <memory>

#include "widgets/CutterDockWidget.h"
#include "pp/annotations/Annotation.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class AnnotationsWidget;
}

class AnnotationsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit AnnotationsWidget(MainWindow *main, QAction *action = nullptr);
    ~AnnotationsWidget();

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
    std::unique_ptr<Ui::AnnotationsWidget> ui;
    MainWindow      *main;
};

#endif // AnnotationsWidget_H
