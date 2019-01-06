#include "inc/calcblcthread.h"
#include "numpy/arrayobject.h"
#include <QFile>

calcBlcThread::calcBlcThread(QObject *parent, QMap<qint32, QStringList>& iso_fn, rawinfoDialog::bayerMode bm,
                             QSize rawsz, quint16 bd, QDomDocument* doc, QDomElement& root, quint16 ts, QMap<quint16, SurfaceDateArrP_4>* const p_surface_data_map)
    : QThread(parent),
     blc_fn_map(iso_fn),
     bayerMode(bm),
     bitDepth(bd),
     rawSize(rawsz),
     taskID(0),
     xmlDoc(doc),
     docRoot(root),
     maxTask(ts),
     pSurfaceDataMap(p_surface_data_map)
{
    Q_ASSERT(pSurfaceDataMap!=NULL && pSurfaceDataMap->size()==0);//必须保证传进来的map事先已经分配并且没有内容
    if(_import_array()< 0){ //WIN7+VS2017+PYTHON3.5.4 测试可以
        emit pyInitFail();
    }
}


void calcBlcThread::run()
{
    //-----一次性计算过程，如果多次，import 模块内容应放在构造函数里面，并保持PyObject*指针，并在析构函数中Py_DECREF
    //if(_import_array()< 0){//WIN10+VS2015+PYTHON3.6.6 测试可以
    //    emit pyInitFail();
    //    return;
    //}
#define BLC_TOTAL_DATA 10

    PyObject* pNdimageModule = PyImport_Import(PyUnicode_FromString("scipy.ndimage"));
    PyObject* pUniformFunc = PyObject_GetAttrString(pNdimageModule, "uniform_filter");
    PyObject* pZoomFunc = PyObject_GetAttrString(pNdimageModule, "zoom");
    PyObject* pSignalModule = PyImport_Import(PyUnicode_FromString("scipy.signal"));
    PyObject* pFuncSavgol = PyObject_GetAttrString(pSignalModule, "savgol_filter");
    if(pNdimageModule==NULL || pUniformFunc==NULL || pZoomFunc==NULL || pSignalModule==NULL || pFuncSavgol==NULL){
        Py_XDECREF(pSignalModule);
        Py_XDECREF(pZoomFunc);
        Py_XDECREF(pUniformFunc);
        Py_XDECREF(pNdimageModule);
        Py_XDECREF(pFuncSavgol);
        emit pyInitFail();
        return;
    }
    //----------------------end----------------------------


    qreal* bayer_r_buf = new qreal[rawSize.width()*rawSize.height()/4];
    qreal* bayer_gr_buf = new qreal[rawSize.width()*rawSize.height()/4];
    qreal* bayer_gb_buf = new qreal[rawSize.width()*rawSize.height()/4];
    qreal* bayer_b_buf = new qreal[rawSize.width()*rawSize.height()/4];
    memset((void*)bayer_r_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
    memset((void*)bayer_gr_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
    memset((void*)bayer_gb_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
    memset((void*)bayer_b_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);

    quint8* raw_buf;
    if(bitDepth>8)
        raw_buf = new quint8[rawSize.width()*rawSize.height()*2];
    else
        raw_buf = new quint8[rawSize.width()*rawSize.height()];

    npy_intp shape[2] = {rawSize.height()/2, rawSize.width()/2};
    int i = 0, dataIdx=0;
    QVector<quint16> stored_ae_gain;
    quint16 ae_gain;
    quint16 r_blc_be, gr_blc_be, gb_blc_be, b_blc_be;
    QVector<quint16> r_grid(121), gr_grid(121), gb_grid(121), b_grid(121);
    for(QMap<qint32, QStringList>::iterator it=blc_fn_map.begin(); it!=blc_fn_map.end(); it++){
        ae_gain = it.key()/50;
        for(QStringList::Iterator str=it.value().begin(); str!=it.value().end(); str++){
            QFile raw_f(*str);
            raw_f.open(QFile::ReadOnly);
            raw_f.read((char*)raw_buf, bitDepth>8?(2*rawSize.width()*rawSize.height()):rawSize.width()*rawSize.height());
            raw_f.close();
            addRaw2FourChannel(raw_buf, bayer_r_buf, bayer_gr_buf, bayer_gb_buf, bayer_b_buf);
            memset((void*)raw_buf, 0, bitDepth>8?(2*rawSize.width()*rawSize.height()):rawSize.width()*rawSize.height());
            i++;
            if(i<maxTask)
                emit currentTaskId(i);
        }
        avgBayerChannel(bayer_r_buf, it.value().size());
        avgBayerChannel(bayer_gr_buf, it.value().size());
        avgBayerChannel(bayer_gb_buf, it.value().size());
        avgBayerChannel(bayer_b_buf, it.value().size());
        PyArrayObject* r = (PyArrayObject*)PyArray_SimpleNewFromData(2, shape, NPY_FLOAT64, (void*)bayer_r_buf);
        PyArrayObject* gr = (PyArrayObject*)PyArray_SimpleNewFromData(2, shape, NPY_FLOAT64, (void*)bayer_gr_buf);
        PyArrayObject* gb = (PyArrayObject*)PyArray_SimpleNewFromData(2, shape, NPY_FLOAT64, (void*)bayer_gb_buf);
        PyArrayObject* b = (PyArrayObject*)PyArray_SimpleNewFromData(2, shape, NPY_FLOAT64, (void*)bayer_b_buf);

        PyArrayObject* p_blur_R = (PyArrayObject*)PyObject_CallFunction(pUniformFunc, "(Oi)", (PyObject*)r, 11);
        PyArrayObject* p_blur_Gr = (PyArrayObject*)PyObject_CallFunction(pUniformFunc, "(Oi)", (PyObject*)gr, 11);
        PyArrayObject* p_blur_Gb = (PyArrayObject*)PyObject_CallFunction(pUniformFunc, "(Oi)", (PyObject*)gb, 11);
        PyArrayObject* p_blur_B = (PyArrayObject*)PyObject_CallFunction(pUniformFunc, "(Oi)", (PyObject*)b, 11);
        //因为是手动创建buf然后传给ndarray，所以r 没有own_data_flag，DECREF(r)不会释放data内存
        Py_DECREF(r);
        Py_DECREF(gr);
        Py_DECREF(gb);
        Py_DECREF(b);
        PyArrayObject* zoom_out_R = (PyArrayObject*)PyObject_CallFunction(pZoomFunc, "(OfOis)", (PyObject*)p_blur_R, 0.0625, Py_None, 3, "nearest");
        PyArrayObject* zoom_out_Gr = (PyArrayObject*)PyObject_CallFunction(pZoomFunc, "(OfOis)", (PyObject*)p_blur_Gr, 0.0625, Py_None, 3, "nearest");
        PyArrayObject* zoom_out_Gb = (PyArrayObject*)PyObject_CallFunction(pZoomFunc, "(OfOis)", (PyObject*)p_blur_Gb, 0.0625, Py_None, 3, "nearest");
        PyArrayObject* zoom_out_B = (PyArrayObject*)PyObject_CallFunction(pZoomFunc, "(OfOis)", (PyObject*)p_blur_B, 0.0625, Py_None, 3, "nearest");

        Py_DECREF(p_blur_R);//own_data_flag=1, 不需要手动释放data
        Py_DECREF(p_blur_Gr);
        Py_DECREF(p_blur_Gb);
        Py_DECREF(p_blur_B);

        npy_int32 winLen_order1 = zoom_out_R->dimensions[1]/8;
        winLen_order1 = (((npy_uint32)winLen_order1>>1)<<1)+1;
        npy_int32 winLen_order0 = zoom_out_R->dimensions[0]/8;
        winLen_order0 = (((npy_uint32)winLen_order0>>1)<<1)+1;

        PyArrayObject* savgol_r_order1 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)zoom_out_R, winLen_order1, 2, 0, 1.0, 1);
        Py_DECREF(zoom_out_R);
        PyArrayObject* savgol_r_order0 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)savgol_r_order1, winLen_order0, 2, 0, 1.0, 0);

        PyArrayObject* savgol_gr_order1 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)zoom_out_Gr, winLen_order1, 2, 0, 1.0, 1);
        Py_DECREF(zoom_out_Gr);
        PyArrayObject* savgol_gr_order0 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)savgol_gr_order1, winLen_order0, 2, 0, 1.0, 0);

        PyArrayObject* savgol_gb_order1 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)zoom_out_Gb, winLen_order1, 2, 0, 1.0, 1);
        Py_DECREF(zoom_out_Gb);
        PyArrayObject* savgol_gb_order0 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)savgol_gb_order1, winLen_order0, 2, 0, 1.0, 0);

        PyArrayObject* savgol_b_order1 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)zoom_out_B, winLen_order1, 2, 0, 1.0, 1);
        Py_DECREF(zoom_out_B);
        PyArrayObject* savgol_b_order0 = (PyArrayObject*)PyObject_CallFunction(pFuncSavgol, "(Oiiifi)", (PyObject*)savgol_b_order1, winLen_order0, 2, 0, 1.0, 0);

        Q_ASSERT(savgol_r_order0->nd==2 && savgol_gr_order0->nd==2 && savgol_gb_order0->nd==2 && savgol_b_order0->nd==2);
        r_blc_be  = avgBlcValueBE((qreal*)(savgol_r_order0->data),  savgol_r_order0->dimensions[0]*savgol_r_order0->dimensions[1]);
        gr_blc_be = avgBlcValueBE((qreal*)(savgol_gr_order0->data), savgol_gr_order0->dimensions[0]*savgol_gr_order0->dimensions[1]);
        gb_blc_be = avgBlcValueBE((qreal*)(savgol_gb_order0->data), savgol_gb_order0->dimensions[0]*savgol_gb_order0->dimensions[1]);
        b_blc_be  = avgBlcValueBE((qreal*)(savgol_b_order0->data),  savgol_b_order0->dimensions[0]*savgol_b_order0->dimensions[1]);
        calGridValue((qreal*)(savgol_r_order0->data), savgol_r_order0->dimensions[0], savgol_r_order0->dimensions[1], r_grid);
        calGridValue((qreal*)(savgol_gr_order0->data), savgol_gr_order0->dimensions[0], savgol_gr_order0->dimensions[1], gr_grid);
        calGridValue((qreal*)(savgol_gb_order0->data), savgol_gb_order0->dimensions[0], savgol_gb_order0->dimensions[1], gb_grid);
        calGridValue((qreal*)(savgol_b_order0->data), savgol_b_order0->dimensions[0], savgol_b_order0->dimensions[1], b_grid);

        if(!stored_ae_gain.contains(ae_gain) && dataIdx<BLC_TOTAL_DATA){
            createBlcDateNode(dataIdx, ae_gain, r_blc_be, gr_blc_be, gb_blc_be, b_blc_be, r_grid, gr_grid, gb_grid, b_grid);

            SurfaceDateArrP_4 surface_data_4_p;
            surface_data_4_p.surfaceDateArr_r = new QSurfaceDataArray;//先申请4个 data array，然后去填充；依次为r, gr, gb, b
            surface_data_4_p.surfaceDateArr_gr = new QSurfaceDataArray;
            surface_data_4_p.surfaceDateArr_gb = new QSurfaceDataArray;
            surface_data_4_p.surfaceDateArr_b = new QSurfaceDataArray;
            for(quint32 row=0; row<savgol_r_order0->dimensions[0]; row++){
                QSurfaceDataRow* row_p_r = new QSurfaceDataRow(savgol_r_order0->dimensions[1]);//申请column个3d point vector
                for(quint32 col=0; col<savgol_r_order0->dimensions[1]; col++){
                    (*row_p_r)[col].setPosition(QVector3D(col, ((qreal*)(savgol_r_order0->data))[row*savgol_r_order0->dimensions[1]+col], row));
                }
                surface_data_4_p.surfaceDateArr_r->append(row_p_r);
            }
            for(quint32 row=0; row<savgol_gr_order0->dimensions[0]; row++){
                QSurfaceDataRow* row_p_gr = new QSurfaceDataRow(savgol_gr_order0->dimensions[1]);//申请column个3d point vector
                for(quint32 col=0; col<savgol_gr_order0->dimensions[1]; col++){
                    (*row_p_gr)[col].setPosition(QVector3D(col, ((qreal*)(savgol_gr_order0->data))[row*savgol_gr_order0->dimensions[1]+col], row));
                }
                surface_data_4_p.surfaceDateArr_gr->append(row_p_gr);
            }
            for(quint32 row=0; row<savgol_gb_order0->dimensions[0]; row++){
                QSurfaceDataRow* row_p_gb = new QSurfaceDataRow(savgol_gb_order0->dimensions[1]);//申请column个3d point vector
                for(quint32 col=0; col<savgol_gb_order0->dimensions[1]; col++){
                    (*row_p_gb)[col].setPosition(QVector3D(col, ((qreal*)(savgol_gb_order0->data))[row*savgol_gb_order0->dimensions[1]+col], row));
                }
                surface_data_4_p.surfaceDateArr_gb->append(row_p_gb);
            }
            for(quint32 row=0; row<savgol_b_order0->dimensions[0]; row++){
                QSurfaceDataRow* row_p_b = new QSurfaceDataRow(savgol_b_order0->dimensions[1]);//申请column个3d point vector
                for(quint32 col=0; col<savgol_b_order0->dimensions[1]; col++){
                    (*row_p_b)[col].setPosition(QVector3D(col, ((qreal*)(savgol_b_order0->data))[row*savgol_b_order0->dimensions[1]+col], row));
                }
                surface_data_4_p.surfaceDateArr_b->append(row_p_b);
            }
            if(!pSurfaceDataMap->contains(ae_gain))
                (*pSurfaceDataMap)[ae_gain] = surface_data_4_p;

            stored_ae_gain.append(ae_gain);
            dataIdx++;
        }

        Py_DECREF(savgol_r_order1);
        Py_DECREF(savgol_r_order0);
        Py_DECREF(savgol_gr_order1);
        Py_DECREF(savgol_gr_order0);
        Py_DECREF(savgol_gb_order1);
        Py_DECREF(savgol_gb_order0);
        Py_DECREF(savgol_b_order1);
        Py_DECREF(savgol_b_order0);

        memset((void*)bayer_r_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
        memset((void*)bayer_gr_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
        memset((void*)bayer_gb_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
        memset((void*)bayer_b_buf, 0, sizeof(qreal)*rawSize.width()*rawSize.height()/4);
    }

    Py_XDECREF(pSignalModule);
    Py_XDECREF(pZoomFunc);
    Py_XDECREF(pUniformFunc);
    Py_XDECREF(pNdimageModule);
    Py_XDECREF(pFuncSavgol);
    delete[] raw_buf;
    delete[] bayer_b_buf;
    delete[] bayer_gb_buf;
    delete[] bayer_gr_buf;
    delete[] bayer_r_buf;

    if(dataIdx<BLC_TOTAL_DATA){
        ae_gain=ae_gain+20;
        for(int k=dataIdx; k<BLC_TOTAL_DATA; k++, ae_gain+=20){
            createBlcDateNode(k, ae_gain, r_blc_be, gr_blc_be, gb_blc_be, b_blc_be, r_grid, gr_grid, gb_grid, b_grid);
        }
    }
    emit currentTaskId(maxTask);
}

void calcBlcThread::addRaw2FourChannel(quint8 *raw_buf, qreal *r_ch, qreal *gr_ch, qreal *gb_ch, qreal *b_ch)
{
    qint32 width = rawSize.width();
    qint32 height = rawSize.height();
    switch(bayerMode){
    case rawinfoDialog::RG:
        for(qint32 row=0; row<height; row+=2){
            for(qint32 col=0; col<width; col+=2){
                r_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*width+col]:raw_buf[row*width+col];
                gr_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*width+col+1]:raw_buf[row*width+col+1];
                gb_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*(width+1)+col]:raw_buf[row*(width+1)+col];
                b_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*(width+1)+col+1]:raw_buf[row*(width+1)+col+1];
            }
        }
        break;
    case rawinfoDialog::GR:
        for(qint32 row=0; row<height; row+=2){
            for(qint32 col=0; col<width; col+=2){
                gr_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*width+col]:raw_buf[row*width+col];
                r_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*width+col+1]:raw_buf[row*width+col+1];
                b_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*(width+1)+col]:raw_buf[row*(width+1)+col];
                gb_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*(width+1)+col+1]:raw_buf[row*(width+1)+col+1];
            }
        }
        break;
    case rawinfoDialog::GB:
        for(qint32 row=0; row<height; row+=2){
            for(qint32 col=0; col<width; col+=2){
                gb_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*width+col]:raw_buf[row*width+col];
                b_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*width+col+1]:raw_buf[row*width+col+1];
                r_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*(width+1)+col]:raw_buf[row*(width+1)+col];
                gr_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*(width+1)+col+1]:raw_buf[row*(width+1)+col+1];
            }
        }
        break;
    case rawinfoDialog::BG:
        for(qint32 row=0; row<height; row+=2){
            for(qint32 col=0; col<width; col+=2){
                b_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*width+col]:raw_buf[row*width+col];
                gb_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*width+col+1]:raw_buf[row*width+col+1];
                gr_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*(width+1)+col]:raw_buf[row*(width+1)+col];
                r_ch[row/2*width/2+col/2] += bitDepth>8?((quint16*)raw_buf)[row*(width+1)+col+1]:raw_buf[row*(width+1)+col+1];
            }
        }
        break;
    default:
        break;
    }
}

