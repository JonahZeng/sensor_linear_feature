#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")  //fuck MSVC complior, use UTF-8, not gb2312/gbk
#endif
#include "inc/blcdialog.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QPainter>
#include <QPaintEvent>


BLCDialog::BLCDialog(QWidget *parent, const QMap<qint32, QStringList>& blc_map, rawinfoDialog::bayerMode bm, QSize rawsz, quint16 bd) :
    QDialog(parent),
    layoutWidget(new QWidget(this)),
    useTip(new QLabel(tr("<font size='+1'><p>双击下面的raw文件名切换</p><p>目前只打算兼容输出</p><p><b>V300和V250</b>的blc xml</p></font>"), layoutWidget)),
    treeWgt(new QTreeWidget(layoutWidget)),
    imgView(new gridImgLabel(layoutWidget, "0x0", NULL)),
    grid_box(new QGroupBox(tr("grid option"), layoutWidget)),
    noGrid(new QRadioButton(tr("no grid"), layoutWidget)),
    grid5_5(new QRadioButton(tr("grid 5x5"), layoutWidget)),
    grid11_11(new QRadioButton(tr("grid 11x11"), layoutWidget)),
    blcDataEdit(new QPlainTextEdit(layoutWidget)),
    hlayout(new QHBoxLayout(layoutWidget)),
    blc_fn_map(blc_map),
    bayerMode(bm),
    rawSize(rawsz),
    bitDepth(bd),
    showImgBuf(NULL)
{
    resize(1024, 600);
    hlayout->setSpacing(6);
    hlayout->setContentsMargins(11,11,11,11);

    treeWgt->setColumnCount(1);
    treeWgt->setMinimumWidth(250);
    treeWgt->setTextElideMode(Qt::ElideMiddle);
    QString header = "ISO/rawfile";
    treeWgt->setHeaderLabel(header);
    quint16 root_id = 0;
    for(QMap<qint32, QStringList>::iterator it=blc_fn_map.begin(); it!=blc_fn_map.end(); it++, root_id++){
        QTreeWidgetItem* item = new QTreeWidgetItem(treeWgt);
        item->setText(0, QString("ISO")+QString::number(it.key(), 10));
        QStringList rawList = it.value();
        for(QStringList::iterator str_it= rawList.begin(); str_it!=rawList.end(); str_it++){
            QTreeWidgetItem* child = new QTreeWidgetItem();
            QFileInfo rawinfo(*str_it);
            QString fn = rawinfo.fileName();
            child->setText(0, fn);
            child->setToolTip(0, fn);
            item->addChild(child);
            if(it==blc_fn_map.begin() && str_it==rawList.begin())
                child->setSelected(true);
            else
                child->setSelected(false);
        }
        treeWgt->insertTopLevelItem(root_id, item);
    }
    treeWgt->expandAll();
    treeWgt->setFocusPolicy(Qt::ClickFocus);

    QVBoxLayout* leftVerLayout = new QVBoxLayout;
    leftVerLayout->addWidget(useTip, 1, Qt::AlignHCenter|Qt::AlignBottom);
    leftVerLayout->addWidget(treeWgt, 1, Qt::AlignHCenter|Qt::AlignVCenter);
    //leftVerLayout->addStretch(1);
    QVBoxLayout* boxLayout = new QVBoxLayout;
    boxLayout->setContentsMargins(7,7,7,7);
    boxLayout->addWidget(noGrid, 1, Qt::AlignVCenter|Qt::AlignLeft);
    boxLayout->addWidget(grid5_5, 1, Qt::AlignVCenter|Qt::AlignLeft);
    boxLayout->addWidget(grid11_11, 1, Qt::AlignVCenter|Qt::AlignLeft);
    grid_box->setLayout(boxLayout);
    leftVerLayout->addWidget(grid_box, 1, Qt::AlignHCenter|Qt::AlignVCenter);

    hlayout->addLayout(leftVerLayout);

    imgView->setFrameStyle(QFrame::Box);
    imgView->setScaledContents(true);
    hlayout->addWidget(imgView, 1);

    hlayout->addWidget(blcDataEdit, 1);
    blcDataEdit->setPlainText("show calib xml context");

    setLayout(hlayout);

    qint32 curSeletISO = blc_fn_map.firstKey();
    QString curRawFn = blc_fn_map[curSeletISO].at(0);

    grid5_5->setChecked(true);//default 5x5
    showRawFile(curRawFn);
    setWindowTitle(tr("BLC tool"));

    connect(treeWgt, &QTreeWidget::itemDoubleClicked, this, &BLCDialog::onTreeItemDoubleClicked);
    connect(noGrid, &QRadioButton::toggled, this, &BLCDialog::onNoGridToggled);
    connect(grid5_5, &QRadioButton::toggled, this, &BLCDialog::onGrid5_5_Toggled);
    connect(grid11_11, &QRadioButton::toggled, this, &BLCDialog::onGrid11_11_Toggled);
}

