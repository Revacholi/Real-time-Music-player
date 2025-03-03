#include "MusicPlayer.h"

void startPlay(MusicPlayer *self) {
    // MusicPlayer musicPlayer = {initObject(), 120, 500, 50, 0, 0};
    self->tempo = 120;
    self->basicBeatLength = 500;
    self->silence = 50;
    self->index = 0;
    self->key = 0;
    SYNC(self->toneGenerator, setStopTone, 0);
    // self->toneGenerator->stop = 0;
    self->stop = 0;

    SEND(0, 0, self->toneGenerator, playTone, 0);
    SEND(0, 0, self, play, 0);
}

void play(MusicPlayer *self) {
    if (self->stop) {
        return;
    }
    // period relies on key 
    int period = frequencies[brotherJohnFrequencyIndices[self->index] + offset + self->key];
    int beatLength = self->basicBeatLength * brotherJohnBeatUnit[self->index];

    // set period to tone generator.
    SEND(0,0, self->toneGenerator, setPeriod, period);
    // unmute immediately in order to play sound.
    SEND(0,0, self->toneGenerator, setMuted, 1);
    // set mute to tone generator after playing several length.
    SEND(MSEC(beatLength - self->silence),0, self->toneGenerator, setMuted ,0);

    // if to the end. replay
    if(self->index == 31) {
        self->index = 0;
    } else {
        self->index++;
    }
    
    // invoked every beat length.
    SEND(MSEC(beatLength), 0, self, play, 0);
}

void stopPlay(MusicPlayer *self) {
    SYNC(self, setStopMusic, 1);
    SYNC(self->toneGenerator, setStopTone, 0);
}


void setBPM(MusicPlayer *self, int c) {
    if (c > 240) c = 240;
    if (c < 60) c = 60;
    self->tempo = c;
    // get beat length
    int length = 60 / self->tempo * 1000;
    setBasicBeatLength(self, length);
}

void setBasicBeatLength(MusicPlayer *self, int c) {
    self->basicBeatLength = c;
}

void setSilence(MusicPlayer *self, int c) {
    self->silence = c;
}

void setKey(MusicPlayer *self, int c) {
    self->key = c;
}



void setStopMusic(MusicPlayer *self, int c) {
    self->stop = c;
}