#ifndef ADDTORRENTDIALOG_H
#define ADDTORRENTDIALOG_H

#include <QDialog>

namespace Ui {
class AddTorrentDialog;
}

class AddTorrentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddTorrentDialog(QWidget *parent = nullptr);
    ~AddTorrentDialog();
    QString getTorrentFilePath();
    QString getOutputPath();

private slots:
    void on_browseButton_clicked();
    void on_browseButton2_clicked();

private:
    Ui::AddTorrentDialog *ui;
};

#endif // ADDTORRENTDIALOG_H
