#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")  //fuck MSVC complior, use UTF-8, not gb2312/gbk
#endif

//#define USE_OPENCV_DEMOSAIC
#ifdef USE_OPENCV_DEMOSAIC
#include "opencv2/opencv.hpp"
#endif

#include "inc/mainwindow.h"
#include "ui_mainwindow.h"
#include "Python.h"
#include <QImage>
#include <QtMath>
#include <QtAlgorithms>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QVector>
#include <QMap>
#include "inc/chartdialog.h"
#include "inc/blcdialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    preWorkPath(QDir::currentPath()),
    aboutThisApp(NULL),
    rawImgPtr(NULL),
    nlc_vec(0),
    rawinfoDlg(NULL),
    curRawIdx(0),
    ui(new Ui::MainWindow),
    imgLabel(new ImgLabel(this))
{
    ui->setupUi(this);
    ui->dockWidgetContents->setLayout(ui->gridLayout);
    ui->statusBar->setStyleSheet("color: yellow; background-color: rgb(0, 102, 190)");
    ui->scrollArea->setBackgroundRole(QPalette::Dark);
    setCentralWidget(ui->scrollArea);

    ui->scrollArea->setWidget(imgLabel);
    //ui->scrollArea->setWidgetResizable(true);
    imgLabel->resize(300, 100);

    ui->actionOpenNLC->setShortcut(QKeySequence(tr("Ctrl+N")));//设置快捷键
    ui->actionOpenNLC->setStatusTip(tr("open NLC raw files..."));
    ui->action_Open_BLC_raw->setShortcut(QKeySequence(tr("Ctrl+B")));
    ui->action_Open_BLC_raw->setStatusTip(tr("open BLC raw files..."));
    ui->actionSave->setShortcut(QKeySequence::Save);
    ui->actionSave->setStatusTip(tr("save to harddisk..."));

    ui->action_Zoom_in->setShortcut(QKeySequence::ZoomIn);
    ui->action_Zoom_in->setStatusTip(tr("scale up image"));
    ui->action_Zoom_out->setShortcut(QKeySequence::ZoomOut);
    ui->action_Zoom_out->setStatusTip(tr("scale down image"));

    connect(ui->action_Zoom_in, &QAction::triggered, imgLabel, &ImgLabel::zoom_in);//交给img label处理放大缩小
    connect(ui->action_Zoom_out, &QAction::triggered, imgLabel, &ImgLabel::zoom_out);

    connect(ui->previousButton, &QPushButton::clicked, this, &MainWindow::onPreButton);
    connect(ui->nextButton, &QPushButton::clicked, this, &MainWindow::onNextButton);
    connect(ui->finishButton, &QPushButton::clicked, this, &MainWindow::onFinishButton);
    connect(ui->syncAllRoi, &QPushButton::clicked, this, &MainWindow::onSyncAllRoiButton);

    connect(imgLabel, &ImgLabel::sendRoi, this, &MainWindow::recieveRoi);//接收绘图传过来的roi
    Py_Initialize();//多次初始化导致import报错，因此放在mainwindow构造里面，保证打开即初始化python
    if(!Py_IsInitialized()){
        QMessageBox::critical(this, tr("python env error"), tr("init python fail"), QMessageBox::Ok);
    }
}

MainWindow::~MainWindow()
{
    if(rawImgPtr != NULL){
        delete[] rawImgPtr;
        rawImgPtr = NULL;
    }
    //ui->tableWidget->clearContents();//释放所有的item内存
    int row = ui->tableWidget->rowCount();
    int col = ui->tableWidget->columnCount();
    for(int h=0; h<row; h++){
        for(int w=0; w<col; w++){
            QTableWidgetItem* item = ui->tableWidget->item(h, w);
            if(item!=NULL)
                delete item;
        }
    }
    Py_Finalize();
    delete ui;
}

void MainWindow::onPreButton()
{
    /*if(!imgLabel->getPainted()){
        QMessageBox::critical(this, tr("warning"), tr("please paint roi for current raw!"), QMessageBox::Ok);
        return;
    }*/
    if(curRawIdx==0){
        QMessageBox::information(this, tr("information"), tr("current raw is the first one"), QMessageBox::Ok);
        return;
    }
    curRawIdx--;
    if(curRawIdx>=0 && curRawIdx<nlc_vec.size() && rawinfoDlg!=NULL)
        handleRawInput(*rawinfoDlg, nlc_vec[curRawIdx]);
}