void calcBlcThread::avgBayerChannel(qreal *channel, qint32 frameNum)
{
    Q_ASSERT(frameNum!=0);
    qint32 size = rawSize.width()*rawSize.height()/4;
    for(qint32 k=0; k<size; k++){
        channel[k] = channel[k]/frameNum;
    }
}

quint16 calcBlcThread::avgBlcValueBE(qreal *savgol_result, qint64 len)
{
    qreal sum=0.0;
    for(qint64 idx=0; idx<len; idx++){
        sum += savgol_result[idx];
    }
    Q_ASSERT(sum>0 && len>0);
    return quint16((sum/len)+0.5);
}

void calcBlcThread::calGridValue(qreal *savgol_result, qint64 total_row, qint64 total_col, QVector<quint16> &grid11_11)
{
    Q_ASSERT(grid11_11.size()==121);
    //qint64 h_step = row/10;
    for(quint8 j=0; j<11; j++){
        for(quint8 k=0; k<11; k++){
            qint64 row = j==10?(j*total_row/10)-1:(j*total_row/10);
            qint64 col = k==10?(k*total_col/10)-1:(k*total_col/10);
            grid11_11[j*11+k] = savgol_result[row*total_col+col];
        }
    }
}

void calcBlcThread::createBlcDateNode(quint8 order,
                                  quint16 aeGain,
                                  quint16 R_blc_be,
                                  quint16 Gr_blc_be,
                                  quint16 Gb_blc_be,
                                  quint16 B_blc_be,
                                  QVector<quint16>& R_grid_val,
                                  QVector<quint16>& Gr_grid_val,
                                  QVector<quint16>& Gb_grid_val,
                                  QVector<quint16>& B_grid_val)
{
    Q_ASSERT(R_grid_val.size()==121 && Gr_grid_val.size()==121 && Gb_grid_val.size()==121 && B_grid_val.size()==121);
    if(!(xmlDoc->isNull()) && !(docRoot.isNull())){
        QDomElement data = xmlDoc->createElement("data"+QString::number(order, 10));

        QDomElement ae_gain = xmlDoc->createElement("ae_gain");
        QDomText ae_gain_val = xmlDoc->createTextNode(QString::number(aeGain));
        ae_gain.appendChild(ae_gain_val);
        data.appendChild(ae_gain);

        QDomElement r_offset = xmlDoc->createElement("blc_r_offset_be");
        QDomText r_offset_data = xmlDoc->createTextNode(QString::number(R_blc_be, 10));
        r_offset.appendChild(r_offset_data);
        data.appendChild(r_offset);
        QDomElement gr_offset = xmlDoc->createElement("blc_gr_offset_be");
        QDomText gr_offset_data = xmlDoc->createTextNode(QString::number(Gr_blc_be, 10));
        gr_offset.appendChild(gr_offset_data);
        data.appendChild(gr_offset);
        QDomElement gb_offset = xmlDoc->createElement("blc_gb_offset_be");
        QDomText gb_offset_data = xmlDoc->createTextNode(QString::number(Gb_blc_be, 10));
        gb_offset.appendChild(gb_offset_data);
        data.appendChild(gb_offset);
        QDomElement b_offset = xmlDoc->createElement("blc_b_offset_be");
        QDomText b_offset_data = xmlDoc->createTextNode(QString::number(B_blc_be, 10));
        b_offset.appendChild(b_offset_data);
        data.appendChild(b_offset);

        QString data11x11("\n");
        for(quint8 row=0; row<11; row++){
            char grid_str[11*6+8+2];
            sprintf(grid_str, row==10?"        %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d\n":"        %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,\n",
                    R_grid_val[11*row], R_grid_val[11*row+1], R_grid_val[11*row+2], R_grid_val[11*row+3], R_grid_val[11*row+4],
                    R_grid_val[11*row+5], R_grid_val[11*row+6], R_grid_val[11*row+7], R_grid_val[11*row+8], R_grid_val[11*row+9], R_grid_val[11*row+10]);
            grid_str[11*6+9]='\0';
            data11x11.append(grid_str);
        }
        data11x11.append("        ");
        QDomElement r_grid = xmlDoc->createElement("blc_grid_r_val");
        QDomText r_grid_data = xmlDoc->createTextNode(data11x11);
        r_grid.appendChild(r_grid_data);
        data.appendChild(r_grid);

        data11x11.clear();
        data11x11.append("\n");
        for(quint8 row=0; row<11; row++){
            char grid_str[11*6+8+2];
            sprintf(grid_str, row==10?"        %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d\n":"        %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,\n",
                    Gr_grid_val[11*row], Gr_grid_val[11*row+1], Gr_grid_val[11*row+2], Gr_grid_val[11*row+3], Gr_grid_val[11*row+4],
                    Gr_grid_val[11*row+5], Gr_grid_val[11*row+6], Gr_grid_val[11*row+7], Gr_grid_val[11*row+8], Gr_grid_val[11*row+9], Gr_grid_val[11*row+10]);
            grid_str[11*6+9]='\0';
            data11x11.append(grid_str);
        }
        data11x11.append("        ");
        QDomElement gr_grid = xmlDoc->createElement("blc_grid_gr_val");
        QDomText gr_grid_data = xmlDoc->createTextNode(data11x11);
        gr_grid.appendChild(gr_grid_data);
        data.appendChild(gr_grid);

        data11x11.clear();
        data11x11.append("\n");
        for(quint8 row=0; row<11; row++){
            char grid_str[11*6+8+2];
            sprintf(grid_str, row==10?"        %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d\n":"        %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,\n",
                    Gb_grid_val[11*row], Gb_grid_val[11*row+1], Gb_grid_val[11*row+2], Gb_grid_val[11*row+3], Gb_grid_val[11*row+4],
                    Gb_grid_val[11*row+5], Gb_grid_val[11*row+6], Gb_grid_val[11*row+7], Gb_grid_val[11*row+8], Gb_grid_val[11*row+9], Gb_grid_val[11*row+10]);
            grid_str[11*6+9]='\0';
            data11x11.append(grid_str);
        }
        data11x11.append("        ");
        QDomElement gb_grid = xmlDoc->createElement("blc_grid_gb_val");
        QDomText gb_grid_data = xmlDoc->createTextNode(data11x11);
        gb_grid.appendChild(gb_grid_data);
        data.appendChild(gb_grid);

        data11x11.clear();
        data11x11.append("\n");
        for(quint8 row=0; row<11; row++){
            char grid_str[11*6+8+2];
            sprintf(grid_str, row==10?"        %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d\n":"        %5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,%5d,\n",
                    B_grid_val[11*row], B_grid_val[11*row+1], B_grid_val[11*row+2], B_grid_val[11*row+3], B_grid_val[11*row+4],
                    B_grid_val[11*row+5], B_grid_val[11*row+6], B_grid_val[11*row+7], B_grid_val[11*row+8], B_grid_val[11*row+9], B_grid_val[11*row+10]);
            grid_str[11*6+9]='\0';
            data11x11.append(grid_str);
        }
        data11x11.append("        ");
        QDomElement b_grid = xmlDoc->createElement("blc_grid_b_val");
        QDomText b_grid_data = xmlDoc->createTextNode(data11x11);
        b_grid.appendChild(b_grid_data);
        data.appendChild(b_grid);
        QDomElement nlc_lut_r = xmlDoc->createElement("nlc_correction_lut_r");
        QDomText nlc_lut_r_val = xmlDoc->createTextNode("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
        nlc_lut_r.appendChild(nlc_lut_r_val);
        data.appendChild(nlc_lut_r);
        QDomElement nlc_lut_gr = xmlDoc->createElement("nlc_correction_lut_gr");
        QDomText nlc_lut_gr_val = xmlDoc->createTextNode("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
        nlc_lut_gr.appendChild(nlc_lut_gr_val);
        data.appendChild(nlc_lut_gr);
        QDomElement nlc_lut_gb = xmlDoc->createElement("nlc_correction_lut_gb");
        QDomText nlc_lut_gb_val = xmlDoc->createTextNode("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
        nlc_lut_gb.appendChild(nlc_lut_gb_val);
        data.appendChild(nlc_lut_gb);
        QDomElement nlc_lut_b = xmlDoc->createElement("nlc_correction_lut_b");
        QDomText nlc_lut_b_val = xmlDoc->createTextNode("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
        nlc_lut_b.appendChild(nlc_lut_b_val);
        data.appendChild(nlc_lut_b);
        QDomElement nlc_cut_r = xmlDoc->createElement("nlc_cut_r");
        QDomText nlc_cut_r_val = xmlDoc->createTextNode("0");
        nlc_cut_r.appendChild(nlc_cut_r_val);
        data.appendChild(nlc_cut_r);
        QDomElement nlc_cut_gr = xmlDoc->createElement("nlc_cut_gr");
        QDomText nlc_cut_gr_val = xmlDoc->createTextNode("0");
        nlc_cut_gr.appendChild(nlc_cut_gr_val);
        data.appendChild(nlc_cut_gr);
        QDomElement nlc_cut_gb = xmlDoc->createElement("nlc_cut_gb");
        QDomText nlc_cut_gb_val = xmlDoc->createTextNode("0");
        nlc_cut_gb.appendChild(nlc_cut_gb_val);
        data.appendChild(nlc_cut_gb);
        QDomElement nlc_cut_b = xmlDoc->createElement("nlc_cut_b");
        QDomText nlc_cut_b_val = xmlDoc->createTextNode("0");
        nlc_cut_b.appendChild(nlc_cut_b_val);
        data.appendChild(nlc_cut_b);

        docRoot.appendChild(data);
    }
}
