#ifndef ALSATUNERGUI_H
#define ALSATUNERGUI_H

#include <QMainWindow>
#include "alsaapi.h"

namespace Ui {
class AlsaTunerGUI;
}

class AlsaTunerGUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit AlsaTunerGUI(QWidget *parent = 0);
    ~AlsaTunerGUI();

private:
    Ui::AlsaTunerGUI *ui;
    QList<AlsaCard> cards;

private slots:
    void selectedCardChanged(int newIndex);
};

#endif // ALSATUNERGUI_H
