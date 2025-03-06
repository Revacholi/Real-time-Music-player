#include "Button.h"

void buttonCallback(Buttons *self) {    
    int currentStatus = SIO_READ(self->sio);

    switch (self->mode) {
        case PRESS_MOMENTARY:
            if (currentStatus == PRESSED){
                self->abortMessage = AFTER(SEC(1), self, checkPressAndHold, 0);
                T_RESET(&self->timerRelease);
                int secInterArrival = SEC_OF(T_SAMPLE(&self->timerPress));
                int msInterArrival = MSEC_OF(T_SAMPLE(&self->timerPress)) + secInterArrival * 1000;
                if (msInterArrival < 100) {
                    T_RESET(&self->timerRelease);
                    T_RESET(&self->timerPress);
                    SIO_TRIG(&sio, 0);
                    //print(self->ser, "Press noise detected %ld ms\n", msInterArrival);
                    return;
                }
                //print(self->ser, "Button pressed, inter-arrival time is %ld ms\n", msInterArrival);
                

                if (self->index == 0)
                    self->timeBuffer[self->index] = msInterArrival; 
                // comparable: no interval differs from another interval by 100 ms (that is strict...)
                //             also interval should no more than 366 ms
                if ((msInterArrival > 2000) && 0) {
                    clearIntervalHistory(self);
                    print(self->ser, "Interval too long\n");
                }
                else if(compareIntervalHistory(self, msInterArrival) || 0) {
                    print(self->ser, "Interval differs too much\n");
                    clearIntervalHistory(self);
                }
                else {
                    self->timeBuffer[self->index] = msInterArrival;
                    self->index = (self->index + 1) % (MAX_BURST * 2);
                    if (self->index >= MAX_BURST){
                        int average = (self->timeBuffer[0] + self->timeBuffer[1] + self->timeBuffer[2]) / 3;
                        SYNC(&musicPlayer, setBPM, 60000 / average);
                        print(self->ser, "BPM set to %d\n", 60000 / average);
                    }
                }
                print(self->ser, "timeBuffer: [%d, %d, %d], index: %d\n",
                    self->timeBuffer[0], self->timeBuffer[1], self->timeBuffer[2], self->index);


                T_RESET(&self->timerPress);
                SIO_TRIG(&sio, 1);
            }
            else {           
                int secRelease = SEC_OF(T_SAMPLE(&self->timerRelease));
                int msRelease = MSEC_OF(T_SAMPLE(&self->timerRelease)) + secRelease * 1000;
                T_RESET(&self->timerRelease);
                if (msRelease < 100) {
                    T_RESET(&self->timerRelease);
                    T_RESET(&self->timerPress);
                    SIO_TRIG(&sio, 0);
                    //print(self->ser, "Release noise detected %ld ms\n", msRelease);
                    return;
                }
                //print(self->ser, "Button released, pressed time is %ld ms\n", msRelease);
                SIO_TRIG(&sio, 0);

                if (msRelease < 999) {
                    ABORT(self->abortMessage);
                }
            }
            break;

        case PRESS_AND_HOLD:
            if (currentStatus == PRESSED){
                T_RESET(&self->timerRelease);
                SIO_TRIG(&sio, 1);
            } 
            else {           
                int secRelease = SEC_OF(T_SAMPLE(&self->timerRelease));
                int msRelease = MSEC_OF(T_SAMPLE(&self->timerRelease)) + secRelease * 1000;
                T_RESET(&self->timerRelease);
                if (msRelease < 100) {
                    T_RESET(&self->timerRelease);
                    T_RESET(&self->timerPress);
                    SIO_TRIG(&sio, 0);
                    //print(self->ser, "Release noise detected %ld ms\n", msRelease);
                    return;
                }
                print(self->ser, "Quit from PRESS_AND_HOLD, pressed time is %ld ms\n", msRelease);
                SIO_TRIG(&sio, 0);
                // PRESS_MOMENTARY -> PRESS_AND_HOLD
                if (msRelease > 2000){
                    SYNC(&musicPlayer, setBPM, 120);
                    print(self->ser, "BPM reset to %d\n", 120);
                }
                self->mode = PRESS_MOMENTARY;
            }
            break;
        default:
            break;
    }
}

void clearIntervalHistory(Buttons *self){
    for (int i = 0; i < MAX_BURST; i++){
        self->timeBuffer[i] = 0;
    }
    self->index = 0;
}

void checkPressAndHold(Buttons *self){
    self->mode = PRESS_AND_HOLD;
    print(self->ser, "Entered PRESS_AND_HOLD MODE\n");
    return;
}


int compareIntervalHistory(Buttons *self, int interval){
    int tolerance = 100; // max diff 100 ms
    int diff, max_check;
    max_check = self->index > MAX_BURST ? MAX_BURST : self->index;
    for(int i = 0; i < max_check; i++){
        diff = interval - self->timeBuffer[i];
        diff = diff > 0 ? diff : -diff;
        if (diff > tolerance)
            return 1;
    }
    return 0;
}

