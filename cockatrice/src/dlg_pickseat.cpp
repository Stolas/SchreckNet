#include "dlg_pickseat.h"

#include "main.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

DlgPickSeat::DlgPickSeat(AbstractClient *_client, QWidget *parent) : QDialog(parent), client(_client)
{
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    // mainLayout->addWidget(dirView);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    setWindowTitle(tr("Pick Seat"));
    setMinimumWidth(sizeHint().width());
    resize(400, 600);

}

int DlgPickSeat::getPickedSeat() const
{
    return 0;
}
