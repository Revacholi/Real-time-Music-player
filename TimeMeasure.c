#include "TimeMeasure.h"


#define sizeofvalues 500


void startMeasure(TimeMeasure *self) {
	if (self->index == sizeofvalues) {
		return;
	}
	self->start = CURRENT_OFFSET();
}

void endMeasure(TimeMeasure *self) {
	if (self->index == sizeofvalues) {
		return;
	}
	
	self->finish = CURRENT_OFFSET();
    self->diff = self->finish - self->start;
    long diff_usec = USEC_OF(self->diff);
    
    // maintain maximum
    if(diff_usec > self->maximum) {
        self->maximum = diff_usec;
    }
    
    // write into buffer
    self->values[self->index++] = (double) diff_usec;

 	//print(self->ser, "%ld\t", diff_usec);
}

double getMaximum(TimeMeasure *self) {
	return self->maximum;
}

double getAverage(TimeMeasure *self) {
	double sum = 0;
	for (int i = 0; i < self->index; i++) {
		sum += self->values[i];
	}
	return sum / self->index;
}