#include "inc\aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->label->setOpenExternalLinks(true);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_pushButton_clicked()
{
    //this->close();
    accept();
}