BLCDialog::~BLCDialog()
{
    if(showImgBuf!=NULL)
        delete[] showImgBuf;
}

void BLCDialog::onTreeItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if(item->parent()==NULL)
        return;
    else{
        QString clicked_real_file;
        QString rootText  = item->parent()->text(column);
        int iso = rootText.right(rootText.size()-3).toInt();
        QStringList specificISO_fn = blc_fn_map[iso];
        for(QStringList::iterator it = specificISO_fn.begin(); it!=specificISO_fn.end(); it++){
            QFileInfo finfo(*it);
            QString fn = finfo.fileName();
            if(item->text(column)==fn){
                clicked_real_file = *it;
                break;
            }
        }
        if(clicked_real_file.isEmpty())
            return;
        showRawFile(clicked_real_file);
    }
}

void BLCDialog::showRawFile(const QString &rawfileName)
{
    QFile rawfile(rawfileName);

    quint8 rawByte = (bitDepth>8)?2:1;
    quint32 bufSize = rawByte*rawSize.width()*rawSize.height();
    quint8* buf = new quint8[bufSize];

    rawfile.open(QIODevice::ReadOnly);
    rawfile.read((char*)buf, bufSize);
    int rawWidth = rawSize.width(); int rawHeight = rawSize.height();
    if(showImgBuf==NULL)
        showImgBuf = new quint8[(rawWidth/4)*(rawHeight/4)];
    if(rawByte==2){
        for(int row=0; row<rawHeight/4; row++)
            for(int col=0; col<rawWidth/4; col++)
                showImgBuf[row*rawWidth/4+col] = ((quint16*)buf)[(row*4)*rawWidth+col*4]>>(bitDepth-8);
    }
    else{
        for(int row=0; row<rawHeight/4; row++)
            for(int col=0; col<rawWidth/4; col++)
                showImgBuf[row*rawWidth/4+col] = ((quint16*)buf)[(row*4)*rawWidth+col*4];
    }
    delete[] buf;
    showIm = QPixmap::fromImage(QImage(showImgBuf, rawSize.width()/4, rawSize.height()/4, QImage::Format_Grayscale8));


    if(noGrid->isChecked()){
        imgView->setGridMode(QString("0x0"));
    }
    else if(grid5_5->isChecked()){
        imgView->setGridMode(QString("5x5"));
    }
    else if(grid11_11->isChecked()){
        imgView->setGridMode(QString("11x11"));
    }
    imgView->setShowImage(&showIm);
}

void BLCDialog::onNoGridToggled(bool statu)
{
    if(statu==true){
        imgView->setGridMode(QString("0x0"));
        imgView->repaint();
    }
}
void BLCDialog::onGrid5_5_Toggled(bool statu)
{
    if(statu==true){
        imgView->setGridMode(QString("5x5"));
        imgView->repaint();
    }
}
void BLCDialog::onGrid11_11_Toggled(bool statu)
{
    if(statu==true){
        imgView->setGridMode(QString("11x11"));
        imgView->repaint();
    }
}
//-------class gridImgLabel------------
gridImgLabel::gridImgLabel(QWidget* parent, QString gridFlag, QPixmap* const img):
    QLabel(parent),
    gridMode(gridFlag),
    image(img)
{

}

