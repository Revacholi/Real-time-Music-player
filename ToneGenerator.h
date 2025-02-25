#ifndef TONE_H__
#define TONE_H__

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "TimeMeasure.h"
#include "Print.h"

typedef struct {
	Object super;
	int volumn; // [1, 20]
	int freq;  
	int period;
	int flag;  // dac flag 1-vol 0-0
	int muted; // 1-not muted. 0-muted
	int useDeadline; // 0-don't use 1-use
	
	Serial *ser;
	TimeMeasure *timeMeasure;
	
} ToneGenerator;

void playTone(ToneGenerator *);
void setMuted(ToneGenerator *self, int c);
void setVolumn(ToneGenerator *self, int c);
void setUseDeadline(ToneGenerator *self, int c);
void setPeriod(ToneGenerator *self, int c);

#endif
