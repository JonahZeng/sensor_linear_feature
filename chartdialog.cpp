#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")  //fuck MSVC complior, use UTF-8, not gb2312/gbk
#endif

#include "inc/chartdialog.h"
#include <QValueAxis>
#include <QMessageBox>
#include <QLabel>
#include "numpy/arrayobject.h"
#include <algorithm>
#include <QFileDialog>
#include <QDomDocument>
#include <QDomNodeList>
#include "inc/rightshiftdialog.h"
#include <QTextStream>
#define MIN(a,b) (a>b?b:a)
#define MAX(a,b) (a>b?a:b)

/*
-----------------------------------
| = = = = = = = = = = = = = = =[tips]
| = = = = = = = = = = = = = = =[r st end]
| = = = = = chart view= = = = =[gr st end]
| = = = = = = = = = = = = = = = [---]
| = = = = = = = = = = = = = = =[---]
-----------------------------------
*/
ChartDialog::ChartDialog(QWidget *parent,
                         const QVector<QPointF> &lineR,
                         const QVector<QPointF> &lineGr,
                         const QVector<QPointF> &lineGb,
                         const QVector<QPointF> &lineB,
                         int nlc_size,
                         int ISO):
        QDialog(parent),
        prevPath(),
        iso(ISO),
        polyfit_success(false),
        pModule(NULL),
        origin_R(lineR),
        origin_Gr(lineGr),
        origin_Gb(lineGb),
        origin_B(lineB)
{
    qreal minx = minQPointsX(lineR);
    qreal maxx = maxQPointsX(lineR);

    qreal miny_R = minQPointsY(lineR);
    qreal miny_Gr = minQPointsY(lineGr);
    qreal miny_Gb = minQPointsY(lineGb);
    qreal miny_B = minQPointsY(lineB);

    qreal maxy_R = maxQPointsY(lineR);
    qreal maxy_Gr = maxQPointsY(lineGr);
    qreal maxy_Gb = maxQPointsY(lineGb);
    qreal maxy_B = maxQPointsY(lineB);

    qreal miny = MIN(MIN(MIN(miny_R, miny_Gr), miny_Gb), miny_B);
    qreal maxy = MAX(MAX(MAX(maxy_R, maxy_Gr), maxy_Gb), maxy_B);

    R = new QLineSeries(); R_pf = new QLineSeries();
    Gr = new QLineSeries(); Gr_pf = new QLineSeries();
    Gb = new QLineSeries(); Gb_pf = new QLineSeries();
    B = new QLineSeries(); B_pf = new QLineSeries();
    R_pf_2pt = new QScatterSeries(); Gr_pf_2pt = new QScatterSeries();
    Gb_pf_2pt = new QScatterSeries(); B_pf_2pt = new QScatterSeries();

    R_pf_2pt->append(lineR[nlc_size/2]); R_pf_2pt->append(lineR[nlc_size-1]);
    R_pf_2pt->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    R_pf_2pt->setColor(QColor::fromRgb(100,0,0));
    R_pf_2pt->setMarkerSize(8);
    R_pf_2pt->setVisible();
    R_pf_2pt->setName("R_pf s-e");
    Gr_pf_2pt->append(lineGr[nlc_size/2]); Gr_pf_2pt->append(lineGr[nlc_size-1]);
    Gr_pf_2pt->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    Gr_pf_2pt->setColor(QColor::fromRgb(0,100,0));
    Gr_pf_2pt->setMarkerSize(8);
    Gr_pf_2pt->setVisible();
    Gr_pf_2pt->setName("Gr_pf s-e");
    Gb_pf_2pt->append(lineGb[nlc_size/2]); Gb_pf_2pt->append(lineGb[nlc_size-1]);
    Gb_pf_2pt->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    Gb_pf_2pt->setColor(QColor::fromRgb(0,100,0));
    Gb_pf_2pt->setMarkerSize(8);
    Gb_pf_2pt->setVisible();
    Gb_pf_2pt->setName("Gb_pf s-e");
    B_pf_2pt->append(lineB[nlc_size/2]); B_pf_2pt->append(lineB[nlc_size-1]);
    B_pf_2pt->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    B_pf_2pt->setColor(QColor::fromRgb(0,0,100));
    B_pf_2pt->setMarkerSize(8);
    B_pf_2pt->setVisible();
    B_pf_2pt->setName(tr("B_pf s-e"));

    R->append(QList<QPointF>::fromVector(lineR));
    R->setName("R");
    R->setPointsVisible();
    Gr->append(QList<QPointF>::fromVector(lineGr));
    Gr->setName("Gr");
    Gr->setPointsVisible();
    Gb->append(QList<QPointF>::fromVector(lineGb));
    Gb->setName("Gb");
    Gb->setPointsVisible();
    B->append(QList<QPointF>::fromVector(lineB));
    B->setName("B");
    B->setPointsVisible();

    QBrush brush(QColor::fromRgb(255, 0, 0));
    QPen pen(brush, 2, Qt::SolidLine);
    R->setPen(pen);

    brush.setColor(QColor::fromRgb(96, 200, 0));
    pen.setBrush(brush);
    Gr->setPen(pen);

    brush.setColor(QColor::fromRgb(0, 200, 96));
    pen.setBrush(brush);
    Gb->setPen(pen);

    brush.setColor(QColor::fromRgb(0, 0, 255));
    pen.setBrush(brush);
    B->setPen(pen);

    R_pf->append(QList<QPointF>::fromVector(lineR));
    R_pf->setName("R_拟合");
    R_pf->setVisible(false);
    brush.setColor(QColor::fromRgb(255, 0 , 0));
    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);
    pen.setBrush(brush);
    R_pf->setPen(pen);

    Gr_pf->append(QList<QPointF>::fromVector(lineGr));
    Gr_pf->setName("Gr_拟合");
    Gr_pf->setVisible(false);
    pen.setColor(QColor::fromRgb(96, 200, 0));
    Gr_pf->setPen(pen);

    Gb_pf->append(QList<QPointF>::fromVector(lineGb));
    Gb_pf->setName("Gb_拟合");
    Gb_pf->setVisible(false);
    pen.setColor(QColor::fromRgb(0, 200, 96));
    Gb_pf->setPen(pen);

    B_pf->append(QList<QPointF>::fromVector(lineB));
    B_pf->setName("B_拟合");
    B_pf->setVisible(false);
    pen.setColor(QColor::fromRgb(0, 0, 255));
    B_pf->setPen(pen);

    chart = new Chart();
    chart->setAnimationOptions(QChart::SeriesAnimations);
    //chart->setTitleFont(QFont("Microsoft YaHei UI", 10));
    //chart->setTheme(QChart::ChartThemeBlueCerulean);
    chart->addSeries(R); chart->addSeries(Gr); chart->addSeries(Gb); chart->addSeries(B);
    chart->addSeries(R_pf); chart->addSeries(Gr_pf); chart->addSeries(Gb_pf); chart->addSeries(B_pf);
    chart->addSeries(R_pf_2pt); chart->addSeries(Gr_pf_2pt); chart->addSeries(Gb_pf_2pt); chart->addSeries(B_pf_2pt);

    chart->setTitle("<font size='+1'><p align='center'>sensor线性度测试&拟合估计</p></font>");
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText(tr("曝光时间(ms)"));
    axisX->setTickCount(8);
    axisX->setMinorTickCount(3);
    axisX->setRange(minx-10, maxx+10);

    chart->setAxisX(axisX);
    R->attachAxis(axisX);    Gr->attachAxis(axisX);    Gb->attachAxis(axisX);    B->attachAxis(axisX);
    R_pf->attachAxis(axisX); Gr_pf->attachAxis(axisX); Gb_pf->attachAxis(axisX); B_pf->attachAxis(axisX);
    R_pf_2pt->attachAxis(axisX); Gr_pf_2pt->attachAxis(axisX); Gb_pf_2pt->attachAxis(axisX); B_pf_2pt->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleFont(QFont("Microsoft YaHei UI", 10));
    axisY->setTitleText(tr("选区平均值(14bit)"));
    axisY->setTickCount(8);
    axisY->setMinorTickCount(3);
    axisY->setRange(miny-10, maxy+10);

    chart->setAxisY(axisY);
    R->attachAxis(axisY); Gr->attachAxis(axisY); Gb->attachAxis(axisY); B->attachAxis(axisY);
    R_pf->attachAxis(axisY); Gr_pf->attachAxis(axisY); Gb_pf->attachAxis(axisY); B_pf->attachAxis(axisY);
    R_pf_2pt->attachAxis(axisY); Gr_pf_2pt->attachAxis(axisY); Gb_pf_2pt->attachAxis(axisY); B_pf_2pt->attachAxis(axisY);

    createUI(nlc_size);
    connect(polyfitBtn, &QPushButton::clicked, this, &ChartDialog::polyfit1);
    connect(saveBtn, &QPushButton::clicked, this, &ChartDialog::save2XML);

    connect(polyfitStartR, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartDialog::pfStartChangeR);
    connect(polyfitEndR, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartDialog::pfEndChangeR);
    connect(polyfitStartGr, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartDialog::pfStartChangeGr);
    connect(polyfitEndGr, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartDialog::pfEndChangeGr);
    connect(polyfitStartGb, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartDialog::pfStartChangeGb);
    connect(polyfitEndGb, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartDialog::pfEndChangeGb);
    connect(polyfitStartB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartDialog::pfStartChangeB);
    connect(polyfitEndB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ChartDialog::pfEndChangeB);

    connect(R_visual, &QCheckBox::toggled, this, &ChartDialog::showR);
    connect(Gr_visual, &QCheckBox::toggled, this, &ChartDialog::showGr);
    connect(Gb_visual, &QCheckBox::toggled, this, &ChartDialog::showGb);
    connect(B_visual, &QCheckBox::toggled, this, &ChartDialog::showB);

    connect(R, &QLineSeries::pressed, chart, &Chart::reciveR_pressed);//线条上的点被选中，通知chart是哪条线的点被选中，mousemove时间由chartview处理
    connect(Gr, &QLineSeries::pressed, chart, &Chart::reciveGr_pressed);
    connect(Gb, &QLineSeries::pressed, chart, &Chart::reciveGb_pressed);
    connect(B, &QLineSeries::pressed, chart, &Chart::reciveB_pressed);

    connect(R, &QLineSeries::released, chart, &Chart::reciveR_released);
    connect(Gr, &QLineSeries::released, chart, &Chart::reciveGr_released);
    connect(Gb, &QLineSeries::released, chart, &Chart::reciveGb_released);
    connect(B, &QLineSeries::released, chart, &Chart::reciveB_released);

    connect(reset_R, &QPushButton::clicked, this, &ChartDialog::onResetR);
    connect(reset_Gr, &QPushButton::clicked, this, &ChartDialog::onResetGr);
    connect(reset_Gb, &QPushButton::clicked, this, &ChartDialog::onResetGb);
    connect(reset_B, &QPushButton::clicked, this, &ChartDialog::onResetB);
}

