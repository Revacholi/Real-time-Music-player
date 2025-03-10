#include "Button.h"


void buttonAnotherCallback(Buttons *self) {
    int currentStatus = SIO_READ(self->sio);

            if (currentStatus == PRESSED) {
                // 1. get delta between current press event and last press event. 
                if (self->hitFirst == 0) {
                    T_RESET(self->timer);
                    SIO_TRIG(&sio, 1);
                    self->hitFirst = 1;
                    return;
                }
                Time timer_ = T_SAMPLE(self->timer);
                int delta = MSEC_OF(timer_);
                // 消抖
                if (delta < 100) {
                    return;
                }

                // 2. determine if it is valid(like range, compare to histories). 
                // 2.1 range check
                if (delta >= 2000 || delta <= 200) {
                    clearIntervalHistory(self);
                    print(self->ser, "Interval too long\n");
                    return;
                }
                // 2.2 compare with histories
                if (compareIntervalHistoryAnotherVersion(self, delta)) {
                    clearIntervalHistory(self);
                    print(self->ser, "Interval differs too much\n");
                    return;
                }
                //      - if valid, write into history, and probaboly set bpm here. TODO
                self->timeBuffer[self->index] = delta;
                // TODO: 是否在这里设置bpm，需求需要明确一个点才能决定：就是当最后一下tap进入了press and hold模式时，是否能设置bpm。暂时在这里先设置bpm
                if (self->index == MAX_BURST - 1) {
                    // set bpm
                    int average = (self->timeBuffer[0] + self->timeBuffer[1] + self->timeBuffer[2]) / 3;
                    SYNC(&musicPlayer, setBPM, 60000 / average);
                    print(self->ser, "BPM set to %d\n", 60000 / average);
                }
                self->index = self->index == MAX_BURST - 1 ? 0 : self->index + 1;

                // 3. change callback trigger. 让释放接收回调
                SIO_TRIG(&sio, 1);
                // 4. change baseline.
                // self->deltaBetweenPressAndPress = delta;
                T_RESET(self->timer);


            } else if (currentStatus == RELEASED) {
                // 1. get delta between current release event and last press event. 
                Timer timer_ = T_SAMPLE(self->timer);
                int delta = MSEC_OF(timer_);

                // 2. if more than 1s, enter into PRESS_AND_HOLD mode. if not could probaboly set bpm here. TODO
                if (delta >= 1000) {
                    // PRESS_AND_HOLD;
                    print(self->ser, "Entered PRESS_AND_HOLD MODE\n");
                    print(self->ser, "Press and hold duration time is : %d\n ", self->deltaBetweenPressAndRelease);
                    clearIntervalHistory(self);
                    
                }
                if (delta >= 2000) {
                    // 回到默认120bpm
                    SYNC(&musicPlayer, setBPM, 120);
                    print(self->ser, "BPM reset to %d\n", 120);
                }
                // 3. change callback trigger. 让下一轮的按下接收回调
                SIO_TRIG(&sio, 0);
                // self->deltaBetweenPressAndRelease = delta;
            }
}


int compareIntervalHistoryAnotherVersion(Buttons *self, int interval){
    int tolerance = 100; // max diff 100 ms

    for (int i = 0; i < self->index; i++) {
        if (interval - self->timeBuffer[i] > 100 || self->timeBuffer[i] - interval > 100) {
            return 1;
        } 
    }
    return 0;
}



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
    self->hitFirst = 0;
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

