#ifndef BLCDIALOG_H
#define BLCDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QMap>
#include <QPlainTextEdit>
#include "inc/rawinfodialog.h"

class gridImgLabel : public QLabel
{
    Q_OBJECT
public:
    gridImgLabel(QWidget* parent, QString gridFlag, QPixmap* const img);
    ~gridImgLabel(){}
    void setGridMode(const QString mode){ gridMode = mode;}
    void setShowImage(QPixmap* const im){ image = im; repaint();}
    const QString& getGridMode()const { return gridMode;}

protected:
    void paintEvent(QPaintEvent *e) override;
private:
    QPixmap* image;
    QString gridMode;
};

class BLCDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BLCDialog(QWidget *parent, const QMap<qint32, QStringList>& blc_map, rawinfoDialog::bayerMode bm, QSize rawsz, quint16 bd);
    ~BLCDialog();

protected slots:
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onNoGridToggled(bool statu);
    void onGrid5_5_Toggled(bool statu);
    void onGrid11_11_Toggled(bool statu);

private:
    QWidget* layoutWidget;
    QLabel* useTip;
    QTreeWidget* treeWgt;
    gridImgLabel* imgView;
    QGroupBox* grid_box;
    QRadioButton *noGrid, *grid5_5, *grid11_11;
    QPlainTextEdit* blcDataEdit;
    QHBoxLayout* hlayout;
    QMap<qint32, QStringList> blc_fn_map;
    rawinfoDialog::bayerMode bayerMode;
    QSize rawSize;
    quint16 bitDepth;
    quint8* showImgBuf;
    QPixmap showIm;

private:
    void showRawFile(const QString& rawfile);
};

#endif // BLCDIALOG_H
