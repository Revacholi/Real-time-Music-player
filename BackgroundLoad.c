#include "BackgroundLoad.h"


void load_bg(BackgroundLoad *self) {
	if (self->stop) {
		return;
	}
	startMeasure(self->timeMeasure);
	int num = self->backgoundLoopRange;
	
	for (int i = 0; i < num; i++) {
		//print(self->ser, "i = %d\n", i);
	}


	endMeasure(self->timeMeasure);
	
	Time dl = self->useDeadline ? USEC(1300) : USEC(0);
	SEND(USEC(1300), dl, self, load_bg, 0);
//	AFTER(USEC(1300), &self, load, 0);
}

void setUseDeadlineForLoad(BackgroundLoad *self, int c) {
	self->useDeadline = c;
}

// increase by steps.
void decreaseLoad(BackgroundLoad *self) {
	self->backgoundLoopRange = (self->backgoundLoopRange - self->step) < 0 ? 0 : (self->backgoundLoopRange - self->step);
	print(self->ser, "The background loop range has changed to %d\n", self->backgoundLoopRange);
}

// decrease by steps.
void increaseLoad(BackgroundLoad *self) {
	self->backgoundLoopRange = self->backgoundLoopRange + self->step;
    print(self->ser, "The background loop range has changed to %d\n", self->backgoundLoopRange);
}

// stop
void stopLoad(BackgroundLoad *self, int c) {
	self->stop = c;
	
}


// -------------------- maybe not useful, ask for ta.-----------------

// set arbitrary value;
void setValue(BackgroundLoad *self, int c) {
	self->value = c;	
	print(self->ser, "The background loop range has changed to %d\n", self->backgoundLoopRange);
}

void setStep(BackgroundLoad *self, int c) {
	self->step = c;
}