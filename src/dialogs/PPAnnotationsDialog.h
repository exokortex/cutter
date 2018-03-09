#ifndef PPANNOTATIONSDIALOG_H
#define PPANNOTATIONSDIALOG_H

#include <QDialog>
#include <memory>

#include "PPCutterCore.h"

#include "models/PPAnnotationDataModel.h"

class PPAnnotationDataModel;

namespace Ui
{
    class PPAnnotationsDialog;
}

class PPAnnotationsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PPAnnotationsDialog(QWidget *parent = 0);
    ~PPAnnotationsDialog();

    void setAddress(AddressType addr);

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    AddressType addr;
    PPAnnotation* annotation;

    std::unique_ptr<Ui::PPAnnotationsDialog> ui;

    bool eventFilter(QObject *obj, QEvent *event);

    PPAnnotationDataModel* dataModel;
};

#endif // PPANNOTATIONSDIALOG_H
