#include "inc\lineseries.h"
#include <QDebug>

LineSeries::LineSeries(QObject *parent):
    QLineSeries(parent)
{
    connect(this, &LineSeries::pressed, this, &LineSeries::onPressed);
}

void LineSeries::onPressed(QPointF point)
{
    qDebug()<<point<<endl;
}
