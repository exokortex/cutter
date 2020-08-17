#include "core/MainWindow.h"
#include "PPGraphWidget.h"
#include "PPGraphView.h"
#include "WidgetShortcuts.h"
#include <QVBoxLayout>

PPGraphWidget::PPGraphWidget(MainWindow *main) :
    MemoryDockWidget(MemoryWidgetType::Graph, main)
{
    setObjectName(main
                  ? main->getUniqueObjectName(getWidgetType())
                  : getWidgetType());

    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto *layoutWidget = new QWidget(this);
    setWidget(layoutWidget);
    auto *layout = new QVBoxLayout(layoutWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layoutWidget->setLayout(layout);

    header = new QLineEdit(this);
    header->setReadOnly(true);
    layout->addWidget(header);
    graphView = new PPGraphView(layoutWidget, seekable, main, {&syncAction});
    layout->addWidget(graphView);

    // Title needs to get set after graphView is defined
    updateWindowTitle();

    // getting the name of the class is implementation defined, and cannot be
    // used reliably across different compilers.
    //QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts[typeid(this).name()], main);
    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["PPGraphWidget"], main);
    connect(toggle_shortcut, &QShortcut::activated, this, [ = ]() {
            toggleDockWidget(true); 
    });

    connect(graphView, &PPGraphView::nameChanged, this, &MemoryDockWidget::updateWindowTitle);

    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        //main->toggleOverview(visibility, this);
        if (visibility) {
            graphView->onSeekChanged(Core()->getOffset());
        }
    });

    QAction *switchAction = new QAction(this);
    switchAction->setShortcut(Qt::Key_Space);
    switchAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(switchAction);
    connect(switchAction, &QAction::triggered, this, [this] {
        mainWindow->showMemoryWidget(MemoryWidgetType::Disassembly);
    });

    connect(graphView, &PPGraphView::graphMoved, this, [ = ]() {
        //main->toggleOverview(true, this);
    });
    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &PPGraphWidget::prepareHeader);
    connect(Core(), &CutterCore::functionRenamed, this, &PPGraphWidget::prepareHeader);
    graphView->installEventFilter(this);
}

QWidget *PPGraphWidget::widgetToFocusOnRaise()
{
    return graphView;
}

void PPGraphWidget::closeEvent(QCloseEvent *event)
{
    CutterDockWidget::closeEvent(event);
    emit graphClosed();
}

QString PPGraphWidget::getWindowTitle() const
{
    return graphView->windowTitle;
}

PPGraphView *PPGraphWidget::getGraphView() const
{
    return graphView;
}

QString PPGraphWidget::getWidgetType()
{
    return "Graph";
}

void PPGraphWidget::prepareHeader()
{
    QString afcf = Core()->cmdRawAt("afcf", seekable->getOffset()).trimmed();
    if (afcf.isEmpty()) {
        header->hide();
        return;
    }
    header->show();
    header->setText(afcf);
}

