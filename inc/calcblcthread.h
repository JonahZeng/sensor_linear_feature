#ifndef INCCALCBLCTHREAD_H
#define INCCALCBLCTHREAD_H

#include <QThread>
#include <QMap>
#include <QStringList>
#include <QDomElement>
#include <QDomDocument>
#include "inc/rawinfodialog.h"

class calcBlcThread : public QThread
{
    Q_OBJECT
public:
    calcBlcThread(QObject *parent, QMap<qint32, QStringList>& iso_fn, rawinfoDialog::bayerMode bm, QSize rawsz, quint16 bd, QDomDocument* doc, QDomElement& root);
protected:
    void run();
signals:
    void currentTaskId(quint16 id);
    void pyInitFail();
private:
    QMap<qint32, QStringList> blc_fn_map;
    rawinfoDialog::bayerMode bayerMode;
    QSize rawSize;
    quint16 bitDepth;
    quint16 taskID;
    QDomDocument* xmlDoc;
    QDomElement docRoot;
private:
    void createBlcDateNode(quint8 order,
                           quint16 aeGain,
                           quint16 R_blc_be,
                           quint16 Gr_blc_be,
                           quint16 Gb_blc_be,
                           quint16 B_blc_be,
                           QVector<quint16>& R_grid_val,
                           QVector<quint16>& Gr_grid_val,
                           QVector<quint16>& Gb_grid_val,
                           QVector<quint16>& B_grid_val);

    void addRaw2FourChannel(quint8* raw_buf, qreal* r_ch, qreal* gr_ch, qreal* gb_ch, qreal* b_ch);
    void avgBayerChannel(qreal* channel, qint32 frameNum);
    quint16 avgBlcValueBE(qreal* savgol_result, qint64 len);
    void calGridValue(qreal* savgol_result, qint64 row, qint64 col, QVector<quint16>& grid11_11);
};

#endif // INCCALCBLCTHREAD_H
