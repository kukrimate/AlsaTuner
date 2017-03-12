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
    // Control change slots
    void sliderValueChanged(int newValue);
    void flagStateChanged(bool newState);
    void selectorSelectionChanged(int newIndex);
};

#include <QSlider>
#include <QComboBox>
#include <QCheckBox>

/* Custom classes for selectors */
class SelectorSlider : public QSlider
{
    Q_OBJECT
public:
    SelectorSlider();

    int cardIndex;
    int controlIndex;
    int valueIndex;
};

class SelectorComboBox : public QComboBox
{
    Q_OBJECT
public:
    SelectorComboBox();

    int cardIndex;
    int controlIndex;
};

class SelectorCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    SelectorCheckBox();

    int cardIndex;
    int controlIndex;
};

#endif // ALSATUNERGUI_H