void gridImgLabel::paintEvent(QPaintEvent *e)
{
    QPixmap target = image->scaled(e->rect().size());
    QPainter p;
    p.begin(&target);
    p.save();
    QBrush brush(QColor::fromRgb(0,200,0));
    QPen pen(brush, 1);
    pen.setStyle(Qt::DashLine);
    p.setPen(pen);
    p.setRenderHint(QPainter::Antialiasing);
    if(getGridMode()=="5x5"){
        QRect rects[25];
        for(int row=0; row<5; row++)
            for(int col=0; col<5; col++){
                int left = (col*target.width()/4 - 4)<0?0:(col*target.width()/4 - 4);
                int top = (row*target.height()/4 - 4)<0?0:(row*target.height()/4 - 4);
                int right = (col*target.width()/4 + 4)>target.width()-1?target.width()-1:(col*target.width()/4 + 4);
                int bottom = (row*target.height()/4 + 4)>target.height()-1?target.height()-1:(row*target.height()/4 + 4);
                if(left==0)
                    right = left+8;
                if(right==target.width()-1)
                    left = right-8;
                if(top==0)
                    bottom = top+8;
                if(bottom==target.height()-1)
                    top = bottom-8;
                rects[row*5+col].setCoords(left, top, right, bottom);
            }
        p.drawRects(rects, 25);
        p.drawLine(1, 1, target.width()-1, 1);
        p.drawLine(1, target.height()/4, target.width()-1, target.height()/4);
        p.drawLine(1, target.height()/2, target.width()-1, target.height()/2);
        p.drawLine(1, target.height()*3/4, target.width()-1, target.height()*3/4);
        p.drawLine(1, target.height()-1, target.width()-1, target.height()-1);
        p.drawLine(1, 1, 1, target.height()-1);
        p.drawLine(target.width()/4, 1, target.width()/4, target.height()-1);
        p.drawLine(target.width()/2, 1, target.width()/2, target.height()-1);
        p.drawLine(target.width()*3/4, 1, target.width()*3/4, target.height()-1);
        p.drawLine(target.width()-1, 1, target.width()-1, target.height()-1);
    }
    else if(getGridMode()=="11x11"){
        QRect rects_2[11*11];
        for(int row=0; row<11; row++)
            for(int col=0; col<11; col++){
                int left = (col*target.width()/10 - 3)<0?0:(col*target.width()/10 - 3);
                int top = (row*target.height()/10 - 3)<0?0:(row*target.height()/10 - 3);
                int right = (col*target.width()/10 + 3)>target.width()-1?target.width()-1:(col*target.width()/10 + 3);
                int bottom = (row*target.height()/10 + 3)>target.height()-1?target.height()-1:(row*target.height()/10 + 3);
                if(left==0)
                    right = left+6;
                if(right==target.width()-1)
                    left = right-6;
                if(top==0)
                    bottom = top+6;
                if(bottom==target.height()-1)
                    top = bottom-6;
                rects_2[row*11+col].setCoords(left, top, right, bottom);
            }
        p.drawRects(rects_2, 11*11);
        p.drawLine(1, 1, target.width()-1, 1);
        for(int row=1; row<10; row++){
            p.drawLine(1, target.height()*row/10, target.width()-1, target.height()*row/10);
        }
        p.drawLine(1, target.height()-1, target.width()-1, target.height()-1);
        p.drawLine(1, 1, 1, target.height()-1);
        for(int col=1; col<10; col++){
            p.drawLine(target.width()*col/10, 1, target.width()*col/10, target.height()-1);
        }
        p.drawLine(target.width()-1, 1, target.width()-1, target.height()-1);
    }
    p.restore();
    p.end();

    p.begin(this);
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.drawPixmap(0,0, target);
    p.restore();
    p.end();
    QLabel::paintEvent(e);
}
