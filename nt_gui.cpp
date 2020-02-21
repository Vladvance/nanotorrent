#include "newtorrentdialog.h"
#include "nt_gui.h"
#include "ui_nt_gui.h"
#include <QFileDialog>
#include <QMessageBox>
#include <addtorrentdialog.h>

NT_GUI::NT_GUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NT_GUI)
{
    ui->setupUi(this);
}

NT_GUI::~NT_GUI()
{
    delete ui;
}

void NT_GUI::on_createNewTorrent_clicked()
{
    QString file = "";
    QString outpath = "";
    NewTorrentDialog dialog;
    dialog.setModal(true);
    if(dialog.exec() == QDialog::Accepted) {
        file = dialog.getFile();
        outpath = dialog.getOutputPath();
        struct Metainfo mi;
        generateMetainfo(file.toLocal8Bit().constData(), &mi);
        if(outpath != "") {
            saveToTorrentFile(&mi, outpath.toLocal8Bit().constData());
        }
        freeMetainfo(&mi);
    }
}

void NT_GUI::on_addNewTorrent_clicked()
{
    QString file = "";
    QString outpath = "";
    AddTorrentDialog dialog;
    dialog.setModal(true);
    if(dialog.exec() == QDialog::Accepted) {
        file = dialog.getTorrentFilePath();
        outpath = dialog.getOutputPath();
        struct Metainfo mi;
        readFromTorrentFile(file.toLocal8Bit().constData(), &mi);
        freeMetainfo(&mi);
    }
}

void NT_GUI::addNewTorrent(QString hash, struct TorrentState& ts) {
    torrents.insert(hash, ts);
}
