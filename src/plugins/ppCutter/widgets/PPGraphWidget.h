#ifndef PPGRAPHWIDGET_H
#define PPGRAPHWIDGET_H

#include "MemoryDockWidget.h"
#include <QLineEdit>

class MainWindow;
class DisassemblerGraphView;

class PPGraphWidget : public MemoryDockWidget
{
    Q_OBJECT

public:
    explicit PPGraphWidget(MainWindow *main);
    ~PPGraphWidget() override {}

    PPGraphView *getGraphView() const;

    static QString getWidgetType();

signals:
    void graphClosed();

protected:
    QWidget *widgetToFocusOnRaise() override;

private:
    void closeEvent(QCloseEvent *event) override;

    QString getWindowTitle() const override;
    void prepareHeader();

    PPGraphView *graphView;
    QLineEdit *header = nullptr;
};

#endif // PPGRAPHWIDGET_H
