#ifndef PPGRAPHWIDGET_H
#define PPGRAPHWIDGET_H

#include "CutterDockWidget.h"

class MainWindow;
class DisassemblerGraphView;

class PPGraphWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit PPGraphWidget(MainWindow *main, QAction *action = nullptr);
    ~PPGraphWidget();

private:
    PPGraphView *graphView;

};

#endif // PPGRAPHWIDGET_H
