#ifndef INCLINESERIES_H
#define INCLINESERIES_H

#include <QLineSeries>
#include <QPointF>

using namespace QtCharts;

class LineSeries : public QLineSeries
{
    Q_OBJECT

public:
    LineSeries(QObject *parent = nullptr);
    ~LineSeries(){}
protected slots:
    void onPressed(QPointF point);
};

#endif // INCLINESERIES_H