void MainWindow::onNextButton()
{
    if(curRawIdx==255 || curRawIdx==nlc_vec.size()-1){
        QMessageBox::information(this, tr("information"), tr("current raw is the last one"), QMessageBox::Ok);
        return;
    }
    curRawIdx++;
    if(curRawIdx>=0 && curRawIdx<nlc_vec.size() && rawinfoDlg!=NULL)
        handleRawInput(*rawinfoDlg, nlc_vec[curRawIdx]);
}

void MainWindow::onFinishButton()
{
//    if(nlc_vec.isEmpty()){
//        QMessageBox::critical(this, tr("error"), tr("no raw inputed !"), QMessageBox::Ok);
//        return;
//    }
    QVector<nlccollection>::size_type nlc_size = nlc_vec.size();
    if(nlc_size<=6){
        QMessageBox::warning(this, tr("warning"), tr("at least 6 raw is needed."), QMessageBox::Ok);
        return;
    }
    for(QVector<nlccollection>::size_type idx = 0; idx<nlc_vec.size(); idx++){
        QTableWidgetItem* item4 = ui->tableWidget->item(idx, 4);
        if(item4==NULL){
            item4 = new QTableWidgetItem();
            ui->tableWidget->setItem(idx, 4, item4);
        }
        item4->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);

        if(nlc_vec[idx].checkParameters()){
            nlc_vec[idx].calRoiAvgValue();
            QString r = QString::number(nlc_vec[idx].getAvgR(), 'g', 5);
            QString gr = QString::number(nlc_vec[idx].getAvgGr(), 'g', 5);
            QString gb = QString::number(nlc_vec[idx].getAvgGb(), 'g', 5);
            QString b =  QString::number(nlc_vec[idx].getAvgB(), 'g', 5);
            item4->setText(tr("[%1, %2, %3, %4]").arg(r).arg(gr).arg(gb).arg(b));
            item4->setBackgroundColor(QColor::fromRgb(0,128,0));
        }
        else{
            item4->setText(tr("[0, 0, 0, 0]"));
            item4->setBackgroundColor(QColor::fromRgb(128,0,0));
        }
    }


    QVector<QPointF> lineR(nlc_size);
    QVector<QPointF> lineGr(nlc_size);
    QVector<QPointF> lineGb(nlc_size);
    QVector<QPointF> lineB(nlc_size);
    for(QVector<nlccollection>::size_type n = 0; n<nlc_size; n++){//倒序，因为最亮的在前面，绘图要放后面
        lineR[n].setX(nlc_vec[nlc_size-1-n].getShut());
        lineR[n].setY(nlc_vec[nlc_size-1-n].getAvgR());
        lineGr[n].setX(nlc_vec[nlc_size-1-n].getShut());
        lineGr[n].setY(nlc_vec[nlc_size-1-n].getAvgGr());
        lineGb[n].setX(nlc_vec[nlc_size-1-n].getShut());
        lineGb[n].setY(nlc_vec[nlc_size-1-n].getAvgGb());
        lineB[n].setX(nlc_vec[nlc_size-1-n].getShut());
        lineB[n].setY(nlc_vec[nlc_size-1-n].getAvgB());
    }

    //为什么要用堆创建，因为使用栈，莫名其妙关闭就崩溃。。甚至堆创建手动delete也崩溃。。。如果你知道怎么回事请一定要告诉我
    ChartDialog* chartDlg = new ChartDialog(this, lineR, lineGr, lineGb, lineB, nlc_size, nlc_vec[0].getISO());
    //chartDlg->setAttribute(Qt::WA_DeleteOnClose);
    chartDlg->exec();
    delete chartDlg;
}

void MainWindow::onSyncAllRoiButton()
{
    if(nlc_vec.isEmpty() || curRawIdx>=nlc_vec.size())
        return;
    ROI_t r = nlc_vec[curRawIdx].getRoi();
    for(QVector<nlccollection>::size_type idx = 0; idx < nlc_vec.size(); idx++){
        nlc_vec[idx].setRoi(r);
        QTableWidgetItem* item3 = ui->tableWidget->item(idx, 3);//roi 显示在第三列
        if(item3==NULL){
            item3 = new QTableWidgetItem(tr("[%1,%2,%3,%4]").arg(r.top).arg(r.left).arg(r.bottom).arg(r.right));
            ui->tableWidget->setItem(idx, 3, item3);
            item3->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        }
        else{
            item3->setText(tr("[%1,%2,%3,%4]").arg(r.top).arg(r.left).arg(r.bottom).arg(r.right));
        }
        item3->setBackgroundColor(QColor::fromRgb(0,128,0));
    }
}

