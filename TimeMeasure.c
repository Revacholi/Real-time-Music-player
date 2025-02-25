#include "TimeMeasure.h"


#define sizeofvalues 500
 

Time start, diff, finish;

void startMeasure(TimeMeasure *self) {
	if (self->index == 500) {
		return;
	}
	start = CURRENT_OFFSET();
}

void endMeasure(TimeMeasure *self, int times) {
	if (self->index == 500) {
		return;
	}
	finish = CURRENT_OFFSET();
	diff = finish - start;
	long diff_usec = USEC_OF(diff); 
	
	// maintain maximum
	if(diff_usec > self->maximum) {
		self->maximum = diff_usec;
	}
	
	// write into buffer. 
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