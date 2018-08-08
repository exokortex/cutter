#ifndef PPANNOTATIONSDIALOG_H
#define PPANNOTATIONSDIALOG_H

#include <QDialog>
#include <memory>

#include "plugins/ppCutter/core/PPCutterCore.h"


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

private:
    std::unique_ptr<Ui::PPAnnotationsDialog> ui;
};

#endif // PPANNOTATIONSDIALOG_H
