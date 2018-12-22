#include "inc\imglabel.h"
#include <QMouseEvent>
//#include <QDebug>
#include <QPainter>

ImgLabel::ImgLabel(QWidget* parent):
    QLabel(parent),
    imgFactor(1.0),
    imgOriginSize(0, 0),
    startPt(0,0),
    endPt(0,0),
    mousePressed(false),
    roi_painted(false)
{
    setAlignment(Qt::AlignCenter);//图片放在scrollarea中央
    setScaledContents(true);
}

ImgLabel::~ImgLabel()
{
    //delete pix;
}

void ImgLabel::setTmpPix(const QPixmap& p)
{
    tmpPix = p;
}

void ImgLabel::mousePressEvent(QMouseEvent* event)
{
    if(pixmap() &&  event->button() == Qt::LeftButton){
        mousePressed = true;
        startPt = QPoint(event->pos());
        roi_painted = false;//只要监测到鼠标左键按下，flag置false，直到完成一个release
    }
    else
        QLabel::mousePressEvent(event);
}

void ImgLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if(pixmap() && event->button() == Qt::LeftButton){
        mousePressed = false;
        endPt = event->pos();
        if(endPt == startPt)//roi只画了一个点，继续false
            return;
        update();
        if(roi_painted == false){            
            roi.top = quint32(startPt.y()/imgFactor);
            roi.left = quint32(startPt.x()/imgFactor);
            roi.bottom = quint32(endPt.y()/imgFactor);
            roi.right = quint32(endPt.x()/imgFactor);
            if(roi.top>roi.bottom){
                quint32 tmp = roi.top;
                roi.top = roi.bottom;
                roi.bottom = tmp;
            }
            if(roi.left>roi.right){
                quint32 tmp = roi.left;
                roi.left = roi.right;
                roi.right = tmp;
            }
            emit sendRoi(roi);//通知nlc_vec和table widget更新
            roi_painted = true;
        }
    }
    else
        QLabel::mouseReleaseEvent(event);
}

void ImgLabel::mouseMoveEvent(QMouseEvent *event)
{
    if(pixmap() && event->buttons() & Qt::LeftButton){
        endPt = event->pos();
        tmpPix = pix;//这里是上一次绘图的结果 复制到 缓冲层， 然后缓冲层绘矩形，防止连续的矩形出现；
        roi_painted = false;
        update();
    }
    else
        QLabel::mouseMoveEvent(event);
}

void ImgLabel::paintEvent(QPaintEvent *event)
{
    event->rect();
    QPainter p;
    if(mousePressed && !tmpPix.isNull()){//当鼠标按下，并且tmp准备好， 在绘图的过程中
        int width = endPt.x()-startPt.x();
        int height = endPt.y()-startPt.y();
        p.begin(&tmpPix);
        p.save();
        QPen pen(Qt::green, 2, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(1.0/imgFactor, 1.0/imgFactor);
        p.drawRect(QRectF(startPt.x(), startPt.y(), width, height));
        p.restore();
        p.end();

        p.begin(this);
        p.save();
        p.scale(imgFactor, imgFactor);
        p.drawPixmap(0,0, tmpPix);
        p.restore();
        p.end();
    }
    else if(!pix.isNull()){//鼠标松开，这个时候是update()调用引发paint, pix始终是最初的空白没有画矩形的pixmap;
        int width = roi.right-roi.left;
        int height = roi.bottom-roi.top;
        tmpPix = pix;
        p.begin(&tmpPix);
        p.save();
        if(width!=0 && height!=0 && roi_painted){
            QPen pen(Qt::green, 2, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(pen);
            p.setRenderHint(QPainter::Antialiasing);
            p.scale(1.0/imgFactor, 1.0/imgFactor);
            p.drawRect(QRectF(roi.left*imgFactor, roi.top*imgFactor, width*imgFactor, height*imgFactor));
        }
        p.restore();
        p.end();

        p.begin(this);
        p.save();
        p.scale(imgFactor, imgFactor);

        p.drawPixmap(0,0, tmpPix);
        p.restore();
        p.end();
    }

    //if(!mousePressed)//画完了以后，保留这一次的矩形，下一次绘画不删掉本次绘画
        //pix = tmpPix;
    //p.restore();
}

void ImgLabel::zoom_in()
{
    if(pixmap() && imgFactor<3){
        imgFactor += 0.25;
        resize(imgOriginSize*imgFactor);
        update();
    }
}

void ImgLabel::zoom_out()
{
    if(pixmap() && imgFactor>0.25){
        imgFactor -= 0.25;
        resize(imgOriginSize*imgFactor);
        update();
    }
}
