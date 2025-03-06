#ifndef BUTTON_H
#define BUTTON_H

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "sioTinyTimber.h"
#include "Print.h"
#include "MusicPlayer.h"

#define RELEASED 1
#define PRESSED  0
#define MAX_BURST 3

#define BUTTON_BG_DEADLINE    USEC(800)
#define BUTTON_BG_PERIODICITY USEC(1300)

typedef enum {
    PRESS_MOMENTARY,
    PRESS_AND_HOLD,
} BUTTON_MODE;

typedef struct {
    Object super;
    BUTTON_MODE mode;
    int lastStatus; // not needed if using interrupt
    Timer timerCallback;
    Timer timerRelease;
    Timer timerPress;
    int index;
    int timeBuffer[MAX_BURST]; // store the history of PRESS interval (ms)
    Msg abortMessage; 
    Serial *ser;
    SysIO *sio;
} Buttons;


void buttonCallback(Buttons *self);
void clearIntervalHistory(Buttons*);
int compareIntervalHistory(Buttons*, int);
void checkPressAndHold(Buttons *self);


#endif