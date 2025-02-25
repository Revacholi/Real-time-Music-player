
#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "Print.h"
#include "ToneGenerator.h"
#include "Common.h"
typedef struct {
    Object super;
    int tempo; // BPM.  default value: 120  unit: bpm.
    int basicBeatLength; // 60 / tempo * 1000,  unit:ms.
    int silence; // default value: 50, unit:ms
    int key; // the changing key value.
    int index; // music frequency index
    
    ToneGenerator *toneGenerator;
} MusicPlayer;

void startPlay(MusicPlayer *self);
void play(MusicPlayer *self);
void setBPM(MusicPlayer *self, int c);
void setBasicBeatLength(MusicPlayer *self, int c);
void setSilence(MusicPlayer *self, int c);
void setKey(MusicPlayer *self, int c);