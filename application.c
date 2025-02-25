#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"

#include "Print.h"
#include "TimeMeasure.h"
#include "ToneGenerator.h"
#include "BackgroundLoad.h"
#include "MusicPlayer.h"

// Enumeration for different modes
enum Mode { DEFAULT = 0, CONDUCTOR = 1, MUSICIAN = 2 };

// Structure to hold mode information
typedef struct {
    enum Mode mode;
    const char *name;
    const char *menuPrompt;
} ModeInfo;

// ModeInfo initialization
const ModeInfo modeInfo[] = {
    [DEFAULT] = {
        DEFAULT,
        "Default",
        "Enter mode (1 for Conductor, 2 for Musician): "
    },
    [CONDUCTOR] = {
        CONDUCTOR,
        "Conductor",
        "Conductor Mode:\n"
        "1. [num]e - Show 3-history sum/median\n"
        "2. F - Clear history\n"
        "3. [num]K - Brother John periods\n"
        "4. g - Generate 1kHz tone\n"
        "5. [num]v - Set volume\n"
        "6. m - Toggle mute\n"
        "7. D - Toggle deadlines\n"
        "8. l - Start background load\n"
        "9. s - Stop background load\n"
        "10. i - Increase load\n"
        "11. d - Decrease load\n"
        "12. t - Tone generator WCET\n"
        "13. y - Load WCET\n"
        "14. q - Quit to mode selection\n"
    },
    [MUSICIAN] = {
        MUSICIAN,
        "Musician",
        "Musician Mode:\n"
        "TODO: Add musician mode commands\n"
        "1. q - Quit to mode selection\n"
    }
};

// Function to get mode by value
enum Mode getModeByValue(int value) {
    if (value >= DEFAULT && value <= MUSICIAN) {
        return (enum Mode)value;
    }
    return DEFAULT;
}

// Structure to hold request information
typedef struct {
    char inputBuffer[128];
    int inputBufferIndex;
} Request;

// Structure to hold application state
typedef struct {
    Object super;
    int count;
    char c;
    Request request;
    enum Mode currentMode;  
} App;

// Structure to hold storage information
typedef struct {
    Object super;
    int storage[128];
    int capacity;
    int count;
    int index;
    int maxNumber;
    int minNumber;
    int sum;
} Storage;

// Function declarations
enum Mode getModeByValue(int);
int getRequestNumFromBuffer(App *self);
void reader(App*, int);
void receiver(App*, int);
void threeHistory(Request*, Storage*, int);
void initial();
int getFrequency(int key);

// Initialize storage for three-history
Storage storageForThreeHistory = { initObject(), {0}, 3, 0, 0, -9999, 9999, 0};

// Initialize application state
App app = { initObject(), 0, 'X', { {0}, 0 }, DEFAULT };

// Initialize serial and CAN communication
Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

// Initialize timers and generators

TimeMeasure timer1 = { initTimer(), 0, 0, 0, {0.0}, 0, 0.0, 0.0, &sci0 };
TimeMeasure timer2 = { initTimer(), 0, 0, 0, {0.0}, 0, 0.0, 0.0, &sci0 };
ToneGenerator toneGenerator = {initObject(), 5, 1000, 500, 1, 1, 0, &sci0, &timer1};
BackgroundLoad backgroundLoad = {initObject(), 1000, 500, 1, 0, 0, &sci0, &timer2};
MusicPlayer musicPlayer = {initObject(), 120, 500, 50, 0, 0};

// Array to hold frequencies
int frequencies[25]; // -10 -- 14
const int offset = 10;
const int brotherJohnFrequencyIndices[32] = {0,2,4,0,0,2,4,0,4,5,7,4,5,7,7,9,7,5,4,0,7,9,7,5,4,0,0,-5,0,0,-5,0};
const float brotherJohnBeatUnit[32] = {1,1,1,1,1,1,1,1,1,1,2,1,1,2,0.5,0.5,0.5,0.5,1,1,0.5,0.5,0.5,0.5,1,1,1,1,2,1,1,2};


// Function to calculate frequency based on key
int getFrequency(int key) {
    double frequency = 440 * pow(2, key / 12.0);
    return (int)(1e6 / (2 * frequency));
}

// Function to initialize frequencies
void initial() {
    for(int i = -10; i < 14; i++) {
        int index = i + offset;
        frequencies[index] = getFrequency(i);
    }
}

// Function to get periods by changing key
void getPeriodsByChangingKey(Serial *ser, int key) {
    if (key < -5 || key > 5) {
        print(ser, "Key out of range (-5 to 5)\n");
        return;
    }
    for(int i = 0; i < 32; i++) {
        int r = frequencies[brotherJohnFrequencyIndices[i] + offset + key];
        print(ser, "%d ", r);
    }
    print(ser, "\n");
}

// Function to handle three-history operations
void threeHistory(Request *req, Storage *sto, int c) {
    switch(c) {
        case 'F': // erase history
            memset(&sto->storage, 0, sizeof(sto->storage));
            req->inputBufferIndex = 0;
            sto->index = 0;
            sto->maxNumber = 0;
            sto->minNumber = 0;
            sto->sum = 0;
            sto->count = 0;
            print(&sci0, "3-History erased\n");
            break;
        case 'e': // store the number
            // 1. convert to int from request buffer.
            int num = 0;
            req->inputBuffer[req->inputBufferIndex] = '\0';
            num = atoi(req->inputBuffer);
            // 2. put it into storage;
            int originNum = sto->storage[sto->index];
            int originSum = sto->sum;
            sto->storage[sto->index] = num;
            // update index
            sto->index = (sto->index + 1 == sto->capacity) ? 0 : sto->index + 1;
            sto->count++;
            sto->sum = sto->sum - originNum + num;
            
            // maintain max and min in Storage.
            if (sto->maxNumber == originNum) {
                int originMid = originSum - sto->minNumber - originNum;
                if (num > originMid) { sto->maxNumber = num; } else sto->maxNumber = originMid;
                if (num < sto->minNumber) { sto->minNumber = num; }
            } else if (sto->minNumber == originNum) {
                int originMid = originSum - sto->maxNumber - originNum;
                if (num < originMid) { sto->minNumber = num; } else sto->minNumber = originMid;
                if (num > sto->maxNumber) { sto->maxNumber = num; }
            } else {
                if (num > sto->maxNumber) { sto->maxNumber = num; }
                if (num < sto->minNumber) { sto->minNumber = num; }
            }

            // 3. calculate or get 3 history from storage.
            int sum;
            int median;
            if (sto->count == 1) {
                sum = num;
                median = num;
            } else if (sto->count == 2) {
                sum = sto->sum;
                median = (sto->maxNumber + sto->minNumber) /2;
            } else {
                sum = sto->sum;
                median = sum - sto->minNumber - sto->maxNumber;
            }

            // 4. print
            print(&sci0, "Entered integer %d: sum = %d, median = %d\n", num, sum, median);
            req->inputBufferIndex = 0;
            break;
        default:
            req->inputBuffer[req->inputBufferIndex++] = c;
            break;
    }
}

// Function to get request number from buffer
int getRequestNumFromBuffer(App *self) {
    self->request.inputBuffer[self->request.inputBufferIndex] = '\0';
    int num = atoi(self->request.inputBuffer);
    self->request.inputBufferIndex = 0;
    return num;
}

// Function to dispatch commands based on current mode
void dispatch(App *self, int c) {
    switch (self->currentMode) {
        case CONDUCTOR:
            switch(c) {
                case 'p': // play music
                    SEND((Time) 0, (Time) 0, &musicPlayer, startPlay, 0);
                    break;
                case 'k': // change key
                    int key = getRequestNumFromBuffer(self);    
                    setKey(&musicPlayer, key);
                    break;
                case 'o': // change BPM(tempo).
                    int bpm = getRequestNumFromBuffer(self); 
                    if (bpm < 60 || bpm > 240) print(&sci0, "Alert: the BPM ranges from 60 to 240. It has been modified to safe range.");
                    setBPM(&musicPlayer, bpm);
                    break;

                case 'y': // Calculate load WCET
                    print(&sci0, "Load WCET - Max: %dµs, Avg: %dµs\n",
                        (int)getMaximum(&timer2), (int)getAverage(&timer2));
                    break;
                case 't': // Calculate tone generator WCET
                    print(&sci0, "Tone WCET - Max: %dµs, Avg: %dµs\n",
                        (int)getMaximum(&timer1), (int)getAverage(&timer1));
                    break;
                case 'l': AFTER(USEC(1300), &backgroundLoad, load_bg, 0); break;  // Start background load
                case 's': stopLoad(&backgroundLoad, 1); break; // Stop background load
                case 'i': increaseLoad(&backgroundLoad); break;  
                case 'd': decreaseLoad(&backgroundLoad); break;
                case 'm': setMuted(&toneGenerator, !toneGenerator.muted); break;
                case 'v': {
                    int vol = getRequestNumFromBuffer(self);
                    setVolumn(&toneGenerator, vol);   // Set volume
                    break;
                }
                case 'g': AFTER(USEC(toneGenerator.period), &toneGenerator, playTone, 0); break;  // Generate 1kHz tone
                case 'D': {
                    int useDeadline = !toneGenerator.useDeadline;
                    setUseDeadline(&toneGenerator, useDeadline);
                    setUseDeadlineForLoad(&backgroundLoad, useDeadline);
                    break;
                }
                case 'K': {
                    int key = getRequestNumFromBuffer(self);   // Brother John periods
                    getPeriodsByChangingKey(&sci0, key);       // Get periods by changing key
                    break;
                }
                case 'q': // Quit to mode selection
                    self->currentMode = DEFAULT;
                    print(&sci0, "\n%s\n", modeInfo[DEFAULT].menuPrompt);
                    break;
                default:
                    threeHistory(&self->request, &storageForThreeHistory, c);
                    break;
            }
            break;
        
        case MUSICIAN:
            switch(c) {
                case 'q': // Quit to mode selection
                    self->currentMode = DEFAULT;
                    print(&sci0, "\n%s\n", modeInfo[DEFAULT].menuPrompt);
                    break;
                // TODO: Add musician mode commands
                default:
                    break;
            }
            break;
        
        default:
            break;
    }
}

// Function to handle serial input
void reader(App *self, int c) {
    SCI_WRITE(&sci0, "Rcv: ");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\n");

    if (self->currentMode == DEFAULT) {
        int modeValue = c - '0';
        enum Mode newMode = getModeByValue(modeValue);
        
        if (newMode != DEFAULT) {
            self->currentMode = newMode;
            print(&sci0, "\nEntered %s Mode\n%s\n",
                modeInfo[newMode].name,
                modeInfo[newMode].menuPrompt);
        } else {
            print(&sci0, "Invalid mode! Enter 1 or 2\n");
        }
    } else {
        dispatch(self, c);
    }
}

// Function to handle CAN messages
void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

// Function to start the application
void startApp(App *self, int arg) {
    initial();
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);

    print(&sci0, "System Boot\n%s\n", modeInfo[DEFAULT].menuPrompt);

	// startMeasure(self->timeMeasure, 1);
	
	// endMeasure(self->timeMeasure, 1);

    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
}

// Main function to install interrupts and start the TinyTimber framework
int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}