#include "inc/chartview.h"
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

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

    if(series_press_flag < Chart::NLC_SPECIFIC_LINE_PRESSED && series_press_flag >= 0){
        QPoint global_p = event->pos();
        QPointF scene_p = mapToScene(global_p);
        QPointF chart_p = this->chart()->mapFromScene(scene_p);
        QPointF series_p =  this->chart()->mapToValue(chart_p, this->chart()->series().at(series_press_flag));
        //qDebug()<<series_p<<endl;
        quint32 idx = static_cast<Chart*>(this->chart())->getSeletedIdx();
        qreal origin_x = ((QLineSeries*)(this->chart()->series().at(series_press_flag)))->at(idx).x();
        ((QLineSeries*)(this->chart()->series().at(series_press_flag)))->replace(idx, origin_x, series_p.y()); //x不动，只动y值
    }
    else if(series_press_flag == Chart::NLC_SPECIFIC_LINE_PRESSED){
        QPoint global_p = event->pos();
        QPointF scene_p = mapToScene(global_p);
        QPointF chart_p = this->chart()->mapFromScene(scene_p);
        QPointF series_p =  this->chart()->mapToValue(chart_p, this->chart()->series().at(series_press_flag));
        qreal origin_0_x = ((QLineSeries*)(this->chart()->series().at(series_press_flag)))->at(0).x();
        qreal origin_1_x = ((QLineSeries*)(this->chart()->series().at(series_press_flag)))->at(1).x();
        QVector<QPointF> new_line = {QPointF(origin_0_x, series_p.ry()), QPointF(origin_1_x, series_p.ry())};
        ((QLineSeries*)(this->chart()->series().at(series_press_flag)))->replace(new_line);
    }
    else if(series_press_flag == Chart::UN_PRESSED)
        return;
}

void ChartView::mousePressEvent(QMouseEvent *event)//假如点击的点在
{
    QPoint global_p = event->pos();
    QPointF scene_p = mapToScene(global_p);
    QPointF chart_p = this->chart()->mapFromScene(scene_p);
    QPointF series_p = this->chart()->mapToValue(chart_p, this->chart()->series().at(Chart::NLC_SPECIFIC_LINE_PRESSED));
    QPointF first_p = ((QLineSeries*)(this->chart()->series().at(Chart::NLC_SPECIFIC_LINE_PRESSED)))->at(0);
    qreal nlc_specific_line_y = first_p.ry();
    qreal axisY_unit = (((QValueAxis*)this->chart()->axisY())->max() - ((QValueAxis*)this->chart()->axisY())->min())/100;
    if(series_p.ry()> nlc_specific_line_y-axisY_unit && series_p.ry()<nlc_specific_line_y+axisY_unit){
        ((Chart*)this->chart())->setPressedSeriseNum(Chart::NLC_SPECIFIC_LINE_PRESSED);
    }
    QChartView::mousePressEvent(event);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    ((Chart*)this->chart())->setPressedSeriseNum(Chart::UN_PRESSED);
    QChartView::mouseReleaseEvent(event);
}
