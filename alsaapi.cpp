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

        if (elemInfo.access & SNDRV_CTL_ELEM_ACCESS_READWRITE != SNDRV_CTL_ELEM_ACCESS_READWRITE) // Skip non-writable element
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
            currentControl.type = SLIDER;
            currentControl.slider.min = elemInfo.value.integer.min;
            currentControl.slider.max = elemInfo.value.integer.max;
            for (int i = 0; i < elemInfo.count; ++i) {
                currentControl.slider.values.append(elemValue.value.integer.value[i]);
            }
            break;
        case SNDRV_CTL_ELEM_TYPE_INTEGER64:
            currentControl.type = SLIDER;
            currentControl.slider.min = elemInfo.value.integer64.min;
            currentControl.slider.max = elemInfo.value.integer64.max;
            for (int i = 0; i < elemInfo.count; ++i) {
                currentControl.slider.values.append(elemValue.value.integer64.value[i]);
            }
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

        case SNDRV_CTL_ELEM_TYPE_IEC958:
            continue;
            //qDebug() << "iec958" << endl;
            break;
        case SNDRV_CTL_ELEM_TYPE_BYTES:
            continue;
            //qDebug() << "bytes" << endl;
            break;
        }
        card.controls.append(currentControl);
    }
    delete elemList.pids;

    close(cardFd);
    return card;
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
