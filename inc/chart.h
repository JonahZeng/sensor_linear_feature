#ifndef CHART_H
#define CHART_H

#include <QtCharts/QChart>
using namespace QtCharts;

class Chart : public QChart
{
    Q_OBJECT
public:
    enum{
        R_PRESSED = 0,
        Gr_PRESSED = 1,
        Gb_PRESSED = 2,
        B_PRESSED = 3,
        NLC_SPECIFIC_LINE_PRESSED = 4,
        UN_PRESSED = -1
    };
    Chart();
    ~Chart();
    qint16 getSeriseNum();
    void setPressedSeriseNum(qint16 idx);
    quint32 getSeletedIdx() const {return seleted_idx;}

public slots:
    void reciveR_pressed(const QPointF& pt);
    void reciveGr_pressed(const QPointF& pt);
    void reciveGb_pressed(const QPointF& pt);
    void reciveB_pressed(const QPointF& pt);

    void reciveR_released(const QPointF& pt);
    void reciveGr_released(const QPointF& pt);
    void reciveGb_released(const QPointF& pt);
    void reciveB_released(const QPointF& pt);

private:
    bool r_flag, gr_flag, gb_flag, b_flag, nlc_specific_line_flag;
    quint32 seleted_idx;
};

#endif // CHART_H
