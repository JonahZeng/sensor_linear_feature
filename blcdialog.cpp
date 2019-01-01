#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")  //fuck MSVC complior, use UTF-8, not gb2312/gbk
#endif

#include "inc/blcdialog.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QPainter>
#include <QPaintEvent>
#include <QTextStream>
#include <QMessageBox>
//#include "numpy/arrayobject.h"

BLCDialog::BLCDialog(QWidget *parent, const QMap<qint32, QStringList>& blc_map, rawinfoDialog::bayerMode bm, QSize rawsz, quint16 bd) :
    QDialog(parent),
    useTip(new QLabel(tr("<font size='-1'><p>双击下面的raw文件名切换</p>"
                         "<p>只兼容输出</p>"
                         "<p><b>V300</b>的blc xml</p>"
                         "<p>blc计算方法:</p>"
                         "<p>1.将选中的raw文件按照ISO分组</p>"
                         "<p>2.每一组ISO的raw叠加求平均</p>"
                         "<p>3.11x11范围平均模糊</p>"
                         "<p>4.长宽都缩小至1/16</p>"
                         "<p>5.水平、竖直方向拟合(2阶)</p>"
                         "<p>6.按照11x11格点位置取拟合后的值</p></font>"), this)),
    treeWgt(new QTreeWidget(this)),
    imgView(new gridImgLabel(this, "0x0", NULL)),
    grid_box(new QGroupBox(tr("grid option"), this)),
    noGrid(new QRadioButton(tr("no grid"), this)),
    grid5_5(new QRadioButton(tr("grid 5x5"), this)),
    grid11_11(new QRadioButton(tr("grid 11x11"), this)),
    blcDataEdit(new QPlainTextEdit(this)),
    saveXml(new QPushButton(tr("保存xml"), this)),
    xmlDoc(new QDomDocument),
    hlayout(new QHBoxLayout(this)),
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
    //QString header = tr("ISO/rawfile");
    treeWgt->setHeaderLabel(tr("ISO/rawfile"));
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
                child->setSelected(true);//默认选中第一个raw
            else
                child->setSelected(false);
        }
        treeWgt->insertTopLevelItem(root_id, item);
    }
    treeWgt->expandAll();
    treeWgt->setFocusPolicy(Qt::ClickFocus);

    QVBoxLayout* leftVerLayout = new QVBoxLayout;
    leftVerLayout->addWidget(useTip, 1, Qt::AlignHCenter|Qt::AlignVCenter);
    leftVerLayout->addWidget(treeWgt, 1);
    //leftVerLayout->addStretch(1);
    QVBoxLayout* boxLayout = new QVBoxLayout;
    boxLayout->setContentsMargins(7,7,7,7);
    boxLayout->addWidget(noGrid, 1, Qt::AlignVCenter|Qt::AlignLeft);
    boxLayout->addWidget(grid5_5, 1, Qt::AlignVCenter|Qt::AlignLeft);
    boxLayout->addWidget(grid11_11, 1, Qt::AlignVCenter|Qt::AlignLeft);
    grid_box->setLayout(boxLayout);
    leftVerLayout->addWidget(grid_box, 0, Qt::AlignHCenter|Qt::AlignVCenter);

    hlayout->addLayout(leftVerLayout);

    imgView->setFrameStyle(QFrame::Box);
    imgView->setScaledContents(true);
    hlayout->addWidget(imgView, 1);

    QVBoxLayout* rightVerLayout = new QVBoxLayout;
    rightVerLayout->addWidget(blcDataEdit, 1);
    rightVerLayout->addWidget(saveXml, 0, Qt::AlignLeft|Qt::AlignVCenter);
    hlayout->addLayout(rightVerLayout, 1);

    QDomProcessingInstruction insturction = xmlDoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'");
    xmlDoc->appendChild(insturction);
    docRoot = xmlDoc->createElement("module_calibration");
    xmlDoc->appendChild(docRoot);

    setLayout(hlayout);

    qint32 curSeletISO = blc_fn_map.firstKey();
    QString curRawFn = blc_fn_map[curSeletISO].at(0);

    grid11_11->setChecked(true);//default 11x11
    grid5_5->setEnabled(false); //close 5x5
    showRawFile(curRawFn);
    setWindowTitle(tr("BLC tool"));

    connect(treeWgt, &QTreeWidget::itemDoubleClicked, this, &BLCDialog::onTreeItemDoubleClicked);
    connect(noGrid, &QRadioButton::toggled, this, &BLCDialog::onNoGridToggled);
    connect(grid5_5, &QRadioButton::toggled, this, &BLCDialog::onGrid5_5_Toggled);
    connect(grid11_11, &QRadioButton::toggled, this, &BLCDialog::onGrid11_11_Toggled);


    int totalTasks = 0;
    for(QMap<qint32, QStringList>::iterator it=blc_fn_map.begin(); it!=blc_fn_map.end(); it++){
        QStringList files = it.value();
        totalTasks += files.size();
    }
    clacBLCprogress = new CalcBlcProgressDlg(this, totalTasks);//创建进度对话框

    calcBLCthread = new calcBlcThread(this, blc_fn_map, bayerMode, rawSize, bitDepth, xmlDoc, docRoot);//创建新线程处理blc raw,并且通知对话框进度更新，完成后自动关闭
    calcBLCthread->start();
    connect(calcBLCthread, &calcBlcThread::currentTaskId, clacBLCprogress, &CalcBlcProgressDlg::handleCurrentTaskId);
    connect(calcBLCthread, &calcBlcThread::pyInitFail, clacBLCprogress, &CalcBlcProgressDlg::reject);//如果Py初始化失败，则进度对话框关闭并导致显示错误信息

    if(clacBLCprogress->exec()==CalcBlcProgressDlg::Accepted){
        QString showXmlDoc;
        QTextStream xmlOutStream(&showXmlDoc, QIODevice::WriteOnly);
        xmlDoc->save(xmlOutStream, 4);
        blcDataEdit->setPlainText(showXmlDoc);
    }
    else{
        blcDataEdit->setPlainText(tr("计算出错，请检查环境配置、raw文件和输入信息是否匹配"));
    }
}

