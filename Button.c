#include "Button.h"


void buttonCallback(Buttons *self) {    
    int currentStatus = SIO_READ(self->sio);

    switch (self->mode) {
        case PRESS_MOMENTARY:
            if (currentStatus == PRESSED){
                self->abortMessage = AFTER(SEC(1), self, checkPressAndHold, 0);
                if (self->hitFirst == 0) {
                    T_RESET(&self->timerPress);
                    T_RESET(&self->timerRelease);
                    SIO_TRIG(&sio, 1);
                    self->hitFirst = 1;
                    return;
                }

                T_RESET(&self->timerRelease);
                int secInterArrival = SEC_OF(T_SAMPLE(&self->timerPress));
                int msInterArrival = MSEC_OF(T_SAMPLE(&self->timerPress)) + secInterArrival * 1000;
                if (msInterArrival < 100) {
                    T_RESET(&self->timerRelease);
                    T_RESET(&self->timerPress);
                    // SIO_TRIG(&sio, 0);
                    self->hitFirst = 0;
                    //print(self->ser, "Press noise detected %ld ms\n", msInterArrival);
                    return;
                }
                //print(self->ser, "Button pressed, inter-arrival time is %ld ms\n", msInterArrival);
                


                if (msInterArrival > 2050) {
                    clearBuffer(self);
                    print(self->ser, "Interval %ld ms too long\n", msInterArrival);
                }   
                else if(msInterArrival < 150) {
                    clearBuffer(self);
                    print(self->ser, "Interval %ld ms too short\n", msInterArrival);
                }   
                else if(compareIntervalHistory(self, msInterArrival)) {
                    print(self->ser, "Interval %ld ms differs too much\n", msInterArrival);
                    clearBuffer(self);
                }
                else {
                    self->timeBuffer[self->index] = msInterArrival;
                    print(self->ser, "[%d, %d, %d], index: %d\n",
                        self->timeBuffer[0], self->timeBuffer[1], self->timeBuffer[2], self->index);
                    // self->index = (self->index + 1) % (MAX);
                    if (self->index == MAX - 1) {
                        int average = (self->timeBuffer[0] + self->timeBuffer[1] + self->timeBuffer[2]) / 3;
                        if (average > 2000 || average < 200) {
                            clearBuffer(self);
                            print(self->ser, "Out of range [30, 300]\n", average);
                            return;
                        }
                        SYNC(&musicPlayer, setBPM, 60000 / average);
                        print(self->ser, "BPM set to %d\n", 60000 / average);
                        self->hitFirst = 0;
                    }
                    self->index = self->index == MAX - 1 ? 0 : self->index + 1;
                }
               
                T_RESET(&self->timerPress);
                SIO_TRIG(&sio, 1);
            }
            else {           
                int secRelease = SEC_OF(T_SAMPLE(&self->timerRelease));
                int msRelease = MSEC_OF(T_SAMPLE(&self->timerRelease)) + secRelease * 1000;
                T_RESET(&self->timerRelease);
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
                    self->hitFirst = 0;
                    //print(self->ser, "Release noise detected %ld ms\n", msRelease);
                    return;
                }
                print(self->ser, "Quit from PRESS_AND_HOLD, pressed time is %ld ms\n", msRelease);
                SIO_TRIG(&sio, 0);
                // PRESS_MOMENTARY -> PRESS_AND_HOLD
                if (msRelease > 2000){
                    SYNC(&musicPlayer, setBPM, 120);
                    print(self->ser, "BPM reset to %d\n", 120);
                    self->hitFirst = 0;
                }
                self->mode = PRESS_MOMENTARY;
            }
            break;
        default:
            break;
    }
}

void clearBuffer(Buttons *self){
    self->hitFirst = 0;
    for (int i = 0; i < MAX; i++){
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

    for (int i = 0; i < self->index; i++) {
        if (interval - self->timeBuffer[i] > tolerance || self->timeBuffer[i] - interval > tolerance) {
            return 1;
        } 
    }
    return 0;
}
