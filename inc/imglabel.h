#ifndef IMGLABEL_H
#define IMGLABEL_H

#include <QLabel>
#include "nlccollection.h"

class ImgLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ImgLabel(QWidget* parent);

    ~ImgLabel();

    void setImgFactor(double f){ imgFactor = f;}
    double getImgFactor()const { return imgFactor;}

    void setImgOriginSize(const QSize& s){ imgOriginSize = s;}

    const ROI_t& getRoi()const{return roi;}
    void setRoi(ROI_t& r){ roi = r;}

    bool getPainted()const {return roi_painted;}
    void setPainted(bool pd){ roi_painted = pd;}

    void setTmpPix(const QPixmap& p);//clear temp paint buffer

    QPixmap pix;

signals:
    void sendRoi(const ROI_t& roi);

protected:
    void mousePressEvent(QMouseEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);

    void mouseMoveEvent(QMouseEvent *event);

    void paintEvent(QPaintEvent *event);

private:
    double imgFactor;
    QSize imgOriginSize;
    bool mousePressed;
    QPoint startPt;
    QPoint endPt;
    QPixmap tmpPix;
    bool roi_painted;
    ROI_t roi;

public slots:
    void zoom_in();

    void zoom_out();
};

#endif // IMGLABEL_H
