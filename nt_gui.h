#ifndef NT_GUI_H
#define NT_GUI_H

#include <QMainWindow>
#include "client.h"

QT_BEGIN_NAMESPACE
namespace Ui { class NT_GUI; }
QT_END_NAMESPACE

class NT_GUI : public QMainWindow
{
    Q_OBJECT

public:
    NT_GUI(QWidget *parent = nullptr);
    ~NT_GUI();
    void addNewTorrent(QString hash, TorrentState &ts);

public slots:
    void on_createNewTorrent_clicked();
    void on_addNewTorrent_clicked();

private:
    Ui::NT_GUI *ui;
    QHash<QString, TorrentState> torrents;
};
#endif // NT_GUI_H
