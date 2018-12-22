#include "inc\rawinfodialog.h"
#include "ui_rawinfodialog.h"
#include <QIntValidator>

rawinfoDialog::rawinfoDialog(QWidget *parent) :
    QDialog(parent),
    //inputFlag(CANCEL),
    bayer(NO_BAYER),
    rawSize(0,0),
    bitDepth(0),
    ui(new Ui::rawinfoDialog)
{
    ui->setupUi(this);
    setLayout(ui->gridLayout);
    ui->groupBox->setLayout(ui->bayerGridLayout);
    //setAttribute(Qt::WA_DeleteOnClose);
    ui->RG->setChecked(true); //default RG
    QIntValidator* rawinputValid = new QIntValidator(0, 10000, this);
    ui->widthInput->setValidator(rawinputValid);
    ui->heightInput->setValidator(rawinputValid);
}

rawinfoDialog::~rawinfoDialog()
{
    delete ui;
}

void rawinfoDialog::on_OkBtn_clicked()
{
    //inputFlag = OK;
    if(ui->RG->isChecked())
        bayer = RG;
    else if(ui->GR->isChecked())
        bayer = GR;
    else if(ui->GB->isChecked())
        bayer = GB;
    else if(ui->BG->isChecked())
        bayer = BG;
    rawSize = QSize(ui->widthInput->text().toInt(), ui->heightInput->text().toInt());
    bitDepth = ui->bitDepthSpbox->value();
    //close();
    accept();
}

void rawinfoDialog::on_CancelBtn_clicked()
{
    //inputFlag = CANCEL;
    //close();
    reject();
}