ChartDialog::~ChartDialog()
{
    if(R!=NULL)
        delete R;
    if(Gr!=NULL)
        delete Gr;
    if(Gb!=NULL)
        delete Gb;
    if(B!=NULL)
        delete B;
    if(R_pf!=NULL)
        delete R_pf;
    if(Gr_pf!=NULL)
        delete Gr_pf;
    if(Gb_pf!=NULL)
        delete Gb_pf;
    if(B_pf!=NULL)
        delete B_pf;
    if(R_pf_2pt!=NULL)
        delete R_pf_2pt;
    if(Gr_pf_2pt!=NULL)
        delete Gr_pf_2pt;
    if(Gb_pf_2pt!=NULL)
        delete Gb_pf_2pt;
    if(B_pf_2pt!=NULL)
        delete B_pf_2pt;
    if(chart!=NULL)
        delete chart;
    if(chartView!=NULL)
        delete chartView;
}


void ChartDialog::polyfit1()
{
    int idx1_R = polyfitStartR->value();
    int idx2_R = polyfitEndR->value();
    int idx1_Gr = polyfitStartGr->value();
    int idx2_Gr = polyfitEndGr->value();
    int idx1_Gb = polyfitStartGb->value();
    int idx2_Gb = polyfitEndGb->value();
    int idx1_B = polyfitStartB->value();
    int idx2_B = polyfitEndB->value();

    QVector<QPointF> pts_R = R->pointsVector();
    QVector<QPointF> pts_Gr = Gr->pointsVector();
    QVector<QPointF> pts_Gb = Gb->pointsVector();
    QVector<QPointF> pts_B = B->pointsVector();
    Q_ASSERT(pts_R.size()==pts_Gr.size());
    Q_ASSERT(pts_Gb.size()==pts_B.size());

    quint32 fitSize_R = idx2_R-idx1_R; //拟合直线
    quint32 fitSize_Gr = idx2_Gr-idx1_Gr;
    quint32 fitSize_Gb = idx2_Gb-idx1_Gb;
    quint32 fitSize_B = idx2_B-idx1_B;

    PyObject* px_R = PyList_New(fitSize_R);
    PyObject* py_R = PyList_New(fitSize_R);
    PyObject* px_Gr = PyList_New(fitSize_Gr);
    PyObject* py_Gr = PyList_New(fitSize_Gr);
    PyObject* px_Gb = PyList_New(fitSize_Gb);
    PyObject* py_Gb = PyList_New(fitSize_Gb);
    PyObject* px_B = PyList_New(fitSize_B);
    PyObject* py_B = PyList_New(fitSize_B);
    Q_ASSERT(pts_R[0].x()==0);
    for(quint32 n=0; n<fitSize_R; n++){
        PyList_SetItem(px_R, n, PyFloat_FromDouble(pts_R[idx1_R+n].x()));
        PyList_SetItem(py_R, n, PyFloat_FromDouble(pts_R[idx1_R+n].y()));
    }

    Q_ASSERT(pts_Gr[0].x()==0);
    for(quint32 n=0; n<fitSize_Gr; n++){
        PyList_SetItem(px_Gr, n, PyFloat_FromDouble(pts_Gr[idx1_Gr+n].x()));
        PyList_SetItem(py_Gr, n, PyFloat_FromDouble(pts_Gr[idx1_Gr+n].y()));
    }
    Q_ASSERT(pts_Gb[0].x()==0);
    for(quint32 n=0; n<fitSize_Gb; n++){
        PyList_SetItem(px_Gb, n, PyFloat_FromDouble(pts_Gb[idx1_Gb+n].x()));
        PyList_SetItem(py_Gb, n, PyFloat_FromDouble(pts_Gb[idx1_Gb+n].y()));
    }

    Q_ASSERT(pts_B[0].x()==0);
    for(quint32 n=0; n<fitSize_B; n++){
        PyList_SetItem(px_B, n, PyFloat_FromDouble(pts_B[idx1_B+n].x()));
        PyList_SetItem(py_B, n, PyFloat_FromDouble(pts_B[idx1_B+n].y()));
    }


    PyObject* pOrder1 = PyLong_FromLong(1);
    PyObject* pArgs_R = PyTuple_New(3);//(x,y,1)
    PyObject* pArgs_Gr = PyTuple_New(3);
    PyObject* pArgs_Gb = PyTuple_New(3);
    PyObject* pArgs_B = PyTuple_New(3);
    PyTuple_SetItem(pArgs_R, 0, px_R);
    PyTuple_SetItem(pArgs_R, 1, py_R);
    PyTuple_SetItem(pArgs_R, 2, pOrder1);
    PyTuple_SetItem(pArgs_Gr, 0, px_Gr);
    PyTuple_SetItem(pArgs_Gr, 1, py_Gr);
    PyTuple_SetItem(pArgs_Gr, 2, pOrder1);
    PyTuple_SetItem(pArgs_Gb, 0, px_Gb);
    PyTuple_SetItem(pArgs_Gb, 1, py_Gb);
    PyTuple_SetItem(pArgs_Gb, 2, pOrder1);
    PyTuple_SetItem(pArgs_B, 0, px_B);
    PyTuple_SetItem(pArgs_B, 1, py_B);
    PyTuple_SetItem(pArgs_B, 2, pOrder1);


    PyObject* pName = PyUnicode_FromString("numpy");
    if(pModule==NULL)
        pModule = PyImport_Import(pName);

    if(pModule!=NULL){
        PyObject* pFunc = PyObject_GetAttrString(pModule, "polyfit");
        if(pFunc && PyCallable_Check(pFunc)){
            PyArrayObject* pRet_R = (PyArrayObject*)PyObject_CallObject(pFunc, pArgs_R);
            Q_ASSERT(pRet_R->nd==1 && pRet_R->dimensions[0]==2);//1维 shape=(2,)
            PyArrayObject* pRet_Gr = (PyArrayObject*)PyObject_CallObject(pFunc, pArgs_Gr);
            Q_ASSERT(pRet_Gr->nd==1 && pRet_Gr->dimensions[0]==2);//1维 shape=(2,)
            PyArrayObject* pRet_Gb = (PyArrayObject*)PyObject_CallObject(pFunc, pArgs_Gb);
            Q_ASSERT(pRet_Gb->nd==1 && pRet_Gb->dimensions[0]==2);//1维 shape=(2,)
            PyArrayObject* pRet_B = (PyArrayObject*)PyObject_CallObject(pFunc, pArgs_B);
            Q_ASSERT(pRet_B->nd==1 && pRet_B->dimensions[0]==2);//1维 shape=(2,)


            qreal k1 = *((qreal*)PyArray_GETPTR1(pRet_R, 0));
            qreal b1 = *((qreal*)PyArray_GETPTR1(pRet_R, 1));
            qreal k2 = *((qreal*)PyArray_GETPTR1(pRet_Gr, 0));
            qreal b2 = *((qreal*)PyArray_GETPTR1(pRet_Gr, 1));
            qreal k3 = *((qreal*)PyArray_GETPTR1(pRet_Gb, 0));
            qreal b3 = *((qreal*)PyArray_GETPTR1(pRet_Gb, 1));
            qreal k4 = *((qreal*)PyArray_GETPTR1(pRet_B, 0));
            qreal b4 = *((qreal*)PyArray_GETPTR1(pRet_B, 1));

            cal4ChannelPolyfitCurve(k1, b1, k2, b2, k3, b3, k4, b4);


            if(R_visual->isChecked())
                R_pf->setVisible(true);
            if(Gr_visual->isChecked())
                Gr_pf->setVisible(true);
            if(Gb_visual->isChecked())
                Gb_pf->setVisible(true);
            if(B_visual->isChecked())
                B_pf->setVisible(true);
            Py_DECREF(pFunc);//decrease reference count,如果引用次数为0，则释放指针指向的python对象
            Py_DECREF(pRet_R); Py_DECREF(pRet_Gr); Py_DECREF(pRet_Gb); Py_DECREF(pRet_B);
        }
        Py_DECREF(pModule);
        saveBtn->setEnabled(true);
        polyfit_success = true;
    }
    else{
        saveBtn->setEnabled(false);
        polyfit_success = false;
    }
    Py_DECREF(pName);
    Py_DECREF(pArgs_R); Py_DECREF(pArgs_Gr);Py_DECREF(pArgs_Gb); Py_DECREF(pArgs_B);
    Py_DECREF(px_R); Py_DECREF(py_R); Py_DECREF(px_Gr); Py_DECREF(py_Gr);
    Py_DECREF(px_Gb); Py_DECREF(py_Gb);Py_DECREF(px_B); Py_DECREF(py_B);
    Py_DECREF(pOrder1);
}