void MainWindow::recieveRoi(const ROI_t &roi)
{
    if(nlc_vec.isEmpty() || curRawIdx>=nlc_vec.size())
        return;
    ROI_t r = {roi.left*2, roi.right*2, roi.top*2, roi.bottom*2};//放大2倍到实际值 因为显示的图像是downscale 1/2的
    nlc_vec[curRawIdx].setRoi(r);
    QTableWidgetItem* item3 = ui->tableWidget->item(curRawIdx, 3);//roi 显示在第三列
    if(item3==NULL){
        item3 = new QTableWidgetItem(tr("[%1,%2,%3,%4]").arg(r.top).arg(r.left).arg(r.bottom).arg(r.right));
        ui->tableWidget->setItem(curRawIdx, 3, item3);
    }
    else{
        item3->setText(tr("[%1,%2,%3,%4]").arg(r.top).arg(r.left).arg(r.bottom).arg(r.right));
    }
    item3->setBackgroundColor(QColor::fromRgb(0,128,0));
    item3->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
}

void MainWindow::on_actionAbout_this_App_triggered()
{
    if(aboutThisApp == NULL){
        aboutThisApp = new AboutDialog(this);
        aboutThisApp->setWindowTitle(tr("about this app"));
        //aboutThisApp->exec();
    }
    aboutThisApp->exec();
}

void MainWindow::on_actionOpenNLC_triggered()
{
    //QFileDialog openRawDlg(this, tr("open file"), preWorkPath, "raw file(*.raw);;All file(*.*)");
    //openRawDlg.setFileMode(QFileDialog::ExistingFiles);
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("open file"), preWorkPath, "raw file(*.raw);;All file(*.*)");
    //if(openRawDlg.exec())
    //   fileNames = openRawDlg.selectedFiles();
    QStringList rawNames;
    for(QStringList::iterator it = fileNames.begin(); it != fileNames.end(); it++){
        if(it->endsWith(".raw") || it->endsWith(".RAW")){
            rawNames.append(*it);
        }
    }

    if(rawNames.size()>0){
        ui->statusBar->showMessage(tr("open %1 raw").arg(rawNames.size()), 2000);
        QFileInfo workPathInfo(fileNames[0]);
        preWorkPath = workPathInfo.absolutePath();//记录打开的目录，下一次open file直接定位到这个目录下
    }
    else{
        ui->statusBar->showMessage(tr("no raw file selected!"), 2000);
        return;
    }

#if 0
    if(fileName.endsWith(QString(".jpg"))
            || fileName.endsWith(QString(".png"))
            || fileName.endsWith(QString(".JPG"))
            || fileName.endsWith(QString(".PNG")))
    {
        if(rawImgPtr != NULL){//如果上一次打开raw，则把这个buffer释放
            delete[] rawImgPtr;
            rawImgPtr = NULL;
        }
        QImage showIm(fileName);
        imgLabel->setPixmap(QPixmap::fromImage(showIm));
        imgLabel->adjustSize();
        imgFactor = 1.0;
        imgOriginSize = imgLabel->pixmap()->size();
    }
