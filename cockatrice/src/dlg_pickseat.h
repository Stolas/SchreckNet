#ifndef DLG_STARTGAME_H
#define DLG_STARTGAME_H

#include <QDialog>
#include <QWidget>

class AbstractClient;
class QPushButton;
class QDialogButtonBox;

class DlgPickSeat : public QDialog
{
    Q_OBJECT
private:
    AbstractClient *client;
    QDialogButtonBox *buttonBox;

public:
    DlgPickSeat(AbstractClient *_client, QWidget *parent = nullptr);
    int getPickedSeat() const;
};

#endif
