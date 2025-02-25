#ifndef TIME_H__
#define TIME_H__
#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "Print.h"

typedef struct {
	Timer timer;
	Time start;      
    Time finish;     
    Time diff;       
	double values[500]; // 500 times
	int index;
	Time maximum;
	Time average;
	Serial *ser;
} TimeMeasure;

void startMeasure(TimeMeasure *self);
void endMeasure(TimeMeasure *self);
double getMaximum(TimeMeasure *self);
double getAverage(TimeMeasure *self);

#endif /* TIME_H__*/