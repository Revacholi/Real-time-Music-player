#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "sioTinyTimber.h"

#include "Print.h"
#include "TimeMeasure.h"
#include "ToneGenerator.h"
#include "BackgroundLoad.h"
#include "MusicPlayer.h"
#include "Common.h"

// Enumeration for different modes
enum Mode { DEFAULT = 0, CONDUCTOR = 1, MUSICIAN = 2 };

// Structure to hold mode information
typedef struct {
    enum Mode mode;
    const char *name;
    const char *menuPrompt;
} ModeInfo;

typedef enum {
    MOMENTARY_MODE,
    HOLD_MODE
} ButtonMode;

typedef struct {
    Time lastPressTime;
    Time holdStartTime;
    ButtonMode currentMode;
    int debouncing;
    Msg holdCheckMsg; 
} ButtonState;

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
        "15. p - Play music\n"
        "16. k - Change key\n"
        "17. o - Change BPM\n"
    },
    [MUSICIAN] = {
        MUSICIAN,
        "Musician",
        "Musician Mode:\n"
        "Use CAN to control musicplayer\n"
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
    Request *request;
    enum Mode currentMode;  
    ButtonState btnState;
    TimeMeasure btnTimer;
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
void buttonCallback(App *self);

// Initialize storage for three-history
Storage storageForThreeHistory = { initObject(), {0}, 3, 0, 0, -9999, 9999, 0};

// Initialize application state
Request req = {{0}, 0};
App app = { initObject(), 0, 'X', &req, DEFAULT };

// Initialize SIO state
SysIO sio = initSysIO(SIO_PORT0, &app, buttonCallback);

// Initialize serial and CAN communication
Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

// Initialize timers and generators

TimeMeasure timer1 = { initTimer(), 0, 0, 0, {0.0}, 0, 0.0, 0.0, &sci0 };
TimeMeasure timer2 = { initTimer(), 0, 0, 0, {0.0}, 0, 0.0, 0.0, &sci0 };
ToneGenerator toneGenerator = {initObject(), 5, 1000, 500, 1, 1, 1, 0, 0, &sci0, &timer1};
BackgroundLoad backgroundLoad = {initObject(), 1000, 500, 1, 0, 0, &sci0, &timer2};
MusicPlayer musicPlayer = {initObject(), 120, 500, 50, 0, 0, 0, &sci0, &toneGenerator};

#define DEBOUNCE_TIME 100 
#define HOLD_TIME     1000 

void checkHoldMode(App *self, int arg) {
    if (SIO_READ(&sio) == 0) { 
        self->btnState.currentMode = HOLD_MODE;
        print(&sci0, "Entered press-and-hold mode\n");
    }
}

Time odd_press = 0;
Time even_press = 0;
Time diff_press = 0;
int callFlag = 1;
void buttonCallback(App *self) {
    if (callFlag)
        odd_press = CURRENT_OFFSET();
    else{
        even_press = CURRENT_OFFSET();
        diff_press = even_press - odd_press;
        if (diff_press < 0) diff_press = -diff_press;
        print(&sci0, "diff_press: %ld\n", USEC_OF(diff_press));
    }
    callFlag = !callFlag;
}

// Function to calculate frequency based on key
int getFrequency(int key) {
    double frequency = 440 * pow(2, key / 12.0);
    return (int)(1e6 / (2 * frequency));
}

// Function to initialize frequencies
void initial() {
    for(int i = -10; i <= 14; i++) {
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
    self->request->inputBuffer[self->request->inputBufferIndex] = '\0';
    int num = atoi(self->request->inputBuffer);
    self->request->inputBufferIndex = 0;
    return num;
}

int8_t flag = 0;
// Function to dispatch commands based on current mode
void dispatch(App *self, int c, int num) {
    switch(c) {
        case 'S': // stop music
            SEND((Time) 0, (Time) 0, &musicPlayer, stopPlay, 0);
            flag = 0;
            break;
        case 'p': // play music
            if(flag) {
                SEND((Time) 0, (Time) 0, &musicPlayer, initIndex, 0);
            } else {
                SEND((Time) 0, (Time) 0, &musicPlayer, startPlay, 0);
                flag = 1;
            }
            break;
        case 'k': // change key
            //int key = getRequestNumFromBuffer(self);   
            int key = num; 
            if (key < -5 || key > 5) print(&sci0, "Alert: the key ranges from -5 to 5. It has been modified to safe range.");
            SYNC(&musicPlayer, setKey, key);
            break;
        case 'o': // change BPM(tempo).
            int bpm = num;
            if (bpm < 60 || bpm > 240) print(&sci0, "Alert: the BPM ranges from 60 to 240. It has been modified to safe range.");
            SYNC(&musicPlayer, setBPM, bpm);
            break;
        case 'y': // Calculate load WCET
            print(&sci0, "Load WCET - Max: %dµs, Avg: %dµs\n",
                (int)SYNC(&timer2, getMaximum, 0), 
                (int)SYNC(&timer2, getAverage, 0));
            break;
        case 't': // Calculate tone generator WCET
            print(&sci0, "Tone WCET - Max: %dµs, Avg: %dµs\n",
                (int)SYNC(&timer1, getMaximum, 0), 
                (int)SYNC(&timer1, getAverage, 0));
            break;
        case 'l': AFTER(USEC(1300), &backgroundLoad, load_bg, 0); break;  // Start background load
        case 's': SYNC(&backgroundLoad, stopLoad, 1); break; // Stop background load
        case 'i': SYNC(&backgroundLoad, increaseLoad, 0); break;  
        case 'd': SYNC(&backgroundLoad, decreaseLoad, 0); break;
        case 'm': 
            int currentMuted = SYNC(&toneGenerator, getMutedByUser, 0);
            SYNC(&toneGenerator, setUserMuted, !currentMuted);
            break;
        case 'v': {
            int vol = num;
            SYNC(&toneGenerator, setVolumn, vol);   // Set volume
            break;
        }
        case 'g': 
            int period = SYNC(&toneGenerator, getPeriod, 0);
            AFTER(USEC(period), &toneGenerator, playTone, 0);
            break;  // Generate 1kHz tone
        case 'D': {
            int currentDeadline = SYNC(&toneGenerator, getUseDeadline, 0);
            SYNC(&toneGenerator, setUseDeadline, !currentDeadline);
            SYNC(&backgroundLoad, setUseDeadlineForLoad, !currentDeadline);
            break;
        }
        case 'K': {
            int key = num;   // Brother John periods
            getPeriodsByChangingKey(&sci0, key);       // Get periods by changing key
            break;
        }
        case 'q': // Quit to mode selection
            self->currentMode = DEFAULT;
            print(&sci0, "\n%s\n", modeInfo[DEFAULT].menuPrompt);
            break;
        default:
            threeHistory(self->request, &storageForThreeHistory, c);
            break;
    }
}

void sendCANMessage(App *self, int can, char c) {
    CANMsg msg;
    msg.msgId = 1;
    msg.nodeId = 1;
    int length = 0;

    if (can != INT_MIN) {
        // why 7? because the length of a buffer in CAN message is just 8 bytes;
        for (int i = 0; i < 7; i++) {
            if (self->request->inputBuffer[i] == '\0') {
                break;
            }
            msg.buff[i] = self->request->inputBuffer[i];
            length++;
        }
    }

    msg.buff[length] = c;
    msg.buff[length+1] = '\0';
    length = length + 2;
    msg.length = length;

    //print(&sci0, "%s", msg.buff);

    CAN_SEND(&can0, &msg);
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
    } else if (self->currentMode == CONDUCTOR) {
        // get whole command.
        int num = INT_MIN;
        if (c == '-' || (c <= '9' && c>= '0')) {
            self->request->inputBuffer[self->request->inputBufferIndex++] = c;
        } else {
            num = getRequestNumFromBuffer(self);
            dispatch(self, c, num);
            sendCANMessage(self, num, c);
        }

    } else if (self->currentMode == MUSICIAN) {
        // get command from keyboard but not directly control. send CAN msg.
        int num = INT_MIN;
        if (c == '-' || (c <= '9' && c>= '0')) {
            self->request->inputBuffer[self->request->inputBufferIndex++] = c;
        } else {
            if(c == 'k' || c == 'o' || c == 'v' || c == 'K') {
                num = getRequestNumFromBuffer(self);
                sendCANMessage(self, num, c);
            } else {
                sendCANMessage(self, num, c);
            }
            
        }

    }
}

// Function to handle CAN messages
void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);

    // get command from can message.
    
    int num = INT_MIN;
    char c = ' ';
    if (msg.length != 0) {
        if (msg.length == 2) {
            c = msg.buff[0];
        } else {
            c = msg.buff[msg.length - 2];

            char msgBuff[8] = {};
            for (int i = 0; i < msg.length - 2; i++) {
                msgBuff[i] = msg.buff[i];
            }
            msgBuff[msg.length - 2] = '\0';
            num = atoi(msgBuff);
        }
    }

    // print(&sci0, "Can msg command: %c\n", c);
    // print(&sci0, "Can msg num: %d\n", num);
    // print(&sci0, "Can msg received: %s\n", msg.buff);

    // only musician mode can be controlled.
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
    } else if (self->currentMode == MUSICIAN) {
        // need to control.
        dispatch(self, c, num);
    }
}

// Function to start the application
void startApp(App *self, int arg) {
    initial();
    // CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);

    SIO_INIT(&sio);
    SIO_TRIG(&sio, 0);

    print(&sci0, "System Boot\n%s\n", modeInfo[DEFAULT].menuPrompt);

    // msg.msgId = 1;
    // msg.nodeId = 1;
    // msg.length = 6;
    // msg.buff[0] = 'H';
    // msg.buff[1] = 'e';
    // msg.buff[2] = 'l';
    // msg.buff[3] = 'l';
    // msg.buff[4] = 'o';
    // msg.buff[5] = 0;
    // CAN_SEND(&can0, &msg);
}

// Main function to install interrupts and start the TinyTimber framework
int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    INSTALL(&sio, sio_interrupt, SIO_IRQ0);

    TINYTIMBER(&app, startApp, 0);
    return 0;
}