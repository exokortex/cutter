#include <QMenu>
#include <QResizeEvent>
#include <QShortcut>

#include "AnnotationsWidget.h"
#include "ui_AnnotationsWidget.h"
#include "MainWindow.h"
#include "common/Helpers.h"
#include "plugins/ppCutter/core/PPCutterCore.h"

AnnotationsModel::AnnotationsModel(QList<std::shared_ptr<Annotation>> *Annotations,
                             QMap<QString, QList<std::shared_ptr<Annotation>>> *nestedAnnotations,
                             QObject *parent)
    : QAbstractItemModel(parent),
      annotations(Annotations),
      nestedAnnotations(nestedAnnotations),
      nested(false)
{}

bool AnnotationsModel::isNested() const
{
    return nested;
}

void AnnotationsModel::setNested(bool nested)
{
    beginResetModel();
    this->nested = nested;
    endResetModel();
}

QModelIndex AnnotationsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, (quintptr)0);

    return createIndex(row, column, (quintptr)(parent.row() + 1));
}

QModelIndex AnnotationsModel::parent(const QModelIndex &index) const
{
    /* Ignore invalid indexes and root nodes */
    if (!index.isValid() || index.internalId() == 0)
        return QModelIndex();

    return this->index((int)(index.internalId() - 1), 0);
}

int AnnotationsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return (isNested() ? nestedAnnotations->size() : annotations->count());

    if (isNested() && parent.internalId() == 0) {
        QString fcnName = nestedAnnotations->keys().at(parent.row());
        return nestedAnnotations->operator[](fcnName).size();
    }

    return 0;
}

int AnnotationsModel::columnCount(const QModelIndex&) const
{
    return (isNested()
            ? static_cast<int>(AnnotationsModel::NestedColumnCount)
            : static_cast<int>(AnnotationsModel::ColumnCount));
}

QVariant AnnotationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (index.internalId() != 0 && !index.parent().isValid()))
        return QVariant();

    int commentIndex;
    bool isSubnode;
    if (index.internalId() != 0) {
        /* Subnode */
        commentIndex = index.parent().row();
        isSubnode = true;
    } else {
        /* Root node */
        commentIndex = index.row();
        isSubnode = false;
    }

    QString offset;
    std::shared_ptr<Annotation> annotation;
    if (isNested()) {
        offset = nestedAnnotations->keys().at(commentIndex);
        if (isSubnode) {
            annotation = nestedAnnotations->operator[](offset).at(index.row());
        }
    } else {
        annotation = annotations->at(commentIndex);
    }

    switch (role)
    {
    case Qt::DisplayRole:
        if (isNested()) {
            if (isSubnode) {
                switch (index.column()) {
                case OffsetNestedColumn:
                    return RAddressString(annotation->address);
                case TypeNestedColumn:
                    return QString::fromStdString(PPCore()->toString(annotation->getType()));
                case DataNestedColumn:
                    return PPCore()->annotationDataToString(annotation.get());
                default:
                    break;
                }
            } else if (index.column() == OffsetNestedColumn) {
                return offset;
            }
        } else {
            switch (index.column()) {
            case AnnotationsModel::OffsetColumn:
                return RAddressString(annotation->address);
            case AnnotationsModel::FunctionColumn:
                return QString::fromStdString(PPCore()->getFile().getFunctionAt(annotation->address)->getJoinedName());
            case TypeColumn:
                return QString::fromStdString(PPCore()->toString(annotation->getType()));
            case DataColumn:
                return PPCore()->annotationDataToString(annotation.get());
            default:
                break;
            }
        }
        break;
    case AnnotationsModel::OffsetRole:
        if (isNested() && index.internalId() == 0) {
            break;
        }
        return RAddressString(annotation->address);
    case AnnotationsModel::AnnotationDescriptionRole:
        if (isNested() && index.internalId() == 0) {
            break;
        }
        return RAddressString(annotation->address) + " " + PPCore()->annotationDataToString(annotation.get());
    default:
        break;
    }

    return QVariant();
}

QVariant AnnotationsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (isNested()) {
            switch (section) {
            case AnnotationsModel::OffsetNestedColumn:
                return tr("Function/Offset");
            case TypeNestedColumn:
                return tr("Type");
            case DataNestedColumn:
                return tr("Data");
            default:
                break;
            }
        } else {
            switch (section) {
            case AnnotationsModel::OffsetColumn:
                return tr("Offset");
            case AnnotationsModel::FunctionColumn:
                return tr("Function");
            case TypeColumn:
                return tr("Type");
            case DataColumn:
                return tr("Data");
            default:
                break;
            }
        }
    }

    return QVariant();
}

