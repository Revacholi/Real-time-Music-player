#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "TimeMeasure.h"
#include "Print.h"
typedef struct {
	Object super;
	int backgoundLoopRange; // default = 1000.
	int step; // recommend value is 500.
	int value; // 1
	int stop; // 0-not stop 1-stop
	int useDeadline; // 0-do not use deadline, 1-use deadline.
	
	Serial *ser;
	TimeMeasure *timeMeasure;
} BackgroundLoad;

void load_bg(BackgroundLoad *self);
void setValue(BackgroundLoad *self, int c);
void setStep(BackgroundLoad *self, int c) ;
void increaseLoad(BackgroundLoad *self);
void decreaseLoad(BackgroundLoad *self);
void stopLoad(BackgroundLoad *self, int c);
void setUseDeadlineForLoad(BackgroundLoad *self, int c);