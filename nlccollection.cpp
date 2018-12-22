#include "inc\nlccollection.h"
//#include <QDebug>
#include <QFile>
#include <QFileInfo>

nlccollection::nlccollection():
    rawName(),
    width(0),
    height(0),
    bm(rawinfoDialog::NO_BAYER),
    bitDepth(0)
{
    avg_b = avg_gb = avg_gr = avg_b = 0;
    shut = 0.0;
    iso = 0;
    roi = {0,0,0,0};
}

nlccollection::nlccollection(QString rn, quint16 w, quint16 h, rawinfoDialog::bayerMode bm, quint16 bd):
    rawName(rn),
    width(w),
    height(h),
    bm(bm),
    bitDepth(bd)
{
    roi = {0,0,0,0};
    avg_b = avg_gb = avg_gr = avg_b = 0;
    QFileInfo info(rn);
    Q_ASSERT(info.exists() || info.isFile());
    QString fn = info.fileName();
    int idx1 = fn.indexOf("_EI_");
    QChar shut_char[7] = {'\0'};
    for(int m = 0; m < 6; m++)
        shut_char[m] = fn[idx1+4+m];
    shut_char[6] = '\0';

    QString time(shut_char);
    if(time.toInt()!=0)
        shut = 1000.0/time.toInt();
    else
        shut = 0.0;
    QStringList sp_list = fn.split('_', QString::SkipEmptyParts);
    int idx = sp_list.indexOf("ISO");
    if(idx==-1)
        iso = 0;
    else
        iso = sp_list[idx+1].toInt();
}

nlccollection::nlccollection(const nlccollection &nl)
{
    avg_b = nl.avg_b;
    avg_gb = nl.avg_gb;
    avg_gr = nl.avg_gr;
    avg_r = nl.avg_r;
    bitDepth = nl.bitDepth;
    bm = nl.bm;
    height = nl.height;
    width = nl.width;
    rawName = nl.rawName;
    roi = nl.roi;
    shut = nl.shut;
    iso = nl.iso;
}

nlccollection& nlccollection::operator=(const nlccollection &nl)
{
    if(this == &nl)
        return *this;
    avg_b = nl.avg_b;
    avg_gb = nl.avg_gb;
    avg_gr = nl.avg_gr;
    avg_r = nl.avg_r;
    bitDepth = nl.bitDepth;
    bm = nl.bm;
    height = nl.height;
    width = nl.width;
    rawName = nl.rawName;
    roi = nl.roi;
    shut = nl.shut;
    iso = nl.iso;
    return *this;
}

bool nlccollection::operator<(const nlccollection &nl) const
{
    if(shut<nl.shut)
        return false;
    else
        return true;
}

void nlccollection::calRoiAvgValue()
{
    QFileInfo rawInfo(rawName);
    qint64 rawFsize = rawInfo.size();

    quint8* buffer = NULL;
    quint32 bufferSize = 0;
    quint8 rawByte = 0;

    if(bitDepth<=8 && bitDepth>0){
        rawByte = 1;
        bufferSize = width*height;
    }
    else if(bitDepth>8 && bitDepth<=16){
        rawByte = 2;
        bufferSize = width*height*rawByte;
    }
    else{
        //qDebug()<<"not supported bit depth"<<endl;
        avg_b = avg_gb = avg_gr = avg_r = 0;
        return;
    }

    if(rawFsize != bufferSize){
        //qDebug()<<"raw size mismatch"<<endl;
        avg_b = avg_gb = avg_gr = avg_r = 0;
        return;
    }

    QFile rawF(rawName);
    if(!rawF.open(QIODevice::ReadOnly)){
        //qDebug()<<"open raw fail"<<endl;
        avg_b = avg_gb = avg_gr = avg_r = 0;
        return;
    }

    buffer = new quint8[bufferSize];
    if(buffer==NULL){
        //qDebug()<<"malloc fail"<<endl;
        avg_b = avg_gb = avg_gr = avg_r = 0;
        return;
    }

    if(rawF.read((char*)buffer, bufferSize) == -1){
        delete[] buffer;
        buffer = NULL;
        rawF.close();
        //qDebug()<<"read raw fail"<<endl;
        avg_b = avg_gb = avg_gr = avg_r = 0;
        return;
    }

    quint64 r_sum=0, gr_sum=0, gb_sum=0, b_sum=0;
    quint32 r_cnt=0, gr_cnt=0, gb_cnt=0, b_cnt=0;
    for(quint32 row = roi.top; row <= roi.bottom; row++)
    {
        for(quint32 col = roi.left; col <= roi.right; col++)
        {
            switch (getBayerCh(col, row))
            {
            case R_IDX:
                r_cnt++;
                r_sum += (rawByte==1)?((quint8*)buffer)[row*width+col]:((quint16*)buffer)[row*width+col];
                break;
            case GR_IDX:
                gr_cnt++;
                gr_sum += (rawByte==1)?((quint8*)buffer)[row*width+col]:((quint16*)buffer)[row*width+col];
                break;
            case GB_IDX:
                gb_cnt++;
                gb_sum += (rawByte==1)?((quint8*)buffer)[row*width+col]:((quint16*)buffer)[row*width+col];
                break;
            case B_IDX:
                b_cnt++;
                b_sum += (rawByte==1)?((quint8*)buffer)[row*width+col]:((quint16*)buffer)[row*width+col];
                break;
            default:
                break;
            }
        }
    }
    Q_ASSERT(bitDepth<=14);
    avg_b = ((double)(b_sum<<(14-bitDepth)))/b_cnt;
    avg_gb = ((double)(gb_sum<<(14-bitDepth)))/gb_cnt;
    avg_gr = ((double)(gr_sum<<(14-bitDepth)))/gr_cnt;
    avg_r = ((double)(r_sum<<(14-bitDepth)))/r_cnt;

    delete[] buffer;
    buffer = NULL;
}

