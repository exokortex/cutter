#ifndef PPANNOTATIONSDIALOG_H
#define PPANNOTATIONSDIALOG_H

#include <QDialog>
#include <memory>

#include "ppCore/PPCutterCore.h"

#include "models/PPAnnotationDataModel.h"


namespace Ui
{
    class PPAnnotationsDialog;
}

class PPAnnotationsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PPAnnotationsDialog(AddressType addr, QWidget *parent = 0);
    ~PPAnnotationsDialog();

    void setAddress(AddressType addr);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void checkBoxStateChanged();

private:
    AddressType addr;

    std::unique_ptr<Ui::PPAnnotationsDialog> ui;

    PPAnnotationDataModel* dataModel;

    std::vector<QCheckBox*> checkBoxes;

    bool eventFilter(QObject *obj, QEvent *event);

    void expandAll();

    void addAnnotationTriggered(AnnotationType type);
};

#endif // PPANNOTATIONSDIALOG_H
