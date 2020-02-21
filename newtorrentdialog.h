#ifndef NEWTORRENTDIALOG_H
#define NEWTORRENTDIALOG_H

#include <QDialog>

namespace Ui {
class NewTorrentDialog;
}

class NewTorrentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewTorrentDialog(QWidget *parent = nullptr);
    ~NewTorrentDialog();
    QString getFile() const;
    QString getOutputPath() const;

private slots:
    void on_browseButton_clicked();
    void on_browseButton2_clicked();
    void on_torrentFileCheckbox_stateChanged(int arg1);

private:
    Ui::NewTorrentDialog *ui;
};

#endif // NEWTORRENTDIALOG_H
