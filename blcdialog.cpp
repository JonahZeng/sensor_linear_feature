#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")  //fuck MSVC complior, use UTF-8, not gb2312/gbk
#endif

#include <QVBoxLayout>
#include <QFileInfo>
#include <QPainter>
#include <QPaintEvent>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include "inc/blcdialog.h"
#include <QtDataVisualization/QValue3DAxis>
//#include <QDebug>

BLCDialog::BLCDialog(QWidget *parent, const QMap<qint32, QStringList>& blc_map, rawinfoDialog::bayerMode bm, QSize rawsz, quint16 bd) :
    QDialog(parent),
    useTip(new QLabel(tr("<font size='-1'><p>blc计算方法:</p>"
                         "<p>1.将选中的raw文件按照ISO分组</p>"
                         "<p>2.每一组ISO的raw叠加求平均</p>"
                         "<p>3.11x11范围平均模糊</p>"
                         "<p>4.长宽都缩小至1/16</p>"
                         "<p>5.水平、竖直方向拟合(2阶)</p>"
                         "<p>6.按照11x11格点位置取拟合后的值</p>"
                         "<p>7.不满10组iso的情况，将使用最后一组参数填充至10组</p></font>"
                         "<p>双击下面的raw文件名滚动到对应的标定参数位置</p>"
                         "<p>点击\"保存xml\"可保存<b>V300</b>的blc xml</p>"), this)),
    treeWgt(new QTreeWidget(this)),
    imgView(new gridImgLabel(this, "0x0", NULL)),
    grid_box(new QGroupBox(tr("grid option"), this)),
    noGrid(new QRadioButton(tr("no grid"), this)),
    grid5_5(new QRadioButton(tr("grid 5x5"), this)),
    grid11_11(new QRadioButton(tr("grid 11x11"), this)),
    blcDataEdit(new QPlainTextEdit(this)),
    blcDataChanged(false),
    xmlFn(),//需要选择一个文件绑定这个文件到blcDateEdit
    preWorkPath(QDir::currentPath()),//保存xml的路径
    saveXml(new QPushButton(tr("保存xml"), this)),
    saveAsXml(new QPushButton(tr("另存为"), this)),
    xmlDoc(new QDomDocument),
    hlayout(new QHBoxLayout(this)),
    blc_fn_map(blc_map),
    bayerMode(bm),
    rawSize(rawsz),
    bitDepth(bd),
    showImgBuf(NULL),
    xmlHL(NULL),
    threeDSurface(new Q3DSurface),
    surfaceContainerWgt(QWidget::createWindowContainer(threeDSurface)),
    aeGain_surfaceData_4p_map(),
    showOnSreenSeries(new QSurface3DSeries)
{
    if(!threeDSurface->hasContext()){
        QMessageBox::critical(this, tr("error"), tr("init opengl fail..."), QMessageBox::Ok);
    }
    threeDSurface->setAxisX(new QValue3DAxis);
    threeDSurface->setAxisY(new QValue3DAxis);
    threeDSurface->setAxisZ(new QValue3DAxis);

    blcDataEdit->setWordWrapMode(QTextOption::NoWrap);
    blcDataEdit->setCenterOnScroll(true);
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

    QVBoxLayout* bayerVerLayout = new QVBoxLayout;//设置3d surface bayer选项radiobutton排列
    QGroupBox* surfaceBox = new QGroupBox(tr("显示"));
    surface_r = new QRadioButton(tr("R"));
    surface_gr = new QRadioButton(tr("Gr"));
    surface_gb = new QRadioButton(tr("Gb"));
    surface_b = new QRadioButton(tr("B"));
    bayerVerLayout->addWidget(surface_r);
    bayerVerLayout->addWidget(surface_gr);
    bayerVerLayout->addWidget(surface_gb);
    bayerVerLayout->addWidget(surface_b);
    surfaceBox->setLayout(bayerVerLayout);
    surface_r->setChecked(true);
    surface_gr->setChecked(false);
    surface_gb->setChecked(false);
    surface_b->setChecked(false);

    QHBoxLayout* surfaceAreaLayout = new QHBoxLayout;//设置3d surface整块区域水平layout
    surfaceAreaLayout->addWidget(surfaceContainerWgt, 1);
    surfaceAreaLayout->addWidget(surfaceBox);

    imgView->setFrameStyle(QFrame::Box);
    imgView->setScaledContents(true);

    QVBoxLayout* middleLayout = new QVBoxLayout;
    middleLayout->addWidget(imgView, 1);
    middleLayout->addLayout(surfaceAreaLayout, 1);
    hlayout->addLayout(middleLayout, 1); //设置中间包括img显示部分，和3d suface显示部分的layout

    QVBoxLayout* rightVerLayout = new QVBoxLayout;
    rightVerLayout->addWidget(blcDataEdit, 1);
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(saveXml, 1);
    buttonLayout->addWidget(saveAsXml, 1);
    buttonLayout->addStretch(2);
    rightVerLayout->addLayout(buttonLayout);
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
    connect(saveXml, &QPushButton::clicked, this, &BLCDialog::onSaveXmlButton);
    connect(saveAsXml, &QPushButton::clicked, this, &BLCDialog::onSaveAsButton);
    connect(blcDataEdit->document(), &QTextDocument::contentsChanged, this, &BLCDialog::onContextChanged);


    int totalTasks = 0;
    for(QMap<qint32, QStringList>::iterator it=blc_fn_map.begin(); it!=blc_fn_map.end(); it++){
        QStringList files = it.value();
        totalTasks += files.size();
    }
    clacBLCprogress = new CalcBlcProgressDlg(this, totalTasks);//创建进度对话框

    calcBLCthread = new calcBlcThread(this, blc_fn_map, bayerMode, rawSize, bitDepth, xmlDoc, docRoot, quint16(totalTasks), &aeGain_surfaceData_4p_map);//创建新线程处理blc raw,并且通知对话框进度更新，完成后自动关闭

    connect(calcBLCthread, &calcBlcThread::currentTaskId, clacBLCprogress, &CalcBlcProgressDlg::handleCurrentTaskId);
    connect(calcBLCthread, &calcBlcThread::pyInitFail, clacBLCprogress, &CalcBlcProgressDlg::reject);//如果Py初始化失败，则进度对话框关闭并导致显示错误信息
    connect(calcBLCthread, &calcBlcThread::destroyed, this, &BLCDialog::onThreadDestroyed);
    //connect(calcBLCthread, &calcBlcThread::finished, calcBLCthread, &QObject::deleteLater); 这里暂时不用，设置在对话框关闭时退出线程
    calcBLCthread->start();

    if(clacBLCprogress->exec()==CalcBlcProgressDlg::Accepted){
        QString showXmlDoc;
        QTextStream xmlOutStream(&showXmlDoc, QIODevice::WriteOnly);
        xmlDoc->save(xmlOutStream, 4);
        blcDataEdit->setPlainText(showXmlDoc);
        xmlHL = new BlcXmlHighlight(blcDataEdit->document());

        showOnSreenSeries.dataProxy()->resetArray(aeGain_surfaceData_4p_map.first().surfaceDateArr_r);//暂时显示第一个r
        threeDSurface->addSeries(&showOnSreenSeries);
        threeDSurface->show();
    }
    else{
        blcDataEdit->setPlainText(tr("计算出错，请检查环境配置、raw文件和输入信息是否匹配"));
        threeDSurface->hide();
    }
}

BLCDialog::~BLCDialog()
{
    if(xmlHL!=NULL)
        delete xmlHL;
    if(showImgBuf!=NULL)
        delete[] showImgBuf;
    if(!(xmlDoc->isNull())){
        delete xmlDoc;
    }
    if(aeGain_surfaceData_4p_map.size()>0){
         QMap<quint16, SurfaceDateArrP_4>::iterator it=aeGain_surfaceData_4p_map.begin();
         while(it!=aeGain_surfaceData_4p_map.end()){
             SurfaceDateArrP_4 tmp = it.value();
             QSurfaceDataArray* arr_ptr = tmp.surfaceDateArr_r;
             for(QSurfaceDataArray::iterator arr_it=arr_ptr->begin(); arr_it!=arr_ptr->end(); arr_it++){
                 delete *arr_it; //释放dataarray(这是一个list,包含所有的row指针)
             }
             delete tmp.surfaceDateArr_r;
             arr_ptr = tmp.surfaceDateArr_gr;
             for(QSurfaceDataArray::iterator arr_it=arr_ptr->begin(); arr_it!=arr_ptr->end(); arr_it++){
                 delete *arr_it; //释放dataarray(这是一个list,包含所有的row指针)
             }
             delete tmp.surfaceDateArr_gr;
             arr_ptr = tmp.surfaceDateArr_gb;
             for(QSurfaceDataArray::iterator arr_it=arr_ptr->begin(); arr_it!=arr_ptr->end(); arr_it++){
                 delete *arr_it; //释放dataarray(这是一个list,包含所有的row指针)
             }
             delete tmp.surfaceDateArr_gb;
             arr_ptr = tmp.surfaceDateArr_b;
             for(QSurfaceDataArray::iterator arr_it=arr_ptr->begin(); arr_it!=arr_ptr->end(); arr_it++){
                 delete *arr_it; //释放dataarray(这是一个list,包含所有的row指针)
             }
             delete tmp.surfaceDateArr_b;//释放list本身

             it++;
         }
    }
}

