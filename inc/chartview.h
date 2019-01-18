#ifndef CHARTVIEW_H
#define CHARTVIEW_H
#include <QtCharts/QChartView>
#include "chart.h"


class ChartView : public QChartView
{
public:
    ChartView(Chart* chart, QWidget* parent=0);
protected:
    void keyPressEvent(QKeyEvent* event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
private:
    qreal zoomFactor;
};

#endif // CHARTVIEW_H
