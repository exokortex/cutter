#ifndef ANNOTATIONSEDITORWIDGET_H
#define ANNOTATIONSEDITORWIDGET_H

#include <QScrollArea>
#include <memory>

#include "pp/types.h"

#include "plugins/ppCutter/models/PPAnnotationDataModel.h"

namespace Ui {
class AnnotationsEditorWidget;
}

class AnnotationsEditorWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit AnnotationsEditorWidget(QWidget *parent = nullptr, AddressType addr = 0);
    ~AnnotationsEditorWidget();

    void setAddress(AddressType addr);
    void expandAll();

private:
    std::unique_ptr<Ui::AnnotationsEditorWidget> ui;

    AddressType addr;
    PPAnnotationDataModel* dataModel;

    void addAnnotationTriggered(AnnotationType type);
};

#endif // ANNOTATIONSEDITORWIDGET_H
