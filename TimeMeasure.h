#ifndef TIME_H__
#define TIME_H__
#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "Print.h"

typedef struct {
	Timer timer;
	double values[500]; // 500 times
	int index;
	double maximum;
	double average;
	Serial *ser;
} TimeMeasure;

void startMeasure(TimeMeasure *self);
void endMeasure(TimeMeasure *self, int times);
double getMaximum(TimeMeasure *self);
double getAverage(TimeMeasure *self);

#endif /* TIME_H__*/