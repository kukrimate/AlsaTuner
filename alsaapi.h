#ifndef ALSAAPI_H
#define ALSAAPI_H

#include <QList>
#include <QString>

typedef enum {
    SLIDER,
    SELECTOR,
    FLAG
} AlsaControlType;

typedef struct {
    long long min;
    long long max;
    QList<long long> values;
} AlsaControlSliderData;

typedef struct {
    QStringList options;
    int selectedIndex;
} AlsaControlSelectorData;

typedef struct {
    bool isSelected;
} AlsaControlFlagData;

typedef struct {
    /* The name of the control */
    AlsaControlType type;
    /* The numerical ID of the control */
    int controlId;
    /* Extra data for some controls */
    AlsaControlSliderData slider;
    AlsaControlSelectorData selector;
    AlsaControlFlagData flag;
    /* Label of the control */
    QString label;
} AlsaControl;

typedef struct {
    int cardId;
    QList<AlsaControl> controls;
    QString name;
} AlsaCard;

class AlsaAPI
{
public:
    static QList<AlsaCard> listSoundCards();
};

#endif // ALSAAPI_H
