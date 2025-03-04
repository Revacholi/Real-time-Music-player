#include "ToneGenerator.h"



//#define DAC_REGISTER *((int *)0x4000741C);

void playTone(ToneGenerator *self) {
	int8_t *DAC_REG = (int8_t *)0x4000741C;
		if (self->stop) {
			return;
		}
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
	  }

		Time dl = self->useDeadline ? USEC(100) : USEC(0);
		SEND(USEC(self->period), dl, self, playTone, 0);
}

void measureTone(ToneGenerator *self) {
	int8_t *DAC_REG = (int8_t *)0x4000741C;

		//self->timeMeasure->start = CURRENT_OFFSET();
		startMeasure(self->timeMeasure);

		for (int i = 0; i < 1000; i++) {
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
		}

		endMeasure(self->timeMeasure);

		// self->timeMeasure->finish = CURRENT_OFFSET();
		// self->timeMeasure->diff = self->timeMeasure->finish - self->timeMeasure->start; 
		// self->timeMeasure->maximum = self->timeMeasure->maximum > (self->timeMeasure->finish - self->timeMeasure->start) ? 
		// 	self->timeMeasure->maximum : (self->timeMeasure->finish - self->timeMeasure->start);
		// //print(self->ser, "%ld", USEC_OF(maxTime));
		// //print(self->ser, "%ld\n", USEC_OF(self->timeMeasure->diff));
		// //print(self->ser, "%ld", USEC_OF(self->timeMeasure->maximum));
		// //print(self->ser, "%ld\n", USEC_OF(self->timeMeasure->diff) / 1000);


		Time dl = self->useDeadline ? USEC(100) : USEC(0);
		SEND(USEC(self->period), dl, self, measureTone, 0);
}

void setMuted(ToneGenerator *self, int c) {
 	self->muted = c;
}

void setUserMuted(ToneGenerator *self, int c) {
	self->mutedByUser = c;
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

int getMutedByUser(ToneGenerator *self) {
    return self->mutedByUser;
}

int getPeriod(ToneGenerator *self) {
    return self->period;
}

int getUseDeadline(ToneGenerator *self) {
    return self->useDeadline;
}

void setStopTone(ToneGenerator *self, int c) {
	self->stop = c;
}