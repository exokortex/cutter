#ifndef ANNOTATIONSWIDGET_H
#define ANNOTATIONSWIDGET_H

#include <memory>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "Cutter.h"
#include "widgets/CutterDockWidget.h"

#include "pp/annotations/Annotation.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui {
class AnnotationsWidget;
}

class AnnotationsModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    QList<std::shared_ptr<Annotation>> *annotations;
    QMap<QString, QList<std::shared_ptr<Annotation>>> *nestedAnnotations;
    bool nested;

public:
    enum Column { OffsetColumn = 0, FunctionColumn, TypeColumn, DataColumn, ColumnCount };
    enum NestedColumn { OffsetNestedColumn = 0, TypeNestedColumn, DataNestedColumn, NestedColumnCount };
    enum Role { OffsetRole = Qt::UserRole, AnnotationDescriptionRole };

    AnnotationsModel(QList<std::shared_ptr<Annotation>> *annotations,
            QMap<QString, QList<std::shared_ptr<Annotation>>> *nestedAnnotations,
            QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadAnnotations();
    void endReloadAnnotations();

    bool isNested() const;
    void setNested(bool nested);
};

class AnnotationsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    AnnotationsProxyModel(AnnotationsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class AnnotationsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit AnnotationsWidget(MainWindow *main);
    ~AnnotationsWidget();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void on_AnnotationsTreeView_doubleClicked(const QModelIndex &index);

    void on_actionHorizontal_triggered();
    void on_actionVertical_triggered();

    void showTitleContextMenu(const QPoint &pt);

    void refreshTree();

private:
    std::unique_ptr<Ui::AnnotationsWidget> ui;
    MainWindow *main;

    AnnotationsModel *annotationsModel;
    AnnotationsProxyModel *annotationsProxyModel;

    QList<std::shared_ptr<Annotation>> annotations;
    QMap<QString, QList<std::shared_ptr<Annotation>>> nestedAnnotations;

    void setScrollMode();
};

#endif // ANNOTATIONSWIDGET_H
