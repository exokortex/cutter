#include "AnnotationsEditorDockWidget.h"
#include "MainWindow.h"
#include "AnnotationsEditorWidget.h"
#include "plugins/ppCutter/core/PPCutterCore.h"

AnnotationsEditorDockWidget::AnnotationsEditorDockWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    seekable(new CutterSeekableWidget(this))
{
    this->setObjectName("annotationEditor");
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->widget = new AnnotationsEditorWidget(this);
    this->setWidget(widget);
    connect(seekable, &CutterSeekableWidget::seekChanged, this, &AnnotationsEditorDockWidget::onSeekChanged);
    onSeekChanged(0);
}

AnnotationsEditorDockWidget::~AnnotationsEditorDockWidget() {}

void AnnotationsEditorDockWidget::onSeekChanged(RVA addr) {
    widget->setAddress(addr);
    this->setWindowTitle(QString("Annotation Editor (%1)")
      .arg(PPCutterCore::addrToString(addr)));
}
