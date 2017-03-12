#include "alsatunergui.h"
#include "ui_alsatunergui.h"

#include <QDebug>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>

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
        QSlider *slider;

        QComboBox *selector;

        QCheckBox *flag;

        currentControl = new QVBoxLayout();
        switch (newCard.controls[i].type) {
        case SLIDER:
            label = new QLabel();
            label->setText(newCard.controls[i].label);
            currentControl->addWidget(label);

            sliderLayout = new QHBoxLayout();
            for (int j = 0; j < newCard.controls[i].slider.values.length(); ++j) {
                slider = new QSlider();
                slider->setMinimum(newCard.controls[i].slider.min);
                slider->setMaximum(newCard.controls[i].slider.max);
                slider->setValue(newCard.controls[i].slider.values[j]);
                sliderLayout->addWidget(slider);
            }
            currentControl->addLayout(sliderLayout);

            break;
        case SELECTOR:
            label = new QLabel();
            label->setText(newCard.controls[i].label);
            currentControl->addWidget(label);

            selector = new QComboBox();
            selector->addItems(newCard.controls[i].selector.options);
            selector->setCurrentIndex(newCard.controls[i].selector.selectedIndex);
            currentControl->addWidget(selector);

            break;
        case FLAG:
            flag = new QCheckBox();
            flag->setText(newCard.controls[i].label);
            flag->setChecked(newCard.controls[i].flag.isSelected);
            currentControl->addWidget(flag);

            break;
        }
        ui->cardControlAreaLayout->addLayout(currentControl);
    }
}
