#ifndef CLIENTINFODIALOG_H
#define CLIENTINFODIALOG_H

#include <QDialog>

namespace Ui {
class ClientInfoDialog;
}

class ClientInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClientInfoDialog(QWidget *parent = nullptr);
    ~ClientInfoDialog();
    void setPeerId(QString peerid);
    void setPort(QString port);
    void setIP(QString ip);

private:
    Ui::ClientInfoDialog *ui;
};

#endif // CLIENTINFODIALOG_H