void ChartDialog::save2XML()
{
    if(prevPath.isEmpty())
        prevPath = QDir::currentPath();
    QString xml_fn = QFileDialog::getOpenFileName(this, tr("save to.."), prevPath, tr("xml file(*.xml);;all file(*.*)"));
    if(xml_fn.isEmpty())
        return;
    QFile xml_f(xml_fn);
    if(!xml_f.open(QIODevice::ReadOnly|QIODevice::Text)){
        QMessageBox::critical(this, tr("error"), tr("文件不存在/打开失败"), QMessageBox::Ok);
        return;
    }
    QDomDocument doc;
    QByteArray strAll = xml_f.readAll();
    strAll.replace('\r', ' ');
    if(!doc.setContent(strAll, false)){
        QMessageBox::critical(this, tr("error"), tr("读取xml失败"), QMessageBox::Ok);
        return;
    }
    xml_f.close();
    //QDomNode instruction = doc.firstChild();
    QDomElement root = doc.documentElement();
    QDomNodeList data0_9 =  root.childNodes();
    int listLen = data0_9.length();
    //int ISO_vec[listLen] = {0};
    int findIdx = -1;
    for(int k=0; k<listLen; k++)
    {
        QDomNode data_x = data0_9.at(k);
        QString ae("ae_gain");
        QDomNode ae_gain = data_x.namedItem(ae);
        if(ae_gain.isElement())
        {
            bool cvtFalg = true;
            int tmpISO = ae_gain.toElement().text().toInt(&cvtFalg, 10) * 50;
            if(cvtFalg)
            {
                //ISO_vec[k] = tmpISO;
                if(tmpISO == iso)
                {
                    findIdx = k;
                    break;
                }
            }
        }
    }
    if(findIdx==-1){
        QMessageBox::critical(this, tr("error"), tr("没有在xml文件中找到ISO %1 对应的ae_gain").arg(iso), QMessageBox::Ok);
        return;
    }
    //qDebug()<<findIdx<<endl;
    QVector<QPointF> R_real = R->pointsVector();    QVector<QPointF> R_ideal = R_pf->pointsVector();
    QVector<QPointF> Gr_real = Gr->pointsVector();  QVector<QPointF> Gr_ideal = Gr_pf->pointsVector();
    QVector<QPointF> Gb_real = Gb->pointsVector();  QVector<QPointF> Gb_ideal = Gb_pf->pointsVector();
    QVector<QPointF> B_real = B->pointsVector();    QVector<QPointF> B_ideal = B_pf->pointsVector();
    QVector<qreal> R_real_y(R_real.size());         QVector<qreal> R_ideal_y(R_ideal.size());
    QVector<qreal> Gr_real_y(Gr_real.size());       QVector<qreal> Gr_ideal_y(Gr_ideal.size());
    QVector<qreal> Gb_real_y(Gb_real.size());       QVector<qreal> Gb_ideal_y(Gb_ideal.size());
    QVector<qreal> B_real_y(B_real.size());         QVector<qreal> B_ideal_y(B_ideal.size());
    Q_ASSERT(R_real.size()==R_ideal.size());
    Q_ASSERT(Gr_real.size()==Gr_ideal.size());
    Q_ASSERT(Gb_real.size()==Gb_ideal.size());
    Q_ASSERT(B_real.size()==B_ideal.size());
    for(int n=0; n<R_real.size(); n++){
        R_real_y[n] = R_real[n].y();
        R_ideal_y[n] = R_ideal[n].y();
    }
    for(int n=0; n<Gr_real.size(); n++){
        Gr_real_y[n] = Gr_real[n].y();
        Gr_ideal_y[n] = Gr_ideal[n].y();
    }
    for(int n=0; n<Gb_real.size(); n++){
        Gb_real_y[n] = Gb_real[n].y();
        Gb_ideal_y[n] = Gb_ideal[n].y();
    }
    for(int n=0; n<B_real.size(); n++){
        B_real_y[n] = B_real[n].y();
        B_ideal_y[n] = B_ideal[n].y();
    }
    qSort(R_real_y);
    qSort(Gr_real_y);
    qSort(Gb_real_y);
    qSort(B_real_y);

    int R_start_idx = polyfitStartR->value();
    int Gr_start_idx = polyfitStartGr->value();
    int Gb_start_idx = polyfitStartGb->value();
    int B_start_idx = polyfitStartB->value();
    qreal R_start = R_real_y[R_start_idx] - R_ideal_y[0];  //首先减掉理想blc，然后再右移
    qreal Gr_start = Gr_real_y[Gr_start_idx] - Gr_ideal_y[0];
    qreal Gb_start = Gb_real_y[Gb_start_idx] - Gb_ideal_y[0];
    qreal B_start = B_real_y[B_start_idx] - B_ideal_y[0];

    qreal max_start = MAX(MAX(MAX(R_start, Gr_start),Gb_start),B_start);
#define BIT_14_MAX 16383
#define BIT_14_MIN 0
    Q_ASSERT(max_start>BIT_14_MIN && max_start<BIT_14_MAX);
    qint16 max_start_d = qint16(max_start+0.5);
#define NLC_POINTS 32
    quint8 right_shift = 0;
    while((max_start_d>>right_shift)>(NLC_POINTS-1)){
        ++right_shift;
    }

    RightShiftDialog dlg(this, right_shift);//弹出对话框给用户选择右移几位，显示的初始值是最高精度下的位数
    if(RightShiftDialog::Accepted == dlg.exec()){
        right_shift = dlg.getRightShiftBit();

        QVector<qreal> calib_Rx(R_start_idx+1); QVector<qreal> calib_Ry(R_start_idx+1);
        QVector<qreal> calib_Grx(Gr_start_idx+1); QVector<qreal> calib_Gry(Gr_start_idx+1);
        QVector<qreal> calib_Gbx(Gb_start_idx+1); QVector<qreal> calib_Gby(Gb_start_idx+1);
        QVector<qreal> calib_Bx(B_start_idx+1); QVector<qreal> calib_By(B_start_idx+1);

        for(int n=0; n<R_start_idx+1; n++){
            calib_Rx[n] = R_real_y[n]-R_ideal_y[0];//横轴代表y值本身 纵轴代表y_ideal - y_real
            calib_Ry[n] = R_ideal_y[n]-R_real_y[n];
        }

        for(int n=0; n<Gr_start_idx+1; n++){
            calib_Grx[n] = Gr_real_y[n]-Gr_ideal_y[0];
            calib_Gry[n] = Gr_ideal_y[n]-Gr_real_y[n];
        }

        for(int n=0; n<Gb_start_idx+1; n++){
            calib_Gbx[n] = Gb_real_y[n]-Gb_ideal_y[0];
            calib_Gby[n] = Gb_ideal_y[n]-Gb_real_y[n];
        }

        for(int n=0; n<B_start_idx+1; n++){
            calib_Bx[n] = B_real_y[n]-B_ideal_y[0];
            calib_By[n] = B_ideal_y[n]-B_real_y[n];
        }

        qint16 cut_val_R = 0; qint16 cut_val_Gr = 0; qint16 cut_val_Gb = 0; qint16 cut_val_B = 0;
        qint16 blc_offset_R = 0; qint16 blc_offset_Gr = 0; qint16 blc_offset_Gb = 0; qint16 blc_offset_B = 0;
        if(R_ideal_y[0]>R_real_y[0]){//如果理想值大于实际值， cut_val >0 , blc offset=0
            cut_val_R = R_ideal_y[0]-R_real_y[0];
            blc_offset_R = 0;
        }
        else{//如果实际值>理想值， cut_val=0, blc_offset >0
            cut_val_R = 0;
            blc_offset_R = R_real_y[0]-R_ideal_y[0];
        }
        if(Gr_ideal_y[0]>Gr_real_y[0]){
            cut_val_Gr = Gr_ideal_y[0]-Gr_real_y[0];
            blc_offset_Gr = 0;
        }
        else{
            cut_val_Gr = 0;
            blc_offset_Gr = Gr_real_y[0]-Gr_ideal_y[0];
        }
        if(Gb_ideal_y[0]>Gb_real_y[0]){
            cut_val_Gb = Gb_ideal_y[0]-Gb_real_y[0];
            blc_offset_Gr = 0;
        }
        else{
            cut_val_Gb = 0;
            blc_offset_Gb = Gb_real_y[0]-Gb_ideal_y[0];
        }
        if(B_ideal_y[0]>B_real_y[0]){
            cut_val_B = B_ideal_y[0]-B_real_y[0];
            blc_offset_B = 0;
        }
        else{
            cut_val_B = 0;
            blc_offset_B = B_real_y[0]-B_ideal_y[0];
        }

        qint32 parameter_R[NLC_POINTS] = {0};
        qint32 parameter_Gr[NLC_POINTS] = {0};
        qint32 parameter_Gb[NLC_POINTS] = {0};
        qint32 parameter_B[NLC_POINTS] = {0};

        for(quint8 nlc_point=0; nlc_point<NLC_POINTS; nlc_point++){
            quint16 in = nlc_point<<right_shift;
            parameter_R[nlc_point]  = getParameter(in, calib_Rx, calib_Ry, R_start_idx)*64;
            parameter_Gr[nlc_point]  = getParameter(in, calib_Grx, calib_Gry, Gr_start_idx)*64;
            parameter_Gb[nlc_point]  = getParameter(in, calib_Gbx, calib_Gby, Gb_start_idx)*64;
            parameter_B[nlc_point]  = getParameter(in, calib_Bx, calib_By, B_start_idx+1)*64;
        }
        QDomNode data_x = data0_9.at(findIdx);

        QString grid_r_name("blc_grid_r_val");
        QString grid_gr_name("blc_grid_gr_val");
        QString grid_gb_name("blc_grid_gb_val");
        QString grid_b_name("blc_grid_b_val");
        QDomNode grid_r_node = data_x.namedItem(grid_r_name);
        QDomNode grid_gr_node = data_x.namedItem(grid_gr_name);
        QDomNode grid_gb_node = data_x.namedItem(grid_gb_name);
        QDomNode grid_b_node = data_x.namedItem(grid_b_name);
        Q_ASSERT(grid_r_node.isElement() && grid_gr_node.isElement() && grid_gb_node.isElement() && grid_b_node.isElement());
#define BLC_GRID_ROW 11
#define BLC_GRID_COL 11
        QString tmp;
        //11x11 grid val
        QStringList context_list_r = grid_r_node.toElement().text().split(',', QString::SkipEmptyParts);
        QStringList context_list_gr = grid_gr_node.toElement().text().split(',', QString::SkipEmptyParts);
        QStringList context_list_gb = grid_gb_node.toElement().text().split(',', QString::SkipEmptyParts);
        QStringList context_list_b = grid_b_node.toElement().text().split(',', QString::SkipEmptyParts);
        Q_ASSERT(context_list_r.size()==BLC_GRID_COL*BLC_GRID_ROW \
                 && context_list_gr.size()==BLC_GRID_COL*BLC_GRID_ROW \
                 && context_list_gb.size()==BLC_GRID_COL*BLC_GRID_ROW \
                 && context_list_b.size()==BLC_GRID_COL*BLC_GRID_ROW);
        for(int k=0; k<BLC_GRID_COL*BLC_GRID_ROW; k++){
            context_list_r[k] = QString::number(context_list_r[k].toInt() - blc_offset_R);
            context_list_gr[k] = QString::number(context_list_gr[k].toInt() - blc_offset_Gr);
            context_list_gb[k] = QString::number(context_list_gb[k].toInt() - blc_offset_Gb);
            context_list_b[k] = QString::number(context_list_b[k].toInt() - blc_offset_B);
        }
        tmp = context_list_r.join(", ");
        context_list_r.clear();
        grid_r_node.toElement().firstChild().setNodeValue(tmp);
        tmp.clear();
        tmp = context_list_gr.join(", ");
        context_list_gr.clear();
        grid_gr_node.toElement().firstChild().setNodeValue(tmp);
        tmp.clear();
        tmp = context_list_gb.join(", ");
        context_list_gb.clear();
        grid_gb_node.toElement().firstChild().setNodeValue(tmp);
        tmp.clear();
        tmp = context_list_b.join(", ");
        context_list_b.clear();
        grid_b_node.toElement().firstChild().setNodeValue(tmp);
        tmp.clear();

        QString nlc_r_tag_ame("nlc_correction_lut_r");
        QString nlc_gr_tag_ame("nlc_correction_lut_gr");
        QString nlc_gb_tag_ame("nlc_correction_lut_gb");
        QString nlc_b_tag_ame("nlc_correction_lut_b");
        QDomNode nlc_r_node = data_x.namedItem(nlc_r_tag_ame);
        QDomNode nlc_gr_node = data_x.namedItem(nlc_gr_tag_ame);
        QDomNode nlc_gb_node = data_x.namedItem(nlc_gb_tag_ame);
        QDomNode nlc_b_node = data_x.namedItem(nlc_b_tag_ame);
        Q_ASSERT(nlc_r_node.isElement() && nlc_gr_node.isElement() && nlc_gb_node.isElement() && nlc_b_node.isElement());

        for(int k=0; k<NLC_POINTS; k++){
            tmp.append(QString("%1, ").arg(parameter_R[k]));
        }

        nlc_r_node.toElement().firstChild().setNodeValue(tmp);
        tmp.clear();

        for(int k=0; k<NLC_POINTS; k++){
            tmp.append(QString("%1, ").arg(parameter_Gr[k]));
        }
        nlc_gr_node.toElement().firstChild().setNodeValue(tmp);
        tmp.clear();

        for(int k=0; k<NLC_POINTS; k++){
            tmp.append(QString("%1, ").arg(parameter_Gb[k]));
        }
        nlc_gb_node.toElement().firstChild().setNodeValue(tmp);
        tmp.clear();

        for(int k=0; k<NLC_POINTS; k++){
            tmp.append(QString("%1, ").arg(parameter_B[k]));
        }
        nlc_b_node.toElement().firstChild().setNodeValue(tmp);
        tmp.clear();

        QString cut_r_tag_ame("nlc_cut_r");
        QString cut_gr_tag_ame("nlc_cut_gr");
        QString cut_gb_tag_ame("nlc_cut_gb");
        QString cut_b_tag_ame("nlc_cut_b");
        QDomNode cut_r_node = data_x.namedItem(cut_r_tag_ame);
        QDomNode cut_gr_node = data_x.namedItem(cut_gr_tag_ame);
        QDomNode cut_gb_node = data_x.namedItem(cut_gb_tag_ame);
        QDomNode cut_b_node = data_x.namedItem(cut_b_tag_ame);
        Q_ASSERT(cut_r_node.isElement() && cut_gr_node.isElement() && cut_gb_node.isElement() && cut_b_node.isElement());

        tmp = QString::number(cut_val_R,10);
        cut_r_node.toElement().firstChild().setNodeValue(tmp);
        tmp = QString::number(cut_val_Gr, 10);
        cut_gr_node.toElement().firstChild().setNodeValue(tmp);
        tmp = QString::number(cut_val_Gb, 10);
        cut_gb_node.toElement().firstChild().setNodeValue(tmp);
        tmp = QString::number(cut_val_B, 10);
        cut_b_node.toElement().firstChild().setNodeValue(tmp);


        QFile xml_s("D:/qt_projection/mainwindow/mainwindow/out.xml");
        xml_s.open(QIODevice::WriteOnly|QIODevice::Text);
        QTextStream xml_save(&xml_s);
        doc.save(xml_save, 4);//indent 4 space
        xml_s.close();
    }
}

