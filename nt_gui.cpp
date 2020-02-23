#include "clientinfodialog.h"
#include "newtorrentdialog.h"
#include "nt_gui.h"
#include "ui_nt_gui.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPointer>
#include <addtorrentdialog.h>

NT_GUI::NT_GUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NT_GUI)
{
    ui->setupUi(this);
    ui->torrentView->setModel(&torrentmodel);
    torrentmodel.setTorrents(app.torrents);
    app.activeTorrentFlags = 0;
}

NT_GUI::~NT_GUI()
{
    delete ui;
    for(int i = 0; i < 64; i++) {
        if (app.activeTorrentFlags & 1ULL<<(63-i)) {
            freeTorrentState(&(app.torrents[i]));
        }
    }
}

void NT_GUI::on_createNewTorrent_clicked()
{
    NewTorrentDialog dialog;
    dialog.setModal(true);
    if(dialog.exec() == QDialog::Accepted) {
        QString file = dialog.getFile();
        QString outpath = dialog.getOutputPath();
        int idx = torrentmodel.insertTorrent(&app, file);

        if(!outpath.isEmpty()) {
            saveToTorrentFile(&(app.torrents[idx].mi), outpath.toLocal8Bit().constData());
        }
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

void NT_GUI::on_showClientInfo_clicked()
{
    static QPointer <ClientInfoDialog> dialog;
    if(!dialog) {
        dialog = new ClientInfoDialog();
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }
    dialog->setPeerId("TestPeerId");
    dialog->setIP("TestIP");
    dialog->setPort("TestPort");
    dialog->show();
}

void NT_GUI::runCore()
{
    runApp(&app);
}
