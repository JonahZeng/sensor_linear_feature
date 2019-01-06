#ifndef BLCDIALOG_H
#define BLCDIALOG_H

#include "inc/calcblcprogressdlg.h"
#include "inc/calcblcthread.h"
#include "inc/rawinfodialog.h"
#include "inc/blcxmlhighlight.h"
#include <QPushButton>
#include <QTreeWidget>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QMap>
#include <QPlainTextEdit>
#include <qdom.h>
#include <QSurface3DSeries>

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
    //qint32 onProgressDlgRun();

protected slots:
    void onTreeItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onNoGridToggled(bool statu);
    void onGrid5_5_Toggled(bool statu);
    void onGrid11_11_Toggled(bool statu);
    void onThreadDestroyed(QObject* obj=nullptr);
    void onSaveXmlButton();
    void onSaveAsButton();
    void onContextChanged();


private:
    QLabel* useTip;
    QTreeWidget* treeWgt;
    gridImgLabel* imgView;
    QGroupBox* grid_box;
    QRadioButton *noGrid, *grid5_5, *grid11_11;
    QPlainTextEdit* blcDataEdit;
    bool blcDataChanged;
    QString xmlFn;
    QString preWorkPath;
    QPushButton* saveXml;
    QPushButton* saveAsXml;
    QDomDocument* xmlDoc;
    QDomElement docRoot;
    QHBoxLayout* hlayout;
    QMap<qint32, QStringList> blc_fn_map;
    rawinfoDialog::bayerMode bayerMode;
    QSize rawSize;
    quint16 bitDepth;
    quint8* showImgBuf;
    QPixmap showIm;
    CalcBlcProgressDlg* clacBLCprogress;
    calcBlcThread* calcBLCthread;
    BlcXmlHighlight* xmlHL;

    Q3DSurface* threeDSurface;
    QWidget* surfaceContainerWgt;
    QRadioButton *surface_r, *surface_gr, *surface_gb, *surface_b;
    QMap<quint16, SurfaceDateArrP_4> aeGain_surfaceData_4p_map;

    QSurface3DSeries showOnSreenSeries;


private:
    void showRawFile(const QString& rawfile);

};

#endif // BLCDIALOG_H
