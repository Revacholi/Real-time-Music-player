#ifndef BUTTON_CONTROLLER_H
#define BUTTON_CONTROLLER_H

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "sioTinyTimber.h"

typedef struct {
    Object super;
    Serial *sci;
    SysIO *sio;        
    Time lastPressTime;
    Time lastValidActivation;
    int isHoldMode;
} ButtonController;

#define initButtonController(sci, sio) { \
    initObject(), \
    sci, \
    sio, \
    0, \
    0, \
    0 \
}

void buttonCallback(ButtonController*, int);
void checkHoldMode(ButtonController*, int);

#endif