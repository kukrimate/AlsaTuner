#include "alsatunergui.h"
#include "ui_alsatunergui.h"

#include <QDebug>

#include <QMessageBox>
#include <QApplication>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QLabel>

SelectorSlider::SelectorSlider() : QSlider() {}
SelectorComboBox::SelectorComboBox() : QComboBox() {}
SelectorCheckBox::SelectorCheckBox() : QCheckBox() {}

AlsaTunerGUI::AlsaTunerGUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AlsaTunerGUI)
{
    ui->setupUi(this);

    cards = AlsaAPI::listSoundCards();
    for (int i = 0; i < cards.length(); ++i) {
        ui->cardSelector->addItem(cards[i].name);
    }
}

AlsaTunerGUI::~AlsaTunerGUI()
{
    delete ui;
}

static void clearLayout(QLayout *layout) {
    QLayoutItem *item;
    while((item = layout->takeAt(0))) {
        if (item->layout()) {
            clearLayout(item->layout());
            delete item->layout();
        }
        if (item->widget()) {
            delete item->widget();
        }
    }
}

void AlsaTunerGUI::selectedCardChanged(int newIndex)
{
    AlsaCard newCard = cards[newIndex];

    clearLayout(ui->controlAreaLayout);

    // Disconnect this signal to avoid updating the selected control when clearing this
    disconnect(ui->controlSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedControlChanged(int)));
    ui->controlSelector->clear();
    connect(ui->controlSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedControlChanged(int)));

    /* Add new controls */
    for (int i = 0; i < newCard.controls.length(); ++i) {
        ui->controlSelector->addItem(newCard.controls[i].label);
    }
}

void AlsaTunerGUI::selectedControlChanged(int newIndex)
{
    AlsaCard newCard = cards[ui->cardSelector->currentIndex()];
    clearLayout(ui->controlAreaLayout);

    SelectorSlider *slider;
    SelectorComboBox *selector;
    SelectorCheckBox *flag;

    switch (newCard.controls[newIndex].type) {
    case SLIDER:
        for (int j = 0; j < newCard.controls[newIndex].slider.values.length(); ++j) {
            slider = new SelectorSlider();

            // Store the ALSA metadata for later usage
            slider->cardIndex = ui->cardSelector->currentIndex();
            slider->controlIndex = newIndex;
            slider->valueIndex = j;

            slider->setMinimum(newCard.controls[newIndex].slider.min);
            slider->setMaximum(newCard.controls[newIndex].slider.max);

            slider->setValue(newCard.controls[newIndex].slider.values[j]);
            connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(sliderValueChanged(int)));
            ui->controlAreaLayout->addWidget(slider);
        }

        break;
    case SELECTOR:
        selector = new SelectorComboBox();

        // Store the ALSA metadata for later usagetring>
        selector->cardIndex = ui->cardSelector->currentIndex();
        selector->controlIndex = newIndex;

        selector->addItems(newCard.controls[newIndex].selector.options);
        selector->setCurrentIndex(newCard.controls[newIndex].selector.selectedIndex);
        connect(selector, SIGNAL(currentIndexChanged(int)), this, SLOT(selectorSelectionChanged(int)));
        ui->controlAreaLayout->addWidget(selector);

        break;
    case FLAG:
        flag = new SelectorCheckBox();

        // Store the ALSA metadata for later usage
        flag->cardIndex = ui->cardSelector->currentIndex();
        flag->controlIndex = newIndex;

        flag->setText(newCard.controls[newIndex].label);
        flag->setChecked(newCard.controls[newIndex].flag.isSelected);
        connect(flag, SIGNAL(toggled(bool)), this, SLOT(flagStateChanged(bool)));
        ui->controlAreaLayout->addWidget(flag);

        break;
    }
}

void AlsaTunerGUI::selectorSelectionChanged(int newIndex)
{
    SelectorComboBox *selector = (SelectorComboBox *) sender();
    cards[selector->cardIndex].controls[selector->controlIndex].selector.selectedIndex = newIndex;

    try {
        AlsaAPI::updateControl(cards[selector->cardIndex], cards[selector->cardIndex].controls[selector->controlIndex]);
    } catch (QString e) {
        QMessageBox::critical(this, "Error", e);
    }
}

void AlsaTunerGUI::flagStateChanged(bool newState)
{
    SelectorCheckBox *flag = (SelectorCheckBox *) sender();
    cards[flag->cardIndex].controls[flag->controlIndex].flag.isSelected = newState;

    try {
        AlsaAPI::updateControl(cards[flag->cardIndex], cards[flag->cardIndex].controls[flag->controlIndex]);
    } catch (QString e) {
        QMessageBox::critical(this, "Error", e);
    }
}

void AlsaTunerGUI::sliderValueChanged(int newValue)
{
    SelectorSlider *slider = (SelectorSlider *) sender();
    cards[slider->cardIndex].controls[slider->controlIndex].slider.values[slider->valueIndex] = newValue;

    try {
        AlsaAPI::updateControl(cards[slider->cardIndex], cards[slider->cardIndex].controls[slider->controlIndex]);
    } catch (QString e) {
        QMessageBox::critical(this, "Error", e);
    }
}