void AnnotationsModel::beginReloadAnnotations()
{
    beginResetModel();
}

void AnnotationsModel::endReloadAnnotations()
{
    endResetModel();
}

AnnotationsProxyModel::AnnotationsProxyModel(AnnotationsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool AnnotationsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    AnnotationsModel *srcModel = static_cast<AnnotationsModel *>(sourceModel());
    if (srcModel->isNested()) {
        // Disable filtering
        return true;
    }

    QModelIndex index = sourceModel()->index(row, 0, parent);
    QString description = index.data(AnnotationsModel::AnnotationDescriptionRole).toString();

    return description.contains(filterRegExp());
}

bool AnnotationsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    AnnotationsModel *srcModel = static_cast<AnnotationsModel *>(sourceModel());
    if (srcModel->isNested()) {
        // Disable sorting
        return false;
    }

    if (!left.isValid() || !right.isValid())
        return false;

    if (left.parent().isValid() || right.parent().isValid())
        return false;

//    auto leftComment = left.data(AnnotationsModel::CommentDescriptionRole).value<std::shared_ptr<Annotation>>();
//    auto rightComment = right.data(AnnotationsModel::CommentDescriptionRole).value<std::shared_ptr<Annotation>>();
//
//    switch (left.column()) {
//        case AnnotationsModel::OffsetColumn:
//        {
//            auto leftComment = left.data(AnnotationsModel::OffsetRole).value<QInteger>();
//            auto rightComment = right.data(AnnotationsModel::OffsetRole).value<QString>();
//            return leftComment->address < rightComment->address;
//        }
//        case AnnotationsModel::FunctionColumn:
//            return Core()->cmdFunctionAt(leftComment->address) < Core()->cmdFunctionAt(rightComment->address);
//        case AnnotationsModel::DataColumn:
//            return false; // TODO leftComment->name < rightComment.name;
//        default:
//            break;
//    }

    return false;
}

AnnotationsWidget::AnnotationsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::AnnotationsWidget),
    main(main)
{
    ui->setupUi(this);

    annotationsModel = new AnnotationsModel(&annotations, &nestedAnnotations, this);
    annotationsProxyModel = new AnnotationsProxyModel(annotationsModel, this);
    ui->AnnotationsTreeView->setModel(annotationsProxyModel);
    ui->AnnotationsTreeView->sortByColumn(AnnotationsModel::OffsetColumn, Qt::AscendingOrder);

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)),
            annotationsProxyModel, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->AnnotationsTreeView, SLOT(setFocus()));

    setScrollMode();

    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));

    connect(PPCore(), SIGNAL(annotationsChanged()), this, SLOT(refreshTree()));
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshTree()));
}

AnnotationsWidget::~AnnotationsWidget() {}

void AnnotationsWidget::on_AnnotationsTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    if (annotationsModel->isNested() && !index.parent().isValid())
        return;

    QString data = index.data(AnnotationsModel::OffsetRole).toString();
    Core()->seek(data);
}

void AnnotationsWidget::on_actionHorizontal_triggered()
{
    annotationsModel->setNested(false);
    ui->AnnotationsTreeView->setIndentation(8);
}

void AnnotationsWidget::on_actionVertical_triggered()
{
    annotationsModel->setNested(true);
    ui->AnnotationsTreeView->setIndentation(20);
}

void AnnotationsWidget::showTitleContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (!annotationsModel->isNested()) {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    } else {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(this->mapToGlobal(pt));
    delete menu;
}

void AnnotationsWidget::resizeEvent(QResizeEvent *event)
{
    if (main->responsive && isVisible()) {
        if (event->size().width() >= event->size().height()) {
            // Set horizontal view (list)
            on_actionHorizontal_triggered();
        } else {
            // Set vertical view (Tree)
            on_actionVertical_triggered();
        }
    }
    QDockWidget::resizeEvent(event);
}

void AnnotationsWidget::refreshTree()
{
    annotationsModel->beginReloadAnnotations();

    annotations.clear();
    annotations.reserve(PPCore()->getFile().annotations.size());
    std::copy(PPCore()->getFile().annotations.begin(),
              PPCore()->getFile().annotations.end(),
              std::back_inserter(annotations));

    nestedAnnotations.clear();
    for (std::shared_ptr<Annotation> annotation : annotations) {
        QString fcnName = Core()->cmdFunctionAt(annotation->address);
        nestedAnnotations[fcnName].append(annotation);
    }

    annotationsModel->endReloadAnnotations();

    qhelpers::adjustColumns(ui->AnnotationsTreeView, 3, 0);
}

void AnnotationsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->AnnotationsTreeView);
}