void ChartDialog::pfStartChangeR(int v)
{
    saveBtn->setEnabled(false);
    R_pf_2pt->replace(0, R->at(v));
}

void ChartDialog::pfEndChangeR(int v)
{
   saveBtn->setEnabled(false);
   R_pf_2pt->replace(1, R->at(v));
}

void ChartDialog::pfStartChangeGr(int v)
{
    saveBtn->setEnabled(false);
    Gr_pf_2pt->replace(0, Gr->at(v));
}

void ChartDialog::pfEndChangeGr(int v)
{
   saveBtn->setEnabled(false);
   Gr_pf_2pt->replace(1, Gr->at(v));
}

void ChartDialog::pfStartChangeGb(int v)
{
    saveBtn->setEnabled(false);
    Gb_pf_2pt->replace(0, Gb->at(v));
}

void ChartDialog::pfEndChangeGb(int v)
{
   saveBtn->setEnabled(false);
   Gb_pf_2pt->replace(1, Gb->at(v));
}

void ChartDialog::pfStartChangeB(int v)
{
    saveBtn->setEnabled(false);
    B_pf_2pt->replace(0, B->at(v));
}

void ChartDialog::pfEndChangeB(int v)
{
   saveBtn->setEnabled(false);
   B_pf_2pt->replace(1, B->at(v));
}


