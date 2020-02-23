#include "addtorrentdialog.h"
#include "ui_addtorrentdialog.h"

#include <qfiledialog.h>

AddTorrentDialog::AddTorrentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddTorrentDialog)
{
    ui->setupUi(this);
}

AddTorrentDialog::~AddTorrentDialog()
{
    delete ui;
}

QString AddTorrentDialog::getTorrentFilePath()
{
    return ui->torrentFileEdit->text();
}

QString AddTorrentDialog::getOutputPath()
{
    return ui->outputEdit->text();

}

void AddTorrentDialog::on_browseButton_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open .torrent file"), "$HOME", tr("Torrent file (*.torrent)"));
    ui->torrentFileEdit->setText(file);
}

void AddTorrentDialog::on_browseButton2_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a directory where to download files"), "$HOME");
    ui->outputEdit->setText(dir);
}
