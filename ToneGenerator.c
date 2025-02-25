#include "ToneGenerator.h"

//#define DAC_REGISTER *((int *)0x4000741C);

void playTone(ToneGenerator *self) {
	int *DAC_REG = (int *)0x4000741C;

		startMeasure(self->timeMeasure);
        // mute check
        if (self->muted == 0) {
        	*DAC_REG = 0;
        } else {
        	if (self->flag) {
        		*DAC_REG = self->volumn * self->flag;
        	} else {
        		*DAC_REG = 0;
        	}
        	//print(self->ser, "regValue: %d", *DAC_REG);
        	self->flag = !self->flag;
        }
		endMeasure(self->timeMeasure,100000);

		Time dl = self->useDeadline ? USEC(100) : USEC(0);
		SEND(USEC(self->period), dl, self, playTone, 0);
//		SEND((Time)0, USEC(self->period), self, playTone, 0);
		
}

void setMuted(ToneGenerator *self, int c) {
 	self->muted = c;
}

void setVolumn(ToneGenerator *self, int c) {
  	self->volumn = c;
	if (self->volumn < 1) self->volumn = 1;
	if (self->volumn > 20) self->volumn = 20;
    return;
}

void setUseDeadline(ToneGenerator *self, int c) {
	self->useDeadline = c;
}

void setPeriod(ToneGenerator *self, int c) {
	self->period = c;
}