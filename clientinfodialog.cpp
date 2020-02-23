#include "clientinfodialog.h"
#include "ui_clientinfodialog.h"

ClientInfoDialog::ClientInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ClientInfoDialog)
{
    ui->setupUi(this);
}

ClientInfoDialog::~ClientInfoDialog()
{
    delete ui;
}

void ClientInfoDialog::setPeerId(QString peerid)
{
    ui->peerid_val->setText(peerid);
}

void ClientInfoDialog::setIP(QString ip)
{
    ui->ip_val->setText(ip);
}

void ClientInfoDialog::setPort(QString port)
{
    ui->port_val->setText(port);
}
