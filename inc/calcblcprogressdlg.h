#ifndef CALCBLCPROGRESSDLG_H
#define CALCBLCPROGRESSDLG_H

#include <QDialog>

namespace Ui {
class CalcBlcProgressDlg;
}

class CalcBlcProgressDlg : public QDialog
{
    Q_OBJECT

public:
    CalcBlcProgressDlg(QWidget *parent, quint16 maxTask);
    ~CalcBlcProgressDlg();
public slots:
    void handleCurrentTaskId(quint16 taskId);

private:
    Ui::CalcBlcProgressDlg *ui;
};

#endif // CALCBLCPROGRESSDLG_H
