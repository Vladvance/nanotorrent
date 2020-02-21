#include "newtorrentdialog.h"
#include "ui_newtorrentdialog.h"

#include <QFileDialog>

NewTorrentDialog::NewTorrentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewTorrentDialog)
{
    ui->setupUi(this);
    ui->label_2->setEnabled(false);
    ui->pathEdit->setEnabled(false);
    ui->browseButton2->setEnabled(false);
}

NewTorrentDialog::~NewTorrentDialog()
{
    delete ui;
}

QString NewTorrentDialog::getFile() const
{
    return ui->filePathEdit->text();
}

QString NewTorrentDialog::getOutputPath() const
{
    return ui->pathEdit->text();
}

void NewTorrentDialog::on_browseButton_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Open a file", "$HOME");
    ui->filePathEdit->setText(file);
}

void NewTorrentDialog::on_browseButton2_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choose a directory to save .torrent file", "$HOME");
    ui->pathEdit->setText(dir);
}

void NewTorrentDialog::on_torrentFileCheckbox_stateChanged(int state)
{
    if(state == Qt::Checked)
    {
        ui->label_2->setEnabled(true);
        ui->pathEdit->setEnabled(true);
        ui->browseButton2->setEnabled(true);
    } else {
        ui->label_2->setEnabled(false);
        ui->pathEdit->setEnabled(false);
        ui->browseButton2->setEnabled(false);
    }
}
