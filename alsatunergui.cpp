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
    /* Remove all old card controls */
    clearLayout(ui->cardControlAreaLayout);

    /* Add new controls */
    for (int i = 0; i < newCard.controls.length(); ++i) {
        QVBoxLayout *currentControl;

        QLabel *label;

        QHBoxLayout *sliderLayout;
        SelectorSlider *slider;

        SelectorComboBox *selector;

        SelectorCheckBox *flag;

        currentControl = new QVBoxLayout();
        switch (newCard.controls[i].type) {
        case SLIDER:
            label = new QLabel();
            label->setText(newCard.controls[i].label);
            currentControl->addWidget(label);

            sliderLayout = new QHBoxLayout();
            for (int j = 0; j < newCard.controls[i].slider.values.length(); ++j) {
                slider = new SelectorSlider();

                // Store the ALSA metadata for later usage
                slider->cardIndex = newIndex;
                slider->controlIndex = i;
                slider->valueIndex = j;

                // Fake all sliders to 0 to 100 and scale in software
                slider->setMinimum(0);
                slider->setMaximum(100);

                slider->setValue(newCard.controls[i].slider.values[j]);
                connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(sliderValueChanged(int)));
                sliderLayout->addWidget(slider);
            }
            currentControl->addLayout(sliderLayout);

            break;
        case SELECTOR:
            label = new QLabel();
            label->setText(newCard.controls[i].label);
            currentControl->addWidget(label);

            selector = new SelectorComboBox();

            // Store the ALSA metadata for later usage
            selector->cardIndex = newIndex;
            selector->controlIndex = i;

            selector->addItems(newCard.controls[i].selector.options);
            selector->setCurrentIndex(newCard.controls[i].selector.selectedIndex);
            connect(selector, SIGNAL(currentIndexChanged(int)), this, SLOT(selectorSelectionChanged(int)));
            currentControl->addWidget(selector);

            break;
        case FLAG:
            flag = new SelectorCheckBox();

            // Store the ALSA metadata for later usage
            flag->cardIndex = newIndex;
            flag->controlIndex = i;

            flag->setText(newCard.controls[i].label);
            flag->setChecked(newCard.controls[i].flag.isSelected);
            connect(flag, SIGNAL(toggled(bool)), this, SLOT(flagStateChanged(bool)));
            currentControl->addWidget(flag);

            break;
        }
        ui->cardControlAreaLayout->addLayout(currentControl);
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
    cards[slider->cardIndex].controls[slider->controlIndex].slider.values[slider->valueIndex]
            = cards[slider->cardIndex].controls[slider->controlIndex].slider.min +
            ((cards[slider->cardIndex].controls[slider->controlIndex].slider.max -
            cards[slider->cardIndex].controls[slider->controlIndex].slider.min) * newValue) / 100;
    try {
        AlsaAPI::updateControl(cards[slider->cardIndex], cards[slider->cardIndex].controls[slider->controlIndex]);
    } catch (QString e) {
        QMessageBox::critical(this, "Error", e);
    }
}