qreal ChartDialog::minQPointsX(const QVector<QPointF>& line) const
{
    if(line.isEmpty())
        return 0.0;
    qreal min = line[0].x();
    for(QPointF p:line){
        if(p.x()<min)
            min = p.x();
    }
    return min;
}
qreal ChartDialog::maxQPointsX(const QVector<QPointF>& line) const
{
    if(line.isEmpty())
        return 0.0;
    qreal max = line[0].x();
    for(QPointF p:line){
        if(p.x()>max)
            max = p.x();
    }
    return max;
}
qreal ChartDialog::minQPointsY(const QVector<QPointF>& line) const
{
    if(line.isEmpty())
        return 0.0;
    qreal min = line[0].y();
    for(QPointF p:line){
        if(p.y()<min)
            min = p.y();
    }
    return min;
}
qreal ChartDialog::maxQPointsY(const QVector<QPointF>& line)const
{
    if(line.isEmpty())
        return 0.0;
    qreal max = line[0].y();
    for(QPointF p:line){
        if(p.y()>max)
            max = p.y();
    }
    return max;
}

void ChartDialog::cal4ChannelPolyfitCurve(const qreal k1, const qreal b1, const qreal k2, const qreal b2, const qreal k3, const qreal b3, const qreal k4, const qreal b4)
{
    Q_ASSERT(R_pf && Gr_pf && Gb_pf && B_pf);
    int ct = R_pf->count();
    qreal p_x, p_y;
    for(int n=0; n<ct; n++){
        p_x = R_pf->at(n).x();

        p_y = p_x*k1+b1;
        R_pf->replace(n, p_x, p_y);

        p_y = p_x*k2+b2;
        Gr_pf->replace(n, p_x, p_y);

        p_y = p_x*k3+b3;
        Gb_pf->replace(n, p_x, p_y);

        p_y = p_x*k4+b4;
        B_pf->replace(n, p_x, p_y);
    }
}



