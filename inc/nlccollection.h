#ifndef NLCCOLLECTION_H
#define NLCCOLLECTION_H

#include <QString>
#include "rawinfodialog.h"

typedef struct ROI_s{
    quint32 left;
    quint32 right;
    quint32 top;
    quint32 bottom;
}ROI_t;

class nlccollection
{
    friend QDebug operator<<(QDebug d, const nlccollection& nl);//used for qDebug()<<
public:
    enum {R_IDX = 0, GR_IDX = 1, GB_IDX = 2, B_IDX = 3};
    nlccollection();
    nlccollection(QString rn, quint16 w, quint16 h, rawinfoDialog::bayerMode bm, quint16 bd);
    nlccollection(const nlccollection& nl);
    nlccollection& operator=(const nlccollection& nl);
    bool operator<(const nlccollection& nl)const;

    bool checkParameters();
    void calRoiAvgValue();
    qreal getAvgR()const{ return avg_r;}
    qreal getAvgGr()const{ return avg_gr;}
    qreal getAvgGb()const{ return avg_gb;}
    qreal getAvgB()const{ return avg_b;}
    const QString& getRawName()const {return rawName;}
    qreal getShut()const {return shut;}

    void setRoi(const ROI_t& r) {roi = r;}
    ROI_t getRoi()const {return roi;}
    int getISO()const { return iso;}

private:
    QString rawName;
    quint16 width, height;
    rawinfoDialog::bayerMode bm;
    quint16 bitDepth;
    ROI_t roi;
    qreal shut;
    int iso;
    qreal avg_r, avg_gr, avg_gb, avg_b;


    quint8 getBayerCh(quint16 x, quint16 y)const;
};
#endif // NLCCOLLECTION_H
