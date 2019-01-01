#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")  //fuck MSVC complior, use UTF-8, not gb2312/gbk
#endif

#include "inc/calcblcprogressdlg.h"
#include "ui_calcblcprogressdlg.h"

CalcBlcProgressDlg::CalcBlcProgressDlg(QWidget *parent, quint16 maxTask) :
    QDialog(parent),
    ui(new Ui::CalcBlcProgressDlg)
{
    ui->setupUi(this);
    ui->progressBar->setMaximum(maxTask);
    setWindowTitle(tr("BLC多帧计算进度..."));
}

CalcBlcProgressDlg::~CalcBlcProgressDlg()
{
    delete ui;
}

void CalcBlcProgressDlg::handleCurrentTaskId(quint16 taskId)
{
    if(isVisible()){
        qint32 maxTaskID = ui->progressBar->maximum();//如果任务数量完成，关闭对话框
        if(taskId==maxTaskID){
            accept();
        }
        if(taskId<maxTaskID){
            ui->progressBar->setValue(taskId);//进度条设置
        }
    }
}