void ChartDialog::createUI(int nlc_size)
{
    chartView = new ChartView(chart, this);
    chartView->setFocus();
    chartView->setRenderHint(QPainter::Antialiasing);

    polyfitBtn = new QPushButton(tr("拟合"), this);
    saveBtn = new QPushButton(tr("保存到XML"), this);
    saveBtn->setEnabled(false);

    QLabel* tips = new QLabel(tr("+放大, -缩小 \n 上下左右移动"), this);
    tips->setWordWrap(true);

    QLabel* pfText1 = new QLabel(tr("拟合起点"), this);
    pfText1->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel* pfText2 = new QLabel(tr("拟合终点"), this);
    pfText2->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel* pfTextR = new QLabel(tr("R:"), this);
    pfTextR->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel* pfTextGr = new QLabel(tr("Gr:"), this);
    pfTextGr->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel* pfTextGb = new QLabel(tr("Gb:"), this);
    pfTextGb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QLabel* pfTextB = new QLabel(tr("B:"), this);
    pfTextB->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    polyfitStartR = new SpinBox(this, true);
    polyfitStartR->setRange(2, nlc_size-1);
    polyfitStartR->setValue(nlc_size/2);//nlc_size >=6
    polyfitStartR->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    polyfitEndR = new SpinBox(this, false);
    polyfitEndR->setRange(2, nlc_size-1);
    polyfitEndR->setValue(nlc_size-1);
    polyfitEndR->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    polyfitStartR->setBro(polyfitEndR);
    polyfitEndR->setBro(polyfitStartR);

    polyfitStartGr = new SpinBox(this, true);
    polyfitStartGr->setRange(2, nlc_size-1);
    polyfitStartGr->setValue(nlc_size/2);
    polyfitStartGr->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    polyfitEndGr = new SpinBox(this, false);
    polyfitEndGr->setRange(2, nlc_size-1);
    polyfitEndGr->setValue(nlc_size-1);
    polyfitEndGr->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    polyfitStartGr->setBro(polyfitEndGr);
    polyfitEndGr->setBro(polyfitStartGr);

    polyfitStartGb = new SpinBox(this, true);
    polyfitStartGb->setRange(2, nlc_size-1);
    polyfitStartGb->setValue(nlc_size/2);
    polyfitStartGb->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    polyfitEndGb = new SpinBox(this, false);
    polyfitEndGb->setRange(2, nlc_size-1);
    polyfitEndGb->setValue(nlc_size-1);
    polyfitEndGb->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    polyfitStartGb->setBro(polyfitEndGb);
    polyfitEndGb->setBro(polyfitStartGb);

    polyfitStartB = new SpinBox(this, true);
    polyfitStartB->setRange(2, nlc_size-1);
    polyfitStartB->setValue(nlc_size/2);
    polyfitStartB->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    polyfitEndB = new SpinBox(this, false);
    polyfitEndB->setRange(2, nlc_size-1);
    polyfitEndB->setValue(nlc_size-1);
    polyfitEndB->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    polyfitStartB->setBro(polyfitEndB);
    polyfitEndB->setBro(polyfitStartB);

    R_visual = new QCheckBox(tr("显示R"), this);
    R_visual->setChecked(true);
    Gr_visual = new QCheckBox(tr("显示Gr"), this);
    Gr_visual->setChecked(true);
    Gb_visual = new QCheckBox(tr("显示Gb"), this);
    Gb_visual->setChecked(true);
    B_visual = new QCheckBox(tr("显示B"), this);
    B_visual->setChecked(true);

    resetGpBox = new QGroupBox(tr("复位"), this);
    reset_R = new QPushButton(tr("复位R"), resetGpBox);
    reset_Gr = new QPushButton(tr("复位Gr"), resetGpBox);
    reset_Gb = new QPushButton(tr("复位Gb"), resetGpBox);
    reset_B = new QPushButton(tr("复位B"), resetGpBox);
    QGridLayout* gp_layout = new QGridLayout;
    gp_layout->addWidget(reset_R, 0, 1, 1, 1, Qt::AlignCenter);
    gp_layout->addWidget(reset_Gr, 0, 2, 1, 1, Qt::AlignCenter);
    gp_layout->addWidget(reset_Gb, 1, 1, 1, 1, Qt::AlignCenter);
    gp_layout->addWidget(reset_B, 1, 2, 1, 1, Qt::AlignCenter);
    resetGpBox->setLayout(gp_layout);

    hlayout = new QHBoxLayout(this);
    glayout = new QGridLayout;
    glayout->addWidget(tips, 0, 0, 1, 3, Qt::AlignHCenter | Qt::AlignVCenter);
    glayout->addWidget(pfText1, 1, 1, 1, 1, Qt::AlignHCenter | Qt::AlignBottom);
    glayout->addWidget(pfText2, 1, 2, 1, 1, Qt::AlignHCenter| Qt::AlignBottom);
    glayout->addWidget(pfTextR, 2, 0, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    glayout->addWidget(pfTextGr, 3, 0, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    glayout->addWidget(pfTextGb, 4, 0, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    glayout->addWidget(pfTextB, 5, 0, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    glayout->addWidget(polyfitStartR, 2, 1, 1, 1, Qt::AlignCenter);
    glayout->addWidget(polyfitEndR, 2, 2, 1, 1, Qt::AlignCenter);
    glayout->addWidget(polyfitStartGr, 3, 1, 1, 1, Qt::AlignCenter);
    glayout->addWidget(polyfitEndGr, 3, 2, 1, 1, Qt::AlignCenter);
    glayout->addWidget(polyfitStartGb, 4, 1, 1, 1, Qt::AlignCenter);
    glayout->addWidget(polyfitEndGb, 4, 2, 1, 1, Qt::AlignCenter);
    glayout->addWidget(polyfitStartB, 5, 1, 1, 1, Qt::AlignCenter);
    glayout->addWidget(polyfitEndB, 5, 2, 1, 1, Qt::AlignCenter);
    glayout->addWidget(R_visual, 6, 1, 1, 1, Qt::AlignCenter);
    glayout->addWidget(Gr_visual, 6, 2, 1, 1, Qt::AlignCenter);
    glayout->addWidget(Gb_visual, 7, 1, 1, 1, Qt::AlignCenter);
    glayout->addWidget(B_visual, 7, 2, 1, 1, Qt::AlignCenter);
    glayout->addWidget(resetGpBox, 8, 0, 1, 3, Qt::AlignCenter);
    glayout->addWidget(polyfitBtn, 9, 1, 1, 2, Qt::AlignCenter);
    glayout->addWidget(saveBtn, 10, 1, 1, 2, Qt::AlignCenter);
    glayout->setSpacing(15);
    for(int row=0; row<11; row++)
        glayout->setRowStretch(row, 1);
    for(int col=0; col<3; col++)
        glayout->setColumnStretch(col, 0);
    hlayout->addWidget(chartView);
    hlayout->addLayout(glayout);
    setLayout(hlayout);
    resize(1024, 768);
    setWindowTitle(tr("Figure"));

}

qint16 ChartDialog::getParameter(quint16 in, QVector<qreal>& x, QVector<qreal>& y, int size)
{
    if(in<x[0]){
        if(y[0]>=0.0){
            return qint16(y[0]+0.5);
        }
        else{
            return qint16(y[0]-0.5);
        }
    }
    if(in>=x[size-1]){
        return 0;
    }
    int i = 0;
    for(; i<size-1; i++){
        if(in>=x[i] && in<x[i+1])
            break;
    }
    qreal ret;
    if(x[i+1]==x[i])
        ret = (y[i+1]+y[i])/2;
    else
        ret = y[i]+(y[i+1]-y[i])*(in-x[i])/(x[i+1]-x[i]);
    if(ret>=0)
        return qint16(ret+0.5);
    else
        return qint16(ret-0.5);
}

//控制4条曲线及其拟合直线的显示、隐藏
void ChartDialog::showR(bool checked)
{
    R->setVisible(checked);
    R_pf_2pt->setVisible(checked);
    if(checked && polyfit_success)
        R_pf->setVisible(true);
    else
        R_pf->setVisible(false);
}
void ChartDialog::showGr(bool checked)
{
    Gr->setVisible(checked);
    Gr_pf_2pt->setVisible(checked);
    if(checked && polyfit_success)
        Gr_pf->setVisible(true);
    else
        Gr_pf->setVisible(false);
}
void ChartDialog::showGb(bool checked)
{
    Gb->setVisible(checked);
    Gb_pf_2pt->setVisible(checked);
    if(checked && polyfit_success)
        Gb_pf->setVisible(true);
    else
        Gb_pf->setVisible(false);
}
void ChartDialog::showB(bool checked)
{
    B->setVisible(checked);
    B_pf_2pt->setVisible(checked);
    if(checked && polyfit_success)
        B_pf->setVisible(true);
    else
        B_pf->setVisible(false);
}

//恢复曲线初始值，在用户移动了点以后，如果想恢复到初始状态
void ChartDialog::onResetR()
{
    R->replace(origin_R);
}

void ChartDialog::onResetGr()
{
    Gr->replace(origin_Gr);
}

void ChartDialog::onResetGb()
{
    Gb->replace(origin_Gb);
}

void ChartDialog::onResetB()
{
    B->replace(origin_B);
}

//------------------class spinbox----------------
SpinBox::SpinBox(QWidget *parent, bool isSt):
    QSpinBox(parent),
    bro(NULL),
    isStart(isSt)
{
    lineEdit()->setReadOnly(true);
}

void SpinBox::stepBy(int steps)//reimplement stepBy, 用于在setValue之前做判断
{
    int min = this->minimum();
    int max = this->maximum();
    int old = this->value();
    if((old+steps)>max || (old+steps)<min)
        return;//do nothing
    if(this->isStart){
        if(bro==NULL)
            return;
        int end = this->bro->value();
        if((end-old-steps)<4)
            return;
    }
    else{
        if(bro==NULL)
            return;
        int begin = this->bro->value();
        if((old+steps-begin)<4)
            return;
    }
    QAbstractSpinBox::stepBy(steps);
}


