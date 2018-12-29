#ifndef BLCDIALOG_H
#define BLCDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QTreeWidget>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QMap>
#include <QPlainTextEdit>
#include "inc/rawinfodialog.h"
#include <qdom.h>

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
    BLCDialog(QWidget *parent, const QMap<qint32, QStringList>& blc_map, rawinfoDialog::bayerMode bm, QSize rawsz, quint16 bd);
    ~BLCDialog();
    qint32 onProgressDlgRun();

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
    QPushButton* saveXml;
    QDomDocument* xmlDoc;
    QDomElement docRoot;
    QHBoxLayout* hlayout;
    QMap<qint32, QStringList> blc_fn_map;
    rawinfoDialog::bayerMode bayerMode;
    QSize rawSize;
    quint16 bitDepth;
    quint8* showImgBuf;
    QPixmap showIm;

private:
    void showRawFile(const QString& rawfile);
    void createBlcDateNode(quint8 order,
                           quint16 aeGain,
                           quint16 R_blc_be,
                           quint16 Gr_blc_be,
                           quint16 Gb_blc_be,
                           quint16 B_blc_be,
                           QVector<quint16>& R_grid_val,
                           QVector<quint16>& Gr_grid_val,
                           QVector<quint16>& Gb_grid_val,
                           QVector<quint16>& B_grid_val);

    void addRaw2FourChannel(quint8* raw_buf, qreal* r_ch, qreal* gr_ch, qreal* gb_ch, qreal* b_ch);
    void avgBayerChannel(qreal* channel, qint32 frameNum);
    quint16 avgBlcValueBE(qreal* savgol_result, qint64 len);
    void calGridValue(qreal* savgol_result, qint64 row, qint64 col, QVector<quint16>& grid11_11);
};

#endif // BLCDIALOG_H
