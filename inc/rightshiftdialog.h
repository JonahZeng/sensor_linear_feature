#ifndef RIGHTSHIFTDIALOG_H
#define RIGHTSHIFTDIALOG_H

#include <QDialog>

namespace Ui {
class RightShiftDialog;
}

class RightShiftDialog : public QDialog
{
    Q_OBJECT

public:
    RightShiftDialog(QWidget *parent, quint8 right_shift_bit);
    ~RightShiftDialog();
    quint8 getRightShiftBit()const {return rst;}
private slots:
    void on_buttonOK_accepted();

private:
    Ui::RightShiftDialog *ui;
    quint8 rst;
};

#endif // RIGHTSHIFTDIALOG_H