#endif

    if(rawinfoDlg == NULL)
        rawinfoDlg = new rawinfoDialog(this);
    QFileInfo firstRawFileInfo(rawNames[0]);
    qint64 raw_sz = firstRawFileInfo.size();
    if(raw_sz%24==0){//default scale 4:3
         qreal unit_len = qSqrt(raw_sz/24);
         if(unit_len-qint64(unit_len) == 0.0){
             rawinfoDlg->setRawWidth(4*quint16(unit_len));
             rawinfoDlg->setRawHeight(3*quint16(unit_len));
         }
    }
    if(rawinfoDlg->exec() == rawinfoDialog::Accepted)
    {
        nlc_vec.clear();//清除上一次的数据
        ui->tableWidget->clearContents();//清除前一次的所有tabel item
        for(QStringList::iterator it=rawNames.begin(); it!=rawNames.end(); it++)
        {
            nlccollection nl(*it, rawinfoDlg->getRawSize().width(), rawinfoDlg->getRawSize().height(), rawinfoDlg->getBayer(), rawinfoDlg->getBitDepth());
            //operator<<(qDebug(), nl);
            nlc_vec.append(nl);
        }
        //---------------------检查ISO一致---------------------
        int iso = nlc_vec[0].getISO();
        bool ISO_flag = true;
        for(int idx=1; idx<nlc_vec.size(); idx++){
            if(iso!=nlc_vec[idx].getISO()){
                ISO_flag = false;
                break;
            }
        }
        if(!ISO_flag){
            int ret = QMessageBox::warning(this, tr("警告"), tr("打开的raw文件ISO不一致，是否继续?"), QMessageBox::Yes, QMessageBox::No);
            if(ret==QMessageBox::No){
                nlc_vec.clear();
                return;
            }
        }
        //--------------------------------------------------

        qSort(nlc_vec.begin(), nlc_vec.end());//根据曝光时间排序，亮的放前面， nlc_vec 已重载operator<
        ui->tableWidget->setRowCount(nlc_vec.size());
        ui->tableWidget->setColumnCount(5);
        QStringList tableHeader = {tr("raw name"), tr("shutter(ms)"), tr("ISO"), tr("roi [t,l,b,r]"), tr("roi [R/Gr/Gb/B]")};
        ui->tableWidget->setHorizontalHeaderLabels(tableHeader);

        for(QVector<nlccollection>::size_type row = 0; row<nlc_vec.size(); row++){
            QTableWidgetItem* item0 = ui->tableWidget->item(row, 0);
            QFileInfo info(nlc_vec[row].getRawName());
            QString fn = info.fileName();

            if(item0 == NULL){
                item0 = new QTableWidgetItem(fn);
                ui->tableWidget->setItem(row, 0, item0);
            }
            else{
                item0->setText(fn);
            }
            item0->setBackgroundColor(QColor::fromRgb(0,128,0));
            item0->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);

            QTableWidgetItem* item1 = ui->tableWidget->item(row, 1);
            if(item1 == NULL){
                item1 = new QTableWidgetItem(tr("%1").arg(nlc_vec[row].getShut()));
                ui->tableWidget->setItem(row, 1, item1);
            }
            else{
                item1->setText(tr("%1").arg(nlc_vec[row].getShut()));
            }
            item1->setBackgroundColor(QColor::fromRgb(0,128,0));
            item1->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);

            QTableWidgetItem* item2 = ui->tableWidget->item(row, 2);
            if(item2 == NULL){
                item2 = new QTableWidgetItem(tr("%1").arg(nlc_vec[row].getISO()));
                ui->tableWidget->setItem(row, 2, item2);
            }
            else{
                item2->setText(tr("%1").arg(nlc_vec[row].getISO()));
            }
            item2->setBackgroundColor(QColor::fromRgb(0,128,0));
            item2->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);

        }
        curRawIdx = 0;
        handleRawInput(*rawinfoDlg, nlc_vec[curRawIdx]);//最亮的排前面显示
    }
}

