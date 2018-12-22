#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QDir>
#include "inc\aboutdialog.h"
#include <QCloseEvent>
#include "inc\rawinfodialog.h"
#include "inc\nlccollection.h"
#include "inc\imglabel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void onPreButton();
    void onNextButton();
    void onFinishButton();
    void onSyncAllRoiButton();
    void recieveRoi(const ROI_t& roi);
private slots:
    void on_actionAbout_this_App_triggered();

    void on_actionOpenNLC_triggered();

    void on_actionExit_triggered();

    void on_actionAbout_Qt_triggered();

    void on_actionSave_triggered();
    void on_action_Open_BLC_raw_triggered();

protected:
    void closeEvent(QCloseEvent* event);
private:
    Ui::MainWindow *ui;
    QString preWorkPath;//记录上一次打开的目录
    AboutDialog* aboutThisApp;
    ImgLabel* imgLabel;
    //QImage showIm;
    quint8* rawImgPtr;
    QVector<nlccollection> nlc_vec;
    rawinfoDialog* rawinfoDlg;
    quint8 curRawIdx;
private:
    void handleRawInput(const rawinfoDialog& rawinfoDlg, const nlccollection& nl);
};

#endif // MAINWINDOW_H