BLCDialog::~BLCDialog()
{
    if(showImgBuf!=NULL)
        delete[] showImgBuf;
    if(!(xmlDoc->isNull())){
        delete xmlDoc;
    }
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

void BLCDialog::onDestoryDlg()
{
    calcBLCthread->quit();
    calcBLCthread->wait();
}

//qint32 BLCDialog::onProgressDlgRun()
//{
    //-----一次性计算过程，如果多次，import 模块内容应放在构造函数里面，并保持PyObject*指针，并在析构函数中Py_DECREF
    //import_array();
    //PyObject* pNdimageModule = PyImport_Import(PyUnicode_FromString("scipy.ndimage"));
    //PyObject* pUniformFunc = PyObject_GetAttrString(pNdimageModule, "uniform_filter");
    //PyObject* pZoomFunc = PyObject_GetAttrString(pNdimageModule, "zoom");
    //PyObject* pSignalModule = PyImport_Import(PyUnicode_FromString("scipy.signal"));
    //PyObject* pFuncSavgol = PyObject_GetAttrString(pSignalModule, "savgol_filter");
    //if(pNdimageModule==NULL || pUniformFunc==NULL || pZoomFunc==NULL || pSignalModule==NULL || pFuncSavgol==NULL){
    //    Py_XDECREF(pSignalModule);
    //    Py_XDECREF(pZoomFunc);
    //    Py_XDECREF(pUniformFunc);
    //    Py_XDECREF(pNdimageModule);
    //    Py_XDECREF(pFuncSavgol);
    //    Py_Finalize();
    //    return -1;
    //}
    //----------------------end----------------------------

    //int numTasks = 0;
    //for(QMap<qint32, QStringList>::iterator it=blc_fn_map.begin(); it!=blc_fn_map.end(); it++){
    //    QStringList files = it.value();
    //    numTasks += files.size();
    //}
    //
    //qreal* bayer_r_buf = new qreal[rawSize.width()*rawSize.height()/4];
    //qreal* bayer_gr_buf = new qreal[rawSize.width()*rawSize.height()/4];
    //qreal* bayer_gb_buf = new qreal[rawSize.width()*rawSize.height()/4];
    //qreal* bayer_b_buf = new qreal[rawSize.width()*rawSize.height()/4];
    //memset((void*)bayer_r_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
    //memset((void*)bayer_gr_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
    //memset((void*)bayer_gb_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
    //memset((void*)bayer_b_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
    //
    //quint8* raw_buf;
    //if(bitDepth>8)
    //    raw_buf = new quint8[rawSize.width()*rawSize.height()*2];
    //else
    //    raw_buf = new quint8[rawSize.width()*rawSize.height()];
    //
    //npy_intp shape[2] = {rawSize.height()/2, rawSize.width()/2};

//    int i = 0, dataIdx=0;
//    QVector<quint16> stored_ae_gain;
//    for(QMap<qint32, QStringList>::iterator it=blc_fn_map.begin(); it!=blc_fn_map.end(); it++){
//        quint16 r_blc_be, gr_blc_be, gb_blc_be, b_blc_be;
//        QVector<quint16> r_grid(121), gr_grid(121), gb_grid(121), b_grid(121);
//        quint16 ae_gain = it.key()/50;
//        for(QStringList::Iterator str=it.value().begin(); str!=it.value().end(); str++){
//            QFile raw_f(*str);
//            raw_f.open(QFile::ReadOnly);
//            raw_f.read((char*)raw_buf, bitDepth>8?(2*rawSize.width()*rawSize.height()):rawSize.width()*rawSize.height());
//            raw_f.close();
//            addRaw2FourChannel(raw_buf, bayer_r_buf, bayer_gr_buf, bayer_gb_buf, bayer_b_buf);
//            memset((void*)raw_buf, 0, bitDepth>8?(2*rawSize.width()*rawSize.height()):rawSize.width()*rawSize.height());
//            i++;
//        }
//        avgBayerChannel(bayer_r_buf, it.value().size());
//        avgBayerChannel(bayer_gr_buf, it.value().size());
//        avgBayerChannel(bayer_gb_buf, it.value().size());
//        avgBayerChannel(bayer_b_buf, it.value().size());
//        PyArrayObject* r = (PyArrayObject*)PyArray_SimpleNewFromData(2, shape, NPY_FLOAT64, (void*)bayer_r_buf);
//        PyArrayObject* gr = (PyArrayObject*)PyArray_SimpleNewFromData(2, shape, NPY_FLOAT64, (void*)bayer_gr_buf);
//        PyArrayObject* gb = (PyArrayObject*)PyArray_SimpleNewFromData(2, shape, NPY_FLOAT64, (void*)bayer_gb_buf);
//        PyArrayObject* b = (PyArrayObject*)PyArray_SimpleNewFromData(2, shape, NPY_FLOAT64, (void*)bayer_b_buf);

//        PyArrayObject* p_blur_R = (PyArrayObject*)PyObject_CallFunction(pUniformFunc, "(Oi)", (PyObject*)r, 11);
//        PyArrayObject* p_blur_Gr = (PyArrayObject*)PyObject_CallFunction(pUniformFunc, "(Oi)", (PyObject*)gr, 11);
//        PyArrayObject* p_blur_Gb = (PyArrayObject*)PyObject_CallFunction(pUniformFunc, "(Oi)", (PyObject*)gb, 11);
//        PyArrayObject* p_blur_B = (PyArrayObject*)PyObject_CallFunction(pUniformFunc, "(Oi)", (PyObject*)b, 11);
//        //因为是手动创建buf然后传给ndarray，所以r 没有own_data_flag，DECREF(r)不会释放data内存
//        Py_DECREF(r);
//        Py_DECREF(gr);
//        Py_DECREF(gb);
//        Py_DECREF(b);
//        PyArrayObject* zoom_out_R = (PyArrayObject*)PyObject_CallFunction(pZoomFunc, "(OfOis)", (PyObject*)p_blur_R, 0.0625, Py_None, 3, "nearest");
//        PyArrayObject* zoom_out_Gr = (PyArrayObject*)PyObject_CallFunction(pZoomFunc, "(OfOis)", (PyObject*)p_blur_Gr, 0.0625, Py_None, 3, "nearest");
//        PyArrayObject* zoom_out_Gb = (PyArrayObject*)PyObject_CallFunction(pZoomFunc, "(OfOis)", (PyObject*)p_blur_Gb, 0.0625, Py_None, 3, "nearest");
//        PyArrayObject* zoom_out_B = (PyArrayObject*)PyObject_CallFunction(pZoomFunc, "(OfOis)", (PyObject*)p_blur_B, 0.0625, Py_None, 3, "nearest");

//        Py_DECREF(p_blur_R);//own_data_flag=1, 不需要手动释放data
//        Py_DECREF(p_blur_Gr);
//        Py_DECREF(p_blur_Gb);
//        Py_DECREF(p_blur_B);

//        npy_int32 winLen_order1 = zoom_out_R->dimensions[1]/8;
//        winLen_order1 = (((npy_uint32)winLen_order1>>1)<<1)+1;
//        npy_int32 winLen_order0 = zoom_out_R->dimensions[0]/8;
//        winLen_order0 = (((npy_uint32)winLen_order0>>1)<<1)+1;

//        PyArrayObject* savgol_r_order1 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)zoom_out_R, winLen_order1, 2, 0, 1.0, 1);
//        Py_DECREF(zoom_out_R);
//        PyArrayObject* savgol_r_order0 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)savgol_r_order1, winLen_order0, 2, 0, 1.0, 0);

//        PyArrayObject* savgol_gr_order1 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)zoom_out_Gr, winLen_order1, 2, 0, 1.0, 1);
//        Py_DECREF(zoom_out_Gr);
//        PyArrayObject* savgol_gr_order0 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)savgol_gr_order1, winLen_order0, 2, 0, 1.0, 0);

//        PyArrayObject* savgol_gb_order1 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)zoom_out_Gb, winLen_order1, 2, 0, 1.0, 1);
//        Py_DECREF(zoom_out_Gb);
//        PyArrayObject* savgol_gb_order0 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)savgol_gb_order1, winLen_order0, 2, 0, 1.0, 0);

//        PyArrayObject* savgol_b_order1 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)zoom_out_B, winLen_order1, 2, 0, 1.0, 1);
//        Py_DECREF(zoom_out_B);
//        PyArrayObject* savgol_b_order0 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)savgol_b_order1, winLen_order0, 2, 0, 1.0, 0);

//        Q_ASSERT(savgol_r_order0->nd==2 && savgol_gr_order0->nd==2 && savgol_gb_order0->nd==2 && savgol_b_order0->nd==2);
//        r_blc_be  = avgBlcValueBE((qreal*)(savgol_r_order0->data),  savgol_r_order0->dimensions[0]*savgol_r_order0->dimensions[1]);
//        gr_blc_be = avgBlcValueBE((qreal*)(savgol_gr_order0->data), savgol_gr_order0->dimensions[0]*savgol_gr_order0->dimensions[1]);
//        gb_blc_be = avgBlcValueBE((qreal*)(savgol_gb_order0->data), savgol_gb_order0->dimensions[0]*savgol_gb_order0->dimensions[1]);
//        b_blc_be  = avgBlcValueBE((qreal*)(savgol_b_order0->data),  savgol_b_order0->dimensions[0]*savgol_b_order0->dimensions[1]);
//        calGridValue((qreal*)(savgol_r_order0->data), savgol_r_order0->dimensions[0], savgol_r_order0->dimensions[1], r_grid);
//        calGridValue((qreal*)(savgol_gr_order0->data), savgol_gr_order0->dimensions[0], savgol_gr_order0->dimensions[1], gr_grid);
//        calGridValue((qreal*)(savgol_gb_order0->data), savgol_gb_order0->dimensions[0], savgol_gb_order0->dimensions[1], gb_grid);
//        calGridValue((qreal*)(savgol_b_order0->data), savgol_b_order0->dimensions[0], savgol_b_order0->dimensions[1], b_grid);

//        if(!stored_ae_gain.contains(ae_gain)){
//            createBlcDateNode(dataIdx, ae_gain, r_blc_be, gr_blc_be, gb_blc_be, b_blc_be, r_grid, gr_grid, gb_grid, b_grid);
//            dataIdx++;
//        }

//        Py_DECREF(savgol_r_order1);
//        Py_DECREF(savgol_r_order0);
//        Py_DECREF(savgol_gr_order1);
//        Py_DECREF(savgol_gr_order0);
//        Py_DECREF(savgol_gb_order1);
//        Py_DECREF(savgol_gb_order0);
//        Py_DECREF(savgol_b_order1);
//        Py_DECREF(savgol_b_order0);

//        memset((void*)bayer_r_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
//        memset((void*)bayer_gr_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
//        memset((void*)bayer_gb_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
//        memset((void*)bayer_b_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
//    }
//    Py_XDECREF(pSignalModule);
//    Py_XDECREF(pZoomFunc);
//    Py_XDECREF(pUniformFunc);
//    Py_XDECREF(pNdimageModule);
//    Py_XDECREF(pFuncSavgol);
//    delete[] raw_buf;
//    delete[] bayer_b_buf;
//    delete[] bayer_gb_buf;
//    delete[] bayer_gr_buf;
//    delete[] bayer_r_buf;
//    return 0;
//}
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