void MainWindow::on_actionExit_triggered()
{
    int ret = QMessageBox::question(this, tr("Quit"), tr("Do you want to quit?"));
    if(ret == QMessageBox::Yes)
        qApp->quit();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_actionSave_triggered()
{
    QString saveJson = QFileDialog::getSaveFileName(this, tr("save as .."), preWorkPath, "json file(*.json);;All file(*.*)");
    if(saveJson.isEmpty())
        return;
    int sz = nlc_vec.size();
    if(sz<=0)
        return;
    for(QVector<nlccollection>::iterator it=nlc_vec.begin(); it!=nlc_vec.end(); it++){
        if(!(it->checkParameters())){
            QMessageBox::information(this, tr("error"), tr("请给所有的raw选择ROI"), QMessageBox::Ok);
            return;
        }
    }
    QJsonArray rawJsObjArr;
    for(int idx=0; idx<sz; idx++){
        QJsonObject roi = {{"top", int(nlc_vec[idx].getRoi().top)},
                           {"bottom", int(nlc_vec[idx].getRoi().bottom)},
                           {"left", int(nlc_vec[idx].getRoi().left)},
                           {"right", int(nlc_vec[idx].getRoi().right)}};
        QJsonObject rawJsObj = {{"name", nlc_vec[idx].getRawName()},
                                {"ISO", nlc_vec[idx].getISO()},
                                {"shutter(ms)", nlc_vec[idx].getShut()},
                                {"roi", QJsonValue(roi)}};
        rawJsObjArr.append(QJsonValue(rawJsObj));
    }
    QJsonDocument rawJsDoc(rawJsObjArr);
    QFile jsFile(saveJson);
    if(!(jsFile.open(QIODevice::Text|QIODevice::WriteOnly|QIODevice::Truncate))){
        QMessageBox::information(this, tr("error"), tr("create %1 fail").arg(saveJson), QMessageBox::Ok);
        return;
    }
    jsFile.write(rawJsDoc.toJson());
    jsFile.close();
}

void MainWindow::handleRawInput(const rawinfoDialog& rawinfoDlg, const nlccollection& nl)
{
    quint16 rawWidth = rawinfoDlg.getRawSize().width();
    quint16 rawHeight = rawinfoDlg.getRawSize().height();
    quint32 rawSize = rawWidth * rawHeight;
    quint8 rawBitDepth = rawinfoDlg.getBitDepth();

    QFileInfo rawfileInfo(nl.getRawName());

    quint8 rawByte = (rawBitDepth%8 > 0)? (rawBitDepth/8 + 1) : rawBitDepth/8;
    if(rawByte > 2){
        return;
    }

    quint32 rawBufferSize = rawSize * rawByte;

    if(rawBufferSize != rawfileInfo.size()){//检查输入的raw大小信息和文件本身大小是否一致
       return;
    }

    if(rawImgPtr != NULL){//释放之前打开的raw
        delete[] rawImgPtr;
        rawImgPtr = NULL;
    }
    rawImgPtr = new quint8[rawBufferSize];

    if(rawImgPtr == NULL){
        return;
    }

    QFile rawFile(nl.getRawName());

    if(!rawFile.open(QIODevice::ReadOnly)){
        delete[] rawImgPtr;
        rawImgPtr = NULL;
        return;
    }

    if(-1 == rawFile.read((char*)rawImgPtr, rawBufferSize)){
        delete[] rawImgPtr;
        rawImgPtr = NULL;
        rawFile.close();
        return;
    }
    rawFile.close();

    //--------------downscale 1/4 to 8bit raw-------------
    //if(rawByte == 2)
    //{
        quint8* rShift = new quint8[(rawHeight/2)*(rawWidth/2)];
        if(rShift == NULL){
            return;
        }
        quint16 r_1, c_1;
        for(quint16 col = 0; col < rawWidth/2; col++){
            for(quint16 row = 0; row < rawHeight/2; row++){
                if(row%2==0)
                    r_1 = 2*row;
                else
                    r_1 = 2*row-1;
                if(col%2==0)
                    c_1 = 2*col;
                else
                    c_1 = 2*col-1;
                if(rawByte==2){
                    rShift[row*(rawWidth/2)+col] = ((quint16*)rawImgPtr)[r_1*rawWidth+c_1]>>(rawBitDepth-8);  //举例，如果输入位宽12bit，则右移12-8=4位，然后放到8bit buffer中
                }
                else{
                    rShift[row*(rawWidth/2)+col] = ((quint16*)rawImgPtr)[r_1*rawWidth+c_1];
                }
            }
        }
        delete[] rawImgPtr;
        rawImgPtr = rShift;
    //}

#ifdef USE_OPENCV_DEMOSAIC
    cv::Mat raw_data(rawHeight/2, rawWidth/2, CV_8UC1, (void*)rawImgPtr); //downscale
    cv::Mat rgb_data;
    rawinfoDialog::bayerMode bm = rawinfoDlg.getBayer();
    if(bm == rawinfoDialog::RG)
        cv::cvtColor(raw_data, rgb_data, cv::COLOR_BayerRG2RGB);
    else if(bm == rawinfoDialog::GR)
        cv::cvtColor(raw_data, rgb_data, cv::COLOR_BayerGR2RGB);
    else if(bm == rawinfoDialog::GB)
        cv::cvtColor(raw_data, rgb_data, cv::COLOR_BayerGB2RGB);
    else if(bm == rawinfoDialog::BG)
        cv::cvtColor(raw_data, rgb_data, cv::COLOR_BayerBG2RGB);
    QImage showIm(rgb_data.data, rawWidth/2, rawHeight/2, QImage::Format_RGB888);
#else
    QImage showIm(rawImgPtr, rawWidth/2, rawHeight/2, QImage::Format_Grayscale8);
#endif
    ROI_t r = nl.getRoi();
    r.bottom = r.bottom/2; r.top = r.top/2; r.left = r.left/2; r.right = r.right/2; //nl记录的是实际roi，imagelabel中缩小到1/4
    if(r.bottom!=r.top && r.left!=r.right){
        imgLabel->setPainted(true);
        imgLabel->setRoi(r);
    }
    else{
        imgLabel->setPainted(false);
    }

    imgLabel->pix = QPixmap::fromImage(showIm);
    imgLabel->setPixmap(imgLabel->pix);
    imgLabel->setTmpPix(imgLabel->pix);
    //imgLabel->setImgFactor(1.0);//构造函数默认1.0，所以这里不再设置
    imgLabel->setImgOriginSize(imgLabel->pixmap()->size());
    imgLabel->resize(imgLabel->getImgFactor()*imgLabel->pixmap()->size());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    int ret = QMessageBox::question(this, tr("Quit"), tr("Do you want to quit?"));
    if(ret == QMessageBox::Yes){
        QMainWindow::closeEvent(event);//交给父类处理，默认退出程序
    }
    else{
        event->ignore();//不处理关闭消息，继续广播转发，已经到了顶层widget，所以这个消息没有组件处理，程序不退出
    }
}

void MainWindow::on_action_Open_BLC_raw_triggered()
{
    QStringList blc_raws =  QFileDialog::getOpenFileNames(this, tr("打开BLC raw"), preWorkPath, "raw file(*.raw);;All file(*.*)");
    if(blc_raws.size()==0)
        return;
    ui->statusBar->showMessage(tr("open %1 raw").arg(blc_raws.size()), 2000);
    QFileInfo firstRawFileInfo(blc_raws[0]);
    preWorkPath = firstRawFileInfo.absolutePath();//记录打开的目录，下一次open file直接定位到这个目录下
    rawinfoDialog dlg;
    qint64 raw_sz = firstRawFileInfo.size();
    if(raw_sz%24==0){//default scale 4:3
         qreal unit_len = qSqrt(raw_sz/24);
         if(unit_len-qint64(unit_len) == 0.0){
             dlg.setRawWidth(4*quint16(unit_len));
             dlg.setRawHeight(3*quint16(unit_len));
         }
    }
    QMap<qint32, QStringList> blc_fn_map;
    if(rawinfoDialog::Accepted == dlg.exec()){
        rawinfoDialog::bayerMode bm = dlg.getBayer();
        quint16 bd = dlg.getBitDepth();
        QSize rawsz = dlg.getRawSize();
        quint8 rawByte = (bd%8 > 0)? (bd/8 + 1) : bd/8;
        quint32 file_sz = rawsz.width()*rawsz.height()*rawByte;
        for(QStringList::const_iterator it = blc_raws.cbegin(); it!=blc_raws.cend(); it++){
            QFileInfo raw_info(*it);
            QString fn = raw_info.fileName();
            if(file_sz != raw_info.size()){
                QMessageBox::warning(this, tr("raw size error"), tr("%1 size mismatch with your input").arg(fn), QMessageBox::Ok);
                continue;
            }            
            QStringList rawfn_split = fn.split('_', QString::SkipEmptyParts);
            int iso_idx = rawfn_split.indexOf("ISO");
            if(iso_idx<0){
                QMessageBox::warning(this, tr("raw name error"), tr("%1 name have no ISO tag").arg(fn), QMessageBox::Ok);
                continue;
            }
            bool cvtF = false;
            qint32 iso = rawfn_split[iso_idx+1].toInt(&cvtF, 10);
            if(cvtF==false){
                QMessageBox::warning(this, tr("raw name error"), tr("%1 name have no ISO number").arg(fn), QMessageBox::Ok);
                continue;
            }
            if(blc_fn_map.contains(iso))
                blc_fn_map[iso].append(*it);
            else
                blc_fn_map.insert(iso, QStringList(*it));
        }
        if(blc_fn_map.size()>10 || blc_fn_map.size()<=0){
            QMessageBox::information(this, tr("error"), tr("ISO种类必须在1-10范围内，请选择(含)10种ISO以内的raw文件"), QMessageBox::Ok);
            return;
        }
        BLCDialog blc_dlg(this, blc_fn_map, bm, rawsz, bd); // iso-fn map/ bayer / raw size/ bitdepth
        blc_dlg.exec();
    }
}
