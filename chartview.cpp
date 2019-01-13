#include "inc/chartview.h"
#include <QtCharts/QLineSeries>

ChartView::ChartView(Chart *chart, QWidget *parent):
    QChartView(chart, parent),
    zoomFactor(1.0)
{

}

void ChartView::keyPressEvent(QKeyEvent *event)
{
    if(event->key()==Qt::Key_Plus /*&& event->modifiers() == Qt::ControlModifier*/){
        this->chart()->zoomIn();
        zoomFactor *= 2;
        return;
    }
    if(event->key()==Qt::Key_Minus /*&& event->modifiers() == Qt::ControlModifier*/){
        this->chart()->zoomOut();
        zoomFactor /= 2;
        return;
    }
    if(event->key()==Qt::Key_Up){
        this->chart()->scroll(0, -15);
        return;
    }
    if(event->key()==Qt::Key_Down){
        this->chart()->scroll(0, 15);
        return;
    }
    if(event->key()==Qt::Key_Left){
        this->chart()->scroll(15, 0);
        return;
    }
    if(event->key()==Qt::Key_Right){
        this->chart()->scroll(-15, 0);
        return;
    }
    QChartView::keyPressEvent(event);
}

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    qint16 series_press_flag = ((Chart*)this->chart())->getSeriseNum();
    if(series_press_flag == Chart::UN_PRESSED)
        return;

    QPoint global_p = event->pos();
    QPointF scene_p = mapToScene(global_p);
    QPointF chart_p = this->chart()->mapFromScene(scene_p);
    QPointF series_p =  this->chart()->mapToValue(chart_p, this->chart()->series().at(series_press_flag));
    //qDebug()<<series_p<<endl;
    quint32 idx = static_cast<Chart*>(this->chart())->getSeletedIdx();
    qreal origin_x = ((QLineSeries*)(this->chart()->series().at(series_press_flag)))->at(idx).x();
    ((QLineSeries*)(this->chart()->series().at(series_press_flag)))->replace(idx, origin_x, series_p.y()); //x不动，只动y值
}
