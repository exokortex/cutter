#include "MainWindow.h"
#include "PPGraphWidget.h"
#include "PPGraphView.h"

PPGraphWidget::PPGraphWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setObjectName("ppGraph");
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->graphView = new PPGraphView(this);
    this->setWidget(graphView);

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::PPGraph);
        }
    });

    connect(Core(), &CutterCore::raisePrioritizedMemoryWidget,
    this, [ = ](CutterCore::MemoryWidgetType type) {
        if (type == CutterCore::MemoryWidgetType::PPGraph) {
            this->raise();
            this->graphView->setFocus();
        }
    });
}

PPGraphWidget::~PPGraphWidget() {}
