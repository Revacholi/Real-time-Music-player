#include "MusicPlayer.h"

void startPlay(MusicPlayer *self) {
    // ToneGenerator toneGenerator = {initObject(), 5, 1000, 500, 1, 1, 1, 0, 0, &sci0, &timer1};
    self->toneGenerator->freq = 1000;
    self->toneGenerator->period = 500;
    self->toneGenerator->flag = 1;
    self->toneGenerator->muted = 1;
    self->toneGenerator->mutedByUser = 1;
    self->toneGenerator->useDeadline = 0;
    self->toneGenerator->stop = 0;

    // MusicPlayer musicPlayer = {initObject(), 120, 500, 50, 0, 0};
    self->tempo = 120;
    self->basicBeatLength = 500;
    self->silence = 50;
    self->index = 0;
    self->key = 0;

    SIO_WRITE(self->sio, 0);  // LED on at start

    SYNC(self->toneGenerator, setStopTone, 0);
    // self->toneGenerator->stop = 0;
    self->stop = 0;

    SEND(0, 0, self->toneGenerator, playTone, 0);
    SEND(0, 0, self, play, 0);
}

void initIndex(MusicPlayer *self) {
    self->index = 0;
}

void toggleLED(MusicPlayer *self, int unused) {
    SIO_WRITE(self->sio, 1);  // LED off
}

void play(MusicPlayer *self) {
    
    // period relies on key 
    int period = frequencies[brotherJohnFrequencyIndices[self->index] + offset + self->key];
    int beatLength = self->basicBeatLength * brotherJohnBeatUnit[self->index];

    // lcd blink
    if (brotherJohnBeatUnit(self->index) == 1) { // a
        // lit once, toggle once
        SEND((Time)0, (Time) 0, self->sio, SIO_WRITE, 0);
        SEND(MSEC(beatLength/2), 0, self, toggleLED, 0);
    } else if (brotherJohnBeatUnit(self->index) == 2) { // b
        // lit once, toggle three times
        SEND((Time)0, (Time) 0, self->sio, SIO_WRITE, 0);
        SEND(MSEC(beatLength/4), 0, self, toggleLED, 0);
        SEND(MSEC(beatLength/2), 0, self, toggleLED, 0);
        SEND(MSEC(3 * beatLength/4), 0, self, toggleLED, 0);
    } else if (brotherJohnBeatUnit(self->index) == 0.5) { // c
        // just toggle once.
        SEND((Time) 0, (Time) 0, self, toggleLED, 0);
    }
    

    // SIO_WRITE(self->sio, 0);  // LED on
    // set period to tone generator.
    SEND(0, 0, self->toneGenerator, setPeriod, period); 
    // unmute immediately in order to play sound.
    SEND(0, 0, self->toneGenerator, setMuted, 1); 
    // set mute to tone generator after playing several length.
    SEND(MSEC(beatLength - self->silence), 0, self->toneGenerator, setMuted ,0);

    // SEND(MSEC(beatLength/2), 0, self, toggleLED, 0);
    // if to the end. replay
    if(self->index == 31) {
        self->index = 0;
    } else {
        self->index++;
    }
    
    if (!self->stop) {
        SEND(MSEC(beatLength), 0, self, play, 0);
    } else {  
        return; 
    }
        
}

void stopPlay(MusicPlayer *self) {
        setStopMusic(self, 1);
        SYNC(self->toneGenerator, setStopTone, 1);
    
}


void setBPM(MusicPlayer *self, int c) {
    if (c > 240) c = 240;
    if (c < 60) c = 60;
    self->tempo = c;
    // get beat length
    int length = 60 * 1000 / self->tempo;
    setBasicBeatLength(self, length);
}

void setBasicBeatLength(MusicPlayer *self, int c) {
    self->basicBeatLength = c;
}

void setSilence(MusicPlayer *self, int c) {
    self->silence = c;
}

void setKey(MusicPlayer *self, int c) {
    if (c > 5) c = 5;
    if (c < -5) c = -5;
    self->key = c;
}



void setStopMusic(MusicPlayer *self, int c) {
    self->stop = c;
}