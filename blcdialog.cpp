#if _MSC_VER > 1600
#pragma execution_character_set("utf-8")  //fuck MSVC complior, use UTF-8, not gb2312/gbk
#endif

#include "inc/blcdialog.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFileInfo>
#include <QPainter>
#include <QPaintEvent>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QtDataVisualization/QValue3DAxis>
#include <QDebug>

const int SELECT_R = 1;
const int SELECT_GR = 2;
const int SELECT_GB = 3;
const int SELECT_B = 4;

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
    showOnSreenSeries(new QSurface3DSeries),
    showOnScreenDataArr(new QSurfaceDataArray)
{
    resize(1024, 600);
    setWindowTitle(tr("BLC tool"));
    if(!threeDSurface->hasContext()){
        QMessageBox::critical(this, tr("error"), tr("init opengl fail..."), QMessageBox::Ok);
    }
    threeDSurface->setAxisX(new QValue3DAxis);
    threeDSurface->setAxisY(new QValue3DAxis);
    threeDSurface->setAxisZ(new QValue3DAxis);

    blcDataEdit->setWordWrapMode(QTextOption::NoWrap);
    blcDataEdit->setCenterOnScroll(true);
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
            if(it==blc_fn_map.begin() && str_it==rawList.begin()){
                child->setSelected(true);//默认选中第一个raw
                iso = (quint16)(it.key());
            }
            else
                child->setSelected(false);
        }
        treeWgt->insertTopLevelItem(root_id, item);
    }
    treeWgt->expandAll();
    treeWgt->setFocusPolicy(Qt::ClickFocus);

    setLeftUI();
    setMiddleUI();
    yMinSpinBox->setBro(yMaxSpinBox);
    yMaxSpinBox->setBro(yMinSpinBox);
    setRightUI();
    setLayout(hlayout);
    threeDSurface->setSelectionMode(QAbstract3DGraph::SelectionNone);

    QDomProcessingInstruction insturction = xmlDoc->createProcessingInstruction("xml", "version='1.0' encoding='utf-8'");
    xmlDoc->appendChild(insturction);
    docRoot = xmlDoc->createElement("module_calibration");
    xmlDoc->appendChild(docRoot);

    qint32 curSeletISO = blc_fn_map.firstKey();
    QString curRawFn = blc_fn_map[curSeletISO].at(0);

    grid11_11->setChecked(true);//default 11x11
    grid5_5->setEnabled(false); //close 5x5
    showRawFile(curRawFn);

    connect(treeWgt, &QTreeWidget::itemDoubleClicked, this, &BLCDialog::onTreeItemDoubleClicked);
    connect(noGrid, &QRadioButton::toggled, this, &BLCDialog::onNoGridToggled);
    connect(grid5_5, &QRadioButton::toggled, this, &BLCDialog::onGrid5_5_Toggled);
    connect(grid11_11, &QRadioButton::toggled, this, &BLCDialog::onGrid11_11_Toggled);
    connect(surface_r, &QRadioButton::toggled, this, &BLCDialog::onSurface_r_Toggled);//切换3d 显示bayer channel
    connect(surface_gr, &QRadioButton::toggled, this, &BLCDialog::onSurface_gr_Toggled);//切换3d 显示bayer channel
    connect(surface_gb, &QRadioButton::toggled, this, &BLCDialog::onSurface_gb_Toggled);//切换3d 显示bayer channel
    connect(surface_b, &QRadioButton::toggled, this, &BLCDialog::onSurface_b_Toggled);//切换3d 显示bayer channel
    connect(selection_no, &QRadioButton::toggled, this, &BLCDialog::onSelectionNoItem);
    connect(selection_item, &QRadioButton::toggled, this, &BLCDialog::onSelectionSingelItem);
    connect(selection_row, &QRadioButton::toggled, this, &BLCDialog::onSelectionRow);
    connect(selection_col, &QRadioButton::toggled, this, &BLCDialog::onSelectionColumn);
    connect(themeList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BLCDialog::onThemeListIdxChanged);
    connect(yMinSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &BLCDialog::onYminValueChanged);
    connect(yMaxSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &BLCDialog::onYmaxValueChanged);
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
    //connect(calcBLCthread, &calcBlcThread::finished, calcBLCthread, &QObject::deleteLater); //这里暂时不用，设置在对话框关闭时退出线程
    calcBLCthread->start();

    if(clacBLCprogress->exec()==CalcBlcProgressDlg::Accepted){
        QString showXmlDoc;
        QTextStream xmlOutStream(&showXmlDoc, QIODevice::WriteOnly);
        xmlDoc->save(xmlOutStream, 4);
        blcDataEdit->setPlainText(showXmlDoc);
        xmlHL = new BlcXmlHighlight(blcDataEdit->document());//渲染xml高亮

        //QSurfaceDataArray showOnScreenDataArr;
        if(deepCopyDataArray(showOnScreenDataArr, aeGain_surfaceData_4p_map, this->iso, SELECT_R)){
            showOnSreenSeries.dataProxy()->resetArray(showOnScreenDataArr);
            showOnSreenSeries.setDrawMode(QSurface3DSeries::DrawSurface);
            threeDSurface->addSeries(&showOnSreenSeries);
            threeDSurface->axisZ()->setReversed(true);//row颠倒
            yMaxSpinBox->setValue(threeDSurface->axisY()->max()-1);
            yMinSpinBox->setValue(threeDSurface->axisY()->min()+1);
            threeDSurface->show();
        }
    }
    else{
        blcDataEdit->setPlainText(tr("计算出错，请检查环境配置、raw文件和输入信息是否匹配"));
        threeDSurface->hide();
        yMaxSpinBox->setValue(0);
        yMinSpinBox->setValue(1);
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
    // showOnSreenSeries自身析构，series->dataproxy析构dataarray,不用主动释放data
    /*if(showOnScreenDataArr!=NULL){
        while(showOnScreenDataArr->size()!=0){
            delete showOnScreenDataArr->takeFirst();
        }
        delete showOnScreenDataArr;
        showOnScreenDataArr = NULL;
    }*/
    if(aeGain_surfaceData_4p_map.size()>0){
         QMap<quint16, SurfaceDateArrP_4>::iterator it=aeGain_surfaceData_4p_map.begin();
         while(it!=aeGain_surfaceData_4p_map.end()){
             SurfaceDateArrP_4 tmp = it.value();
             QSurfaceDataArray* arr_ptr = tmp.surfaceDateArr_r;
             while(arr_ptr->size()!=0){
                 delete arr_ptr->takeFirst();
             }
             delete tmp.surfaceDateArr_r;
             arr_ptr = tmp.surfaceDateArr_gr;
             while(arr_ptr->size()!=0){
                 delete arr_ptr->takeFirst();
             }
             delete tmp.surfaceDateArr_gr;
             arr_ptr = tmp.surfaceDateArr_gb;
             while(arr_ptr->size()!=0){
                 delete arr_ptr->takeFirst();
             }
             delete tmp.surfaceDateArr_gb;
             arr_ptr = tmp.surfaceDateArr_b;
             while(arr_ptr->size()!=0){
                 delete arr_ptr->takeFirst();
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
        this->iso = iso;
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

        if(threeDSurface->isVisible()){
            int bayerSelected;
            if(surface_r->isChecked()){
                bayerSelected = SELECT_R;
            }
            else if(surface_gr->isChecked()){
                bayerSelected = SELECT_GR;
            }
            else if(surface_gb->isChecked()){
                bayerSelected = SELECT_GB;
            }
            else if(surface_b->isChecked()){
                bayerSelected = SELECT_B;
            }
            else
                bayerSelected = -1;
            if(deepCopyDataArray(showOnScreenDataArr, aeGain_surfaceData_4p_map, this->iso, bayerSelected)){
                threeDSurface->seriesList().at(0)->dataProxy()->resetArray(showOnScreenDataArr);
                threeDSurface->axisY()->setAutoAdjustRange(true);
                yMaxSpinBox->setValue(threeDSurface->axisY()->max());
                yMinSpinBox->setValue(threeDSurface->axisY()->min());
            }
        }
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


/* ---------------------------------------------------------------------
 *  dst作为目标指针，根据iso和r/gr/gb/b选项决定拷贝数据
 *  调用这个函数需要保证dst指向一个QSrufaceDataArray对象，不可为null
 *
 *  如果dst本身已存在data数据，则被覆盖，如果无数据或者row不够，则从堆上new
 *  Q3DSurface负责最后的销毁
 *--------------------------------------------------------------------*/
bool BLCDialog::deepCopyDataArray(QSurfaceDataArray *dst, const QMap<quint16, SurfaceDateArrP_4> &src_map, quint16 iso, int selectBayer)
{
    /*QList<QTreeWidgetItem*> curSeletedList = treeWgt->selectedItems();
    if(curSeletedList.size()!=1)
        return false;


    QTreeWidgetItem* parentIsoItem = curSeletedList.at(0)->parent();
    if(parentIsoItem==NULL)
        return false;
    QString iso_str = parentIsoItem->text(0).right(parentIsoItem->text(0).length()-3);
    bool flag = false;
    qint32 iso = iso_str.toInt(&flag, 10);
    if(flag == false)
        return false;*/
    if(selectBayer<0 || selectBayer >4)
        return false;
    if(dst==NULL || src_map.size()==0)
        return false;
    if(!src_map.contains(iso))
        return false;

    QSurfaceDataArray* pData;
    switch(selectBayer){
        case(SELECT_R):
            pData = src_map[iso].surfaceDateArr_r;
            break;
        case(SELECT_GR):
            pData = src_map[iso].surfaceDateArr_gr;
            break;
        case(SELECT_GB):
            pData = src_map[iso].surfaceDateArr_gb;
            break;
        case(SELECT_B):
            pData = src_map[iso].surfaceDateArr_b;
            break;
        default:
            pData = NULL;
            break;
    }
    if(pData==NULL)
        return false;


    if(dst->size()==pData->size()){//已存在row相等，则deep copy
        for(QSurfaceDataArray::iterator it_dst=dst->begin(), it_src=pData->begin(); it_src!=pData->end(); it_dst++, it_src++){
            *(*it_dst) = *(*it_src); //QVector<3dpoint> 复制，不是指针复制
        }
    }
    else if(dst->size()<pData->size()){
        QSurfaceDataArray::iterator it_dst=dst->begin();
        QSurfaceDataArray::iterator it_src=pData->begin();
        for(; it_dst!=dst->end(); it_dst++, it_src++){
            *(*it_dst) = *(*it_src); //QVector<3dpoint> 复制，不是指针复制
        }
        while(it_src!=pData->end()){
            QSurfaceDataRow* tmpRow = new QSurfaceDataRow;
            *tmpRow = *(*it_src);
            *dst << tmpRow;
            it_src++;
        }
    }
    else{
        while(dst->size()>pData->size()){
            QSurfaceDataRow* lastRow = dst->last();
            if(lastRow!=NULL)
                delete lastRow;
            dst->removeLast();
        }
        Q_ASSERT(dst->size()==pData->size());
        for(QSurfaceDataArray::iterator it_dst=dst->begin(), it_src=pData->begin(); it_src!=pData->end(); it_dst++, it_src++){
            *(*it_dst) = *(*it_src); //QVector<3dpoint> 复制，不是指针复制
        }
    }
    return true;
}

void BLCDialog::setLeftUI()
{
    QVBoxLayout* leftVerLayout = new QVBoxLayout;
    leftVerLayout->addWidget(useTip, 1, Qt::AlignHCenter|Qt::AlignVCenter);
    leftVerLayout->addWidget(treeWgt, 1);
    QVBoxLayout* boxLayout = new QVBoxLayout;
    boxLayout->setContentsMargins(7,7,7,7);
    boxLayout->addWidget(noGrid, 1, Qt::AlignVCenter|Qt::AlignLeft);
    boxLayout->addWidget(grid5_5, 1, Qt::AlignVCenter|Qt::AlignLeft);
    boxLayout->addWidget(grid11_11, 1, Qt::AlignVCenter|Qt::AlignLeft);
    grid_box->setLayout(boxLayout);
    leftVerLayout->addWidget(grid_box, 0, Qt::AlignHCenter|Qt::AlignVCenter);

    hlayout->addLayout(leftVerLayout);
}

void BLCDialog::setMiddleUI()
{
    QVBoxLayout* middleLayout = new QVBoxLayout;
    QHBoxLayout* surfaceAreaLayout = new QHBoxLayout;//设置3d surface整块区域水平layout
    QVBoxLayout* surfaceAreaRightLayout = new QVBoxLayout;
//----------------------------------------------------
    QGridLayout* bayerGridLayout = new QGridLayout;//设置3d surface bayer选项radiobutton排列
    QGroupBox* surfaceBox = new QGroupBox(tr("选择bayer"));
    surface_r = new QRadioButton(tr("R"));
    surface_gr = new QRadioButton(tr("Gr"));
    surface_gb = new QRadioButton(tr("Gb"));
    surface_b = new QRadioButton(tr("B"));
    bayerGridLayout->addWidget(surface_r, 0, 0, 1, 1);
    bayerGridLayout->addWidget(surface_gr, 0, 1, 1, 1);
    bayerGridLayout->addWidget(surface_gb, 1, 0, 1, 1);
    bayerGridLayout->addWidget(surface_b, 1, 1, 1, 1);
    surfaceBox->setLayout(bayerGridLayout);
    surface_r->setChecked(true);
    surface_gr->setChecked(false);
    surface_gb->setChecked(false);
    surface_b->setChecked(false);

    QGridLayout* itemSeletionMode = new QGridLayout;
    QGroupBox* seletionBox = new QGroupBox(tr("选定数据点显示"));
    selection_no = new QRadioButton(tr("不显示"));
    selection_item = new QRadioButton(tr("显示单个点"));
    selection_row = new QRadioButton(tr("显示同一行"));
    selection_col = new QRadioButton(tr("显示同一列"));
    itemSeletionMode->addWidget(selection_no, 0, 0, 1, 1);
    itemSeletionMode->addWidget(selection_item, 0, 1, 1, 1);
    itemSeletionMode->addWidget(selection_row, 1, 0, 1, 1);
    itemSeletionMode->addWidget(selection_col, 1, 1, 1, 1);
    selection_no->setChecked(true);
    seletionBox->setLayout(itemSeletionMode);

    QHBoxLayout* surfaceThemelayout = new QHBoxLayout;
    QLabel* themeLabel = new QLabel(tr("显示主题:"));
    surfaceThemelayout->addWidget(themeLabel);
    themeList = new QComboBox;
    themeList->addItem(QStringLiteral("Qt"));
    themeList->addItem(QStringLiteral("Primary Colors"));
    themeList->addItem(QStringLiteral("Digia"));
    themeList->addItem(QStringLiteral("Stone Moss"));
    themeList->addItem(QStringLiteral("Army Blue"));
    themeList->addItem(QStringLiteral("Retro"));
    themeList->addItem(QStringLiteral("Ebony"));
    themeList->addItem(QStringLiteral("Isabelle"));
    themeList->setCurrentIndex(0);
    surfaceThemelayout->addWidget(themeList);

    QGridLayout* yAxisSettingLayout = new QGridLayout;
    QLabel* yMin = new QLabel(tr("Y轴下限"));
    QLabel* yMax = new QLabel(tr("Y轴上限"));
    yMinSpinBox = new BlcSpinBox(nullptr, true);
    yMaxSpinBox = new BlcSpinBox(nullptr, false);
    yAxisSettingLayout->addWidget(yMin, 0, 0, 1, 1);
    yAxisSettingLayout->addWidget(yMax, 0, 1, 1, 1);
    yAxisSettingLayout->addWidget(yMinSpinBox, 1, 0, 1, 1);
    yAxisSettingLayout->addWidget(yMaxSpinBox, 1, 1, 1, 1);
    yMinSpinBox->setRange(-16384, 16383);
    yMaxSpinBox->setRange(-16384, 16383);
//----------------------------------------------------------------
    surfaceAreaRightLayout->addWidget(surfaceBox);
    surfaceAreaRightLayout->addWidget(seletionBox);
    surfaceAreaRightLayout->addLayout(surfaceThemelayout);
    surfaceAreaRightLayout->addLayout(yAxisSettingLayout);

    surfaceAreaLayout->addWidget(surfaceContainerWgt, 1);
    surfaceAreaLayout->addLayout(surfaceAreaRightLayout);

    imgView->setFrameStyle(QFrame::Box);
    imgView->setScaledContents(true);

    middleLayout->addWidget(imgView, 1);
    middleLayout->addLayout(surfaceAreaLayout, 1);
    hlayout->addLayout(middleLayout, 1); //设置中间包括img显示部分，和3d suface显示部分的layout
}

void BLCDialog::setRightUI()
{
    QVBoxLayout* rightVerLayout = new QVBoxLayout;
    rightVerLayout->addWidget(blcDataEdit, 1);
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(saveXml, 1);
    buttonLayout->addWidget(saveAsXml, 1);
    buttonLayout->addStretch(2);
    rightVerLayout->addLayout(buttonLayout);
    hlayout->addLayout(rightVerLayout, 1);
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

void BLCDialog::onSurface_r_Toggled(bool statu)
{
    if(statu){
        if(deepCopyDataArray(showOnScreenDataArr, aeGain_surfaceData_4p_map, this->iso, SELECT_R)){//内部判断哪个bayer channel被选中
            threeDSurface->seriesList().at(0)->dataProxy()->resetArray(showOnScreenDataArr);
            threeDSurface->axisY()->setAutoAdjustRange(true);
            yMaxSpinBox->setValue(threeDSurface->axisY()->max());
            yMinSpinBox->setValue(threeDSurface->axisY()->min());
        }
    }
}

void BLCDialog::onSurface_gr_Toggled(bool statu)
{
    if(statu){
        if(deepCopyDataArray(showOnScreenDataArr, aeGain_surfaceData_4p_map, this->iso, SELECT_GR)){//内部判断哪个bayer channel被选中
            threeDSurface->seriesList().at(0)->dataProxy()->resetArray(showOnScreenDataArr);
            yMaxSpinBox->setValue(threeDSurface->axisY()->max());
            yMinSpinBox->setValue(threeDSurface->axisY()->min());
        }
    }
}

void BLCDialog::onSurface_gb_Toggled(bool statu)
{
    if(statu){
        if(deepCopyDataArray(showOnScreenDataArr, aeGain_surfaceData_4p_map, this->iso, SELECT_GB)){//内部判断哪个bayer channel被选中
            threeDSurface->seriesList().at(0)->dataProxy()->resetArray(showOnScreenDataArr);
            yMaxSpinBox->setValue(threeDSurface->axisY()->max());
            yMinSpinBox->setValue(threeDSurface->axisY()->min());
        }
    }
}

void BLCDialog::onSurface_b_Toggled(bool statu)
{
    if(statu){
        if(deepCopyDataArray(showOnScreenDataArr, aeGain_surfaceData_4p_map, this->iso, SELECT_B)){//内部判断哪个bayer channel被选中
            threeDSurface->seriesList().at(0)->dataProxy()->resetArray(showOnScreenDataArr);
            yMaxSpinBox->setValue(threeDSurface->axisY()->max());
            yMinSpinBox->setValue(threeDSurface->axisY()->min());
        }
    }
}

void BLCDialog::onSelectionNoItem(bool statu)
{
    if(statu)
        threeDSurface->setSelectionMode(QAbstract3DGraph::SelectionNone);
}

void BLCDialog::onSelectionSingelItem(bool statu)
{
    if(statu)
        threeDSurface->setSelectionMode(QAbstract3DGraph::SelectionItem);
}

void BLCDialog::onSelectionRow(bool statu)
{
    if(statu)
        threeDSurface->setSelectionMode(QAbstract3DGraph::SelectionItemAndRow|QAbstract3DGraph::SelectionSlice);
}

void BLCDialog::onSelectionColumn(bool statu)
{
    if(statu)
        threeDSurface->setSelectionMode(QAbstract3DGraph::SelectionItemAndColumn|QAbstract3DGraph::SelectionSlice);
}

void BLCDialog::onThemeListIdxChanged(int idx)
{
    threeDSurface->activeTheme()->setType(Q3DTheme::Theme(idx));
}

void BLCDialog::onYminValueChanged(int val)
{
    threeDSurface->axisY()->setRange(val, yMaxSpinBox->value());
}

void BLCDialog::onYmaxValueChanged(int val)
{
    threeDSurface->axisY()->setRange(yMinSpinBox->value(), val);
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

//------------------class spinbox----------------
BlcSpinBox::BlcSpinBox(QWidget *parent, bool isMin):
    QSpinBox(parent),
    bro(NULL)
{
    this->isMin = isMin;
    lineEdit()->setReadOnly(true);
}

void BlcSpinBox::stepBy(int steps)//reimplement stepBy, 用于在setValue之前做判断
{
    int min = this->minimum();
    int max = this->maximum();
    int old = this->value();
    if((old+steps)>max || (old+steps)<min)
        return;//do nothing
    if(this->isMin){
        if(bro==NULL)
            return;
        int end = this->bro->value();
        if((end-old-steps)<1)
            return;
    }
    else{
        if(bro==NULL)
            return;
        int begin = this->bro->value();
        if((old+steps-begin)<1)
            return;
    }
    QAbstractSpinBox::stepBy(steps);
}