bool nlccollection::checkParameters()
{
    if(rawName.count() == 0 || bitDepth == 0 || bm==rawinfoDialog::NO_BAYER || width <= 0 || height <= 0)
        return false;
    if(roi.right>=width || roi.bottom>= height || roi.bottom <= roi.top || roi.right <= roi.left)
        return false;
    return true;
}

quint8 nlccollection::getBayerCh(quint16 x, quint16 y) const
{
    if(bm==rawinfoDialog::RG){
        if((x&0x1)==0 && (y&0x1)==0)
            return R_IDX;
        else if((x&0x1)==1 && (y&0x1)==0)
            return GR_IDX;
        else if((x&0x1)==0 && (y&0x1)==1)
            return GB_IDX;
        else if((x&0x1)==1 && (y&0x1)==1)
            return B_IDX;
        else
            return -1;
    }
    else if(bm==rawinfoDialog::GR){
        if((x&0x1)==0 && (y&0x1)==0)
            return GR_IDX;
        else if((x&0x1)==1 && (y&0x1)==0)
            return R_IDX;
        else if((x&0x1)==0 && (y&0x1)==1)
            return B_IDX;
        else if((x&0x1)==1 && (y&0x1)==1)
            return GB_IDX;
        else
            return -1;
    }
    else if(bm==rawinfoDialog::GB){
        if((x&0x1)==0 && (y&0x1)==0)
            return GB_IDX;
        else if((x&0x1)==1 && (y&0x1)==0)
            return B_IDX;
        else if((x&0x1)==0 && (y&0x1)==1)
            return R_IDX;
        else if((x&0x1)==1 && (y&0x1)==1)
            return GR_IDX;
        else
            return -1;
    }
    else if(bm==rawinfoDialog::BG){
        if((x&0x1)==0 && (y&0x1)==0)
            return B_IDX;
        else if((x&0x1)==1 && (y&0x1)==0)
            return GB_IDX;
        else if((x&0x1)==0 && (y&0x1)==1)
            return GR_IDX;
        else if((x&0x1)==1 && (y&0x1)==1)
            return R_IDX;
        else
            return -1;
    }
    else{
        return -1;
    }
}

/*QDebug operator<<(QDebug d, const nlccollection &nl)
{
    d<<"raw file name:"<<nl.rawName<<endl;
    d<<"raw size(w*h):"<<nl.width<<"*"<<nl.height<<endl;
    d<<"raw bit depth:"<<nl.bitDepth<<endl;
    d<<"shut:"<<nl.shut<<" ISO:"<<nl.iso<<endl;
    d<<"selected ROI(top left, bottom right):"<<nl.roi.top<<" "<<nl.roi.left<<","<<nl.roi.bottom<<" "<<nl.roi.right<<endl;
    d<<"avg r, gr, gb, b:"<<nl.avg_r<<","<<nl.avg_gr<<","<<nl.avg_gb<<","<<nl.avg_b<<endl;
    return d;
}*/
