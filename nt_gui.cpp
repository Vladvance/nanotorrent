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
    ui->torrentView->setModel(&peermodel);
    app.activeTorrentFlags = 0;
}

NT_GUI::~NT_GUI()
{
    delete ui;
}

void NT_GUI::on_createNewTorrent_clicked()
{
    NewTorrentDialog dialog;
    dialog.setModal(true);
    if(dialog.exec() == QDialog::Accepted) {
        QString file = dialog.getFile();
        QString outpath = dialog.getOutputPath();

        int idx = getEmptyTorrentSlot(&(this->app));
        app.activeTorrentFlags |= 1ULL<<(63 - idx);

        struct Metainfo *mi = &(app.torrents[idx].mi);
        generateMetainfo(file.toLocal8Bit().constData(), mi);
        app.torrents[idx].status = SEEDING;
        initTorrentState(&(app.torrents[idx]), file.toLocal8Bit().constData());

        if(!outpath.isEmpty()) {
            saveToTorrentFile(mi, outpath.toLocal8Bit().constData());
        }
//        peermodel.insertRows(row, 1, this);
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
        int idx = getEmptyTorrentSlot(&(this->app));
        readFromTorrentFile(file.toLocal8Bit().constData(), &(app.torrents[idx].mi));
    }
}

void NT_GUI::runCore()
{
    runApp(&app);
}
