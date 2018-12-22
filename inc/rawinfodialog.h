﻿#ifndef RAWINFODIALOG_H
#define RAWINFODIALOG_H

#include <QDialog>

namespace Ui {
class rawinfoDialog;
}

class rawinfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit rawinfoDialog(QWidget *parent = 0);
    ~rawinfoDialog();
    enum bayerMode{
        RG = 0,
        GR = 1,
        GB = 2,
        BG = 3,
        NO_BAYER = -1
    };
    /*enum inputStat{
        CANCEL = false,
        OK = true
    };*/

    //inline inputStat getInputFlag()const{
    //  return inputFlag;
    //}
    inline bayerMode getBayer()const{
        return bayer;
    }
    inline QSize getRawSize()const{
        return rawSize;
    }
    inline unsigned short getBitDepth()const{
        return bitDepth;
    }

private slots:
    void on_OkBtn_clicked();

    void on_CancelBtn_clicked();

private:
    Ui::rawinfoDialog *ui;
    //inputStat inputFlag;
    bayerMode bayer;
    QSize rawSize;
    quint16 bitDepth;
};

#endif // RAWINFODIALOG_H