void BLCDialog::onTreeItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if(item->parent()==NULL)
        return;
    else{
        QString clicked_real_file;
        QString rootText  = item->parent()->text(column);
        qint32 iso = rootText.right(rootText.size()-3).toInt();//ISO100 排除掉ISO三个字符，拿到整数值
        QStringList specificISO_fn = blc_fn_map[iso];
        for(QStringList::iterator it = specificISO_fn.begin(); it!=specificISO_fn.end(); it++){
            QFileInfo finfo(*it);
            QString fn = finfo.fileName();
            if(item->text(column)==fn){
                clicked_real_file = *it;
                break;
            }
        }
        if(clicked_real_file.isEmpty())//如果没有找到匹配的raw文件，不做处理
            return;
        showRawFile(clicked_real_file);//更改raw显示
        Q_ASSERT(iso>0);
        quint16 ae_gain = quint16(iso/50);
        QString regexp = QString("<ae_gain>\\s*%1\\s*</ae_gain>").arg(ae_gain);
        QRegularExpression express(regexp);
        QTextCursor txtCsr = blcDataEdit->document()->find(express);//从开头开始搜索
        if(!txtCsr.isNull())
            blcDataEdit->setTextCursor(txtCsr);

        //int curpos = blcDataEdit->verticalScrollBar()->value();
        //blcDataEdit->verticalScrollBar()->setValue(curpos+blcDataEdit->verticalScrollBar()->pageStep());

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

void BLCDialog::onThreadDestroyed(QObject *obj)
{
    Q_UNUSED(obj);
    calcBLCthread->quit();
    calcBLCthread->wait();
    showOnSreenSeries.dataProxy()->resetArray(NULL);
    threeDSurface->removeSeries(&showOnSreenSeries);
}

void BLCDialog::onSaveXmlButton()
{
    if(xmlFn.isEmpty()){
        xmlFn = QFileDialog::getSaveFileName(this, tr("保存为..."), preWorkPath, "XML file(*.xml);;All file(*.*)");
        if(xmlFn.isEmpty())
            return;
        QFileInfo fn_info(xmlFn);
        preWorkPath = fn_info.absoluteFilePath();
        QFile xmlF(xmlFn);
        if(!xmlF.open(QFile::WriteOnly|QFile::Text|QFile::Truncate)){
            QMessageBox::critical(this, tr("error"), tr("打开文件失败..."), QMessageBox::Ok);
            return;
        }
        QTextStream out(&xmlF);
        QString context = blcDataEdit->document()->toPlainText();
        out<<context;
        xmlF.close();
        blcDataChanged = false;
    }
    else{
        QFileInfo info(xmlFn);
        if(info.exists() && blcDataChanged==false)//如果文件已经存在并且编辑内容没有改变，则直接返回
            return;
        else{
            QFile xmlF(xmlFn);
            if(!xmlF.open(QFile::WriteOnly|QFile::Text|QFile::Truncate)){
                QMessageBox::critical(this, tr("error"), tr("打开文件失败..."), QMessageBox::Ok);
                return;
            }
            QTextStream out(&xmlF);
            QString context = blcDataEdit->document()->toPlainText();
            QString show = context.left(100);
            xmlF.close();
            blcDataChanged = false;
        }
    }
}

void BLCDialog::onSaveAsButton()
{
    QString saveAsName = QFileDialog::getSaveFileName(this, tr("保存为..."), preWorkPath, "XML file(*.xml);;All file(*.*)");
    if(saveAsName.isEmpty())
        return;
    QFileInfo fn_info(saveAsName);
    preWorkPath = fn_info.absoluteFilePath();
    QFile xmlF(saveAsName);
    if(!xmlF.open(QFile::WriteOnly|QFile::Text|QFile::Truncate)){
        QMessageBox::critical(this, tr("error"), tr("打开文件失败..."), QMessageBox::Ok);
        return;
    }
    QTextStream out(&xmlF);
    QString context = blcDataEdit->document()->toPlainText();
    out<<context;
    xmlF.close();
}

void BLCDialog::onContextChanged()
{
    blcDataChanged = true;
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
