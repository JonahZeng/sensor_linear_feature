#ifndef CHARTDIALOG_H
#define CHARTDIALOG_H

#include <QDialog>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVector>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include "inc/chartview.h"
#include "Python.h"
#include <QCheckBox>

class SpinBox: public QSpinBox
{
    Q_OBJECT
public:
    SpinBox(QWidget* parent, bool isSt);
    ~SpinBox(){}
    void stepBy(int steps);
public:
    void setBro(SpinBox* b){bro=b;}
private:
    SpinBox* bro;
    bool isStart;
};

class ChartDialog : public QDialog
{
    Q_OBJECT

public:
    ChartDialog(QWidget *parent,
                const QVector<QPointF>& lineR,
                const QVector<QPointF>& lineGr,
                const QVector<QPointF>& lineGb,
                const QVector<QPointF>& lineB,
                int nlc_size,
                int ISO);
    ~ChartDialog();

public slots:
    void polyfit1();
    void save2XML();
    void pfStartChangeR(int v);
    void pfEndChangeR(int v);
    void pfStartChangeGr(int v);
    void pfEndChangeGr(int v);
    void pfStartChangeGb(int v);
    void pfEndChangeGb(int v);
    void pfStartChangeB(int v);
    void pfEndChangeB(int v);
    void showR(bool checked);
    void showGr(bool checked);
    void showGb(bool checked);
    void showB(bool checked);
    void onResetR();
    void onResetGr();
    void onResetGb();
    void onResetB();


private:
    //QPointF R_pfSt, R_pfEnd, Gr_pfSt, Gr_pfEnd, Gb_pfSt, Gb_pfEnd, B_pfSt, B_pfEnd;
    QScatterSeries *R_pf_2pt, *Gr_pf_2pt, *Gb_pf_2pt, *B_pf_2pt;
    QLineSeries *R, *Gr, *Gb, *B;
    QLineSeries *R_pf, *Gr_pf, *Gb_pf, *B_pf;
    QLineSeries* nlc_specific_line;
    Chart* chart;
    QHBoxLayout* hlayout;
    QGridLayout* glayout;
    ChartView* chartView;
    SpinBox* polyfitStartR;
    SpinBox* polyfitEndR;
    SpinBox* polyfitStartGr;
    SpinBox* polyfitEndGr;
    SpinBox* polyfitStartGb;
    SpinBox* polyfitEndGb;
    SpinBox* polyfitStartB;
    SpinBox* polyfitEndB;
    QCheckBox* R_visual, *Gr_visual, *Gb_visual, *B_visual;
    QGroupBox* resetGpBox;
    QPushButton* reset_R, *reset_Gr, *reset_Gb, *reset_B;
    QPushButton* polyfitBtn;
    QPushButton* saveBtn;
    QString prevPath;
    PyObject* pModule;
    int iso;
    bool polyfit_success;
    const QVector<QPointF> origin_R;
    const QVector<QPointF> origin_Gr;
    const QVector<QPointF> origin_Gb;
    const QVector<QPointF> origin_B;

private:
    qreal minQPointsX(const QVector<QPointF>& line) const;
    qreal maxQPointsX(const QVector<QPointF>& line) const;
    qreal minQPointsY(const QVector<QPointF>& line) const;
    qreal maxQPointsY(const QVector<QPointF>& line) const;
    void cal4ChannelPolyfitCurve(const qreal k1, const qreal b1, const qreal k2, const qreal b2, const qreal k3, const qreal b3, const qreal k4, const qreal b4);

    void createUI(int nlc_size);

    qreal getParameter(quint16 in, QVector<qreal>& x, QVector<qreal>& y, int size);
};

#endif // CHARTDIALOG_H
