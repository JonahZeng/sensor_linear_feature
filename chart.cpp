#include "inc/chart.h"
#include <QtCharts/QLineSeries>
#include <math.h>

using namespace QtCharts;

Chart::Chart():
    QChart(),
    r_flag(false),
    gr_flag(false),
    gb_flag(false),
    b_flag(false),
    seleted_idx(0)
{

}

Chart::~Chart()
{

}

qint16 Chart::getSeriseNum()
{
    if(r_flag)
        return R_PRESSED;
    else if(gr_flag)
        return Gr_PRESSED;
    else if(gb_flag)
        return Gb_PRESSED;
    else if(b_flag)
        return B_PRESSED;
    else
        return UN_PRESSED;
}


void Chart::reciveR_pressed(const QPointF &pt)
{
    r_flag = true;
    gr_flag = gb_flag = b_flag = false;
    //线性查找， TODO 二分查找
    QVector<QPointF> line = ((QLineSeries*)(series().at(0)))->pointsVector();
    QVector<QPointF>::size_type idx = 0;
    if(pt.x()<line[0].x()){
        idx = 0;
    }
    else if(pt.x()>=line.last().x()){
        idx = line.size()-1;
    }
    else{
        for(; idx<line.size()-1; idx++){
            if(pt.x()>=line[idx].x() && pt.x()<line[idx+1].x())
                break;
        }

        qreal dist_1 = fabs(line[idx].x()-pt.x()) + fabs(line[idx].y()-pt.y());
        qreal dist_2 = fabs(line[idx+1].x()-pt.x()) + fabs(line[idx+1].y()-pt.y());

        idx = (dist_1>dist_2)?idx+1:idx;
    }
    seleted_idx = idx;
    //((QLineSeries*)(series().at(0)))->replace(idx, seletedP);
}

void Chart::reciveGr_pressed(const QPointF &pt)
{
    gr_flag = true;
    r_flag = gb_flag = b_flag = false;
    QVector<QPointF> line = ((QLineSeries*)(series().at(1)))->pointsVector();
    QVector<QPointF>::size_type idx = 0;
    if(pt.x()<line[0].x()){
        idx = 0;
    }
    else if(pt.x()>=line.last().x()){
        idx = line.size()-1;
    }
    else{
        for(; idx<line.size()-1; idx++){
            if(pt.x()>=line[idx].x() && pt.x()<line[idx+1].x())
                break;
        }

        qreal dist_1 = fabs(line[idx].x()-pt.x()) + fabs(line[idx].y()-pt.y());
        qreal dist_2 = fabs(line[idx+1].x()-pt.x()) + fabs(line[idx+1].y()-pt.y());

        idx = (dist_1>dist_2)?idx+1:idx;
    }
    seleted_idx = idx;
}

void Chart::reciveGb_pressed(const QPointF &pt)
{
    gb_flag = true;
    gr_flag = r_flag = b_flag = false;
    QVector<QPointF> line = ((QLineSeries*)(series().at(2)))->pointsVector();
    QVector<QPointF>::size_type idx = 0;
    if(pt.x()<line[0].x()){
        idx = 0;
    }
    else if(pt.x()>=line.last().x()){
        idx = line.size()-1;
    }
    else{
        for(; idx<line.size()-1; idx++){
            if(pt.x()>=line[idx].x() && pt.x()<line[idx+1].x())
                break;
        }

        qreal dist_1 = fabs(line[idx].x()-pt.x()) + fabs(line[idx].y()-pt.y());
        qreal dist_2 = fabs(line[idx+1].x()-pt.x()) + fabs(line[idx+1].y()-pt.y());

        idx = (dist_1>dist_2)?idx+1:idx;
    }
    seleted_idx = idx;
}

void Chart::reciveB_pressed(const QPointF &pt)
{
    b_flag = true;
    gr_flag = gb_flag = r_flag = false;
    QVector<QPointF> line = ((QLineSeries*)(series().at(3)))->pointsVector();
    QVector<QPointF>::size_type idx = 0;
    if(pt.x()<line[0].x()){
        idx = 0;
    }
    else if(pt.x()>=line.last().x()){
        idx = line.size()-1;
    }
    else{
        for(; idx<line.size()-1; idx++){
            if(pt.x()>=line[idx].x() && pt.x()<line[idx+1].x())
                break;
        }

        qreal dist_1 = fabs(line[idx].x()-pt.x()) + fabs(line[idx].y()-pt.y());
        qreal dist_2 = fabs(line[idx+1].x()-pt.x()) + fabs(line[idx+1].y()-pt.y());

        idx = (dist_1>dist_2)?idx+1:idx;
    }
    seleted_idx = idx;
}

void Chart::reciveR_released(const QPointF &pt)
{
    Q_UNUSED(pt);
    r_flag = false;
    //qDebug()<<"r released"<<pt<<endl;
}

void Chart::reciveGr_released(const QPointF &pt)
{
    Q_UNUSED(pt);
    gr_flag = false;
    //qDebug()<<"gr released"<<pt<<endl;
}

void Chart::reciveGb_released(const QPointF &pt)
{
    Q_UNUSED(pt);
    gb_flag = false;
    //qDebug()<<"gb released"<<pt<<endl;
}

void Chart::reciveB_released(const QPointF &pt)
{
    Q_UNUSED(pt);
    b_flag = false;
    //qDebug()<<"b released"<<pt<<endl;
}
