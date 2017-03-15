#include "alsaapi.h"

#include <dirent.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <sound/asound.h>

#include <QDebug>

static AlsaCard getCardInfo(QString path)
{
    int cardFd;
    struct snd_ctl_card_info cardInfo;
    struct snd_ctl_elem_list elemList;

    AlsaCard card;

    cardFd = open(path.toUtf8().data(), O_RDWR);
    if (cardFd == -1)
        throw QString("Error opening sound card CTL: " + QString::fromUtf8(strerror(errno)));

    if (ioctl(cardFd, SNDRV_CTL_IOCTL_CARD_INFO, &cardInfo) == -1)
        throw QString("Error getting sound card info: " + QString::fromUtf8(strerror(errno)));

    card.cardId = cardInfo.card;
    card.name = QString::fromUtf8((const char *) cardInfo.name);

    memset(&elemList, 0, sizeof(elemList));
    if (ioctl(cardFd, SNDRV_CTL_IOCTL_ELEM_LIST, &elemList) == -1)
        throw QString("Error getting sound card property list: " + QString::fromUtf8(strerror(errno)));

    elemList.space = elemList.count;
    elemList.pids = new struct snd_ctl_elem_id[elemList.count];

    if (ioctl(cardFd, SNDRV_CTL_IOCTL_ELEM_LIST, &elemList) == -1)
        throw QString("Error filling sound card property list: " + QString::fromUtf8(strerror(errno)));

    for (int i = 0; i < elemList.count; ++i) {
        struct snd_ctl_elem_info elemInfo;
        struct snd_ctl_elem_value elemValue;
        AlsaControl currentControl;

        // Read the elem info
        elemInfo.id.numid = elemList.pids[i].numid;
        if (ioctl(cardFd, SNDRV_CTL_IOCTL_ELEM_INFO, &elemInfo) == -1)
            throw QString("Error getting sound card control info: " + QString::fromUtf8(strerror(errno)));

        if (elemInfo.id.iface != SNDRV_CTL_ELEM_IFACE_MIXER) // We only care about mixer stuff
            continue;

        currentControl.controlId = elemInfo.id.numid;
        currentControl.label = QString::fromUtf8((const char *) elemInfo.id.name);

        // Read the elem value
        elemValue.id.numid = elemInfo.id.numid;
        if (ioctl(cardFd, SNDRV_CTL_IOCTL_ELEM_READ, &elemValue) == -1)
            throw QString("Error reading sound card control value: " + QString::fromUtf8(strerror(errno)));

        switch (elemInfo.type) {
        case SNDRV_CTL_ELEM_TYPE_NONE:
            throw QString("Invalid sound card control type.");
        case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
            currentControl.type = FLAG;
            currentControl.flag.isSelected = (bool) elemValue.value.integer.value[0]; // FIXME: allow changing other than the first if the count of values is > 0
            break;

        case SNDRV_CTL_ELEM_TYPE_INTEGER: // NOTE: the step property of slider controls IS unused
            if (elemInfo.value.integer.max - elemInfo.value.integer.min == 1) { // !HACK: convert range 1 sliders into switches
                currentControl.label = currentControl.label.replace("Volume", "Playback");
                currentControl.type = FLAG;
                currentControl.flag.isSelected = elemValue.value.integer.value[0] == elemInfo.value.integer.min ? false : true;
            } else {
                currentControl.type = SLIDER;
                currentControl.slider.sliderType = INT;
                currentControl.slider.min = elemInfo.value.integer.min;
                currentControl.slider.max = elemInfo.value.integer.max;
                for (int i = 0; i < elemInfo.count; ++i) {
                    currentControl.slider.values.append(elemValue.value.integer.value[i]);
                }
            }
            break;
        case SNDRV_CTL_ELEM_TYPE_INTEGER64:
            /* Disable this horrible (mostly) useless int64 hack and it doesn't do anything on 64-bit platforms
            currentControl.type = SLIDER;
            currentControl.slider.sliderType = INT64;
            currentControl.slider.min = elemInfo.value.integer64.min;
            currentControl.slider.max = elemInfo.value.integer64.max;
            for (int i = 0; i < elemInfo.count; ++i) {
                currentControl.slider.values.append(elemValue.value.integer64.value[i]);
            }*/
            break;

        case SNDRV_CTL_ELEM_TYPE_ENUMERATED:
            currentControl.type = SELECTOR;
            for (int i = 0; i < elemInfo.value.enumerated.items; ++i) {
                elemInfo.value.enumerated.item = i;
                if (ioctl(cardFd, SNDRV_CTL_IOCTL_ELEM_INFO, &elemInfo) == -1)
                    throw QString("Error filling sound card control info: " + QString::fromUtf8(strerror(errno)));
                currentControl.selector.options.append(QString::fromUtf8((const char *) elemInfo.value.enumerated.name));
            }
            currentControl.selector.selectedIndex = elemValue.value.enumerated.item[0];
            break;

        default:
            // Ignore other control types
            continue;
        }
        // Change the name of the digital audio controls to something more user friendly
        currentControl.label = currentControl.label.replace("IEC958", "S/PDIF");
        card.controls.append(currentControl);
    }
    delete elemList.pids;

    close(cardFd);
    return card;
}

void AlsaAPI::updateControl(AlsaCard card, AlsaControl control)
{
    int cardFd;
    struct snd_ctl_elem_value elemValue;

    cardFd = open(QString("/dev/snd/controlC" + QString::number(card.cardId)).toUtf8().data(), O_RDWR);
    if (cardFd == -1)
        throw QString("Error opening sound card control device: " + QString::fromUtf8(strerror(errno)));


    elemValue.id.numid = control.controlId;
    switch (control.type) {
    case SLIDER:
        switch (control.slider.sliderType) {
        case INT:
            for (int i = 0; i < control.slider.values.length(); ++i) {
                qDebug() << control.slider.values.at(i);
                elemValue.value.integer.value[i] = control.slider.values.at(i);
            }
            break;
        case INT64:
            for (int i = 0; i < control.slider.values.length(); ++i) {
                qDebug() << control.slider.values.at(i);
                elemValue.value.integer64.value[i] = control.slider.values.at(i);
            }
            break;
        }
        break;
    case SELECTOR:
        elemValue.value.enumerated.item[0] = control.selector.selectedIndex;
        break;
    case FLAG:
        elemValue.value.integer.value[0] = (long) control.flag.isSelected;
        break;
    }

    if (ioctl(cardFd, SNDRV_CTL_IOCTL_ELEM_WRITE, &elemValue) == -1)
        throw QString("Error writing to sound card control: " + QString::fromUtf8(strerror(errno)));

    close(cardFd);
}

QList<AlsaCard> AlsaAPI::listSoundCards()
{
    DIR *cardDir;
    struct dirent *ent;

    QList<AlsaCard> cards;

    cardDir = opendir("/dev/snd");
    if (cardDir == NULL)
        throw QString("Error getting sound card list: " + QString::fromUtf8(strerror(errno)));

    while (ent = readdir(cardDir)) {
        if (strstr(ent->d_name, "control") != NULL) {
            cards.append(getCardInfo("/dev/snd/" + QString::fromUtf8(ent->d_name)));
        }
    }

    closedir(cardDir);
    return cards;
}
