#include "inc\rightshiftdialog.h"
#include "ui_rightshiftdialog.h"

RightShiftDialog::RightShiftDialog(QWidget *parent, quint8 right_shift_bit) :
    QDialog(parent),
    rst(right_shift_bit),
    ui(new Ui::RightShiftDialog)
{
    ui->setupUi(this);
    //ui->verticalLayout->addLayout(ui->horizontalLayout);
    setLayout(ui->verticalLayout);
    ui->label->setText(tr("最小右移bits=%1,如需更改,请点击右侧选择并点击OK").arg(rst));
    ui->spinBox->setRange(rst, 11);// 16bit >> 11 = 32, max right shift
    ui->spinBox->setValue(rst);
}

RightShiftDialog::~RightShiftDialog()
{
    delete ui;
}

void RightShiftDialog::on_buttonOK_accepted()
{
    accept();
    rst = (quint8)ui->spinBox->value();
}
