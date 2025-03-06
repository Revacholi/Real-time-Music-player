#ifndef COMMON_H
#define COMMON_H

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "sioTinyTimber.h"

extern const int brotherJohnFrequencyIndices[32];
extern const float brotherJohnBeatUnit[32];
extern const int offset;
extern int frequencies[25];

extern SysIO sio;
extern Serial sci0;

#endif