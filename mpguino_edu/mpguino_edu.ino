
//mpguino, open source fuel consumption system
//GPL Software, mass production use rights reserved by opengauge.org, personal use is perfectly fine , no warranties expressed or implied

//Special thanks to the good folks at ecomodder.com, ardunio.cc, avrfreaks.net, cadsoft.de, atmel.com,
//and all the folks who donate their time and resources and share their experiences freely

/* External connections:
 
 Vehicle interface pins
 injector open D2 (int0)
 injector closed D3 (int1)
 speed C0 (pcint8)
 
 LCD Pins
 DI  D4
 DB4 D7
 DB5 B0
 DB6 B4
 DB7 B5
 Enable C5
 Contrast D6, controlled by PWM on OC0A
 Brightness B1, controlled by PWM on OC1A
 
 Buttons
 left C3 (pcint11)
 middle  C4 (pcint12)
 right C5 (pcint13)
 
 */

/* Program overview (easier said than done)
 set up digital pins to drive the lcd
 set up pwm pins for lcd brightness and contrast
 set up interrupts for the buttons, the speed signal, and the injector high/low signals.
 set up tx pin for transmitting values over uart
 
 create accumulators for speed/injector data
 
 mainloop{
 incorporate the accumulators into longer storage trips, reset accumulators
 display computations, transmit accumulators
 scan for key presses and perform their function (change screen,reset a trip,goto setup)
 pause for remainder of 1/2 second
 }
 
 */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

///
/// @file	mpguino_edu.ino
/// @brief	Main sketch
/// @details	<#details#>
/// @n @a	Developed with [embedXcode](http://embedXcode.weebly.com )
///
/// @author	Eduardo Rocha Costa
/// @author	Eduardo Rocha Costa
/// @date	28/06/13 00:37
/// @version	VERSION
///
/// @copyright	© Eduardo Rocha Costa, 2013
/// @copyright	CC = BY NC SA
///
/// @see	ReadMe.txt for references
/// @n
///

#include "Arduino.h"
#define tankPin   2
#define voltagePin   1

#define VER 88
#define VERSION " MPGuino  v0." STR(VER1) "E"
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
typedef uint8_t boolean;
typedef uint8_t byte;
#define RISING 3
#define FALLING 2
#define loopsPerSecond 2 // how many times will we try and loop in a second
/*
 #define SAVE_TANK
 template <class T> int EEPROM_writeAnything(int ee, const T& value)
 {
 const byte* p = (const byte*)(const void*)&value;
 unsigned int i;
 for (i = 0; i < sizeof(value); i++)
 EEPROM.write(ee++, *p++);
 return i;
 }
 
 template <class T> int EEPROM_readAnything(int ee, T& value)
 {
 byte* p = (byte*)(void*)&value;
 unsigned int i;
 for (i = 0; i < sizeof(value); i++)
 *p++ = EEPROM.read(ee++);
 return i;
 }
 
 */
//use with 20mhz
/*#define cyclesperhour 4500
 #define dispadj 800
 #define dispadj2 1250
 #define looptime 1250000ul/loopsPerSecond //1/2 second
 #define myubbr (20000000/16/9600-1)
 #define injhold (parms[injectorSettleTimeIdx]*5)/4
 */

#define outhi(port,pin) PORT##port |= ( 1 << P##port##pin )
#define outlo(port,pin) PORT##port &= ~( 1 << P##port##pin )
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

//use with 16mhz, not tested
#define cyclesperhour 3600
//#define dispadj 1000 //!!CRAP
#define dispadj2 1250
#define looptime 1000000ul/loopsPerSecond //1/2 second
#define myubbr (16000000/16/9600-1)
#define injhold parms[injectorSettleTimeIdx]


//failsafe !!!
//#define usedefaults true
void displayTripDebug(char * lm1, unsigned long v1, char * lm2, unsigned long v2,
                      char * lm3, unsigned long v3, char * lm4, unsigned long v4,
                      char * lm5, unsigned long v5, char * lm6, unsigned long v6,
                      char * lm7, unsigned long v7, char * lm8, unsigned long v8);
unsigned long instantrpm(void);
unsigned long readTemp();
unsigned long readVcc();
unsigned long batteryVoltage(void);
unsigned long pinVoltage(void);
unsigned long pinVoltage1(void);
unsigned long batteryVoltageVcc(void);
unsigned long tankLiters(void);
unsigned long tankLiters_norm(void);
void enableLButton();
void enableMButton();
void enableRButton();
void addEvent(byte eventID, unsigned int ms);
unsigned long microSeconds(void);
unsigned long elapsedMicroseconds(unsigned long startMicroSeconds,
                                  unsigned long currentMicroseconds);
unsigned long elapsedMicroseconds(unsigned long startMicroSeconds);
void processInjOpen(void);
void processInjClosed(void);
void enableVSS();
void setup(void);
void mainloop(void);
char* format(unsigned long num);
char * getStr(prog_char * str);
void doDisplayCustom();
void doDisplayCustom0();
void doDisplayEOCIdleData();
void doDisplayInstantCurrent();
void doDisplayInstantTank();
void doDisplayBigInstant();
void doDisplayBigCurrent();
void doDisplayBigTank();
void doDisplayCurrentTripData(void);
void doDisplayTankTripData(void);
void doDisplaySystemInfo(void);
void displayTripCombo(char *lu1, char * lm1, unsigned long v1, char * lu2, char * lm2, unsigned long v2,
                      char * lu3, char * lm3, unsigned long v3, char * lu4, char * lm4, unsigned long v4);
void displayTripCombo2(char *lu1, char * lm1, unsigned long v1, char * lu2, char * lm2, unsigned long v2,
                       char * lu3, char * lm3, unsigned long v3, char * lu4, char * lm4, unsigned long v4,
                       char *lu5, char * lm5, unsigned long v5, char * lu6, char * lm6, unsigned long v6,
                       char * lu7, char * lm7, unsigned long v7, char * lu8, char * lm8, unsigned long v8);
void displayTripCombo3(char *lu1, unsigned long v1, char * lu2,
                       char * lu3, char * lm3, unsigned long v3, char * lu4, char * lm4, unsigned long v4,
                       char *lu5, char * lm5, unsigned long v5, char * lu6, char * lm6, unsigned long v6,
                       char * lu7, unsigned long v7, char * lu8);

void tDisplay(void * r);
int memoryTest();
unsigned long instantmph();
unsigned long instantmpg();
unsigned long instantgph();
void bigNum(unsigned long t, char * txt1, const char * txt2);
void init64(unsigned long an[], unsigned long bigPart,
            unsigned long littlePart);
void shl64(unsigned long an[]);
void shr64(unsigned long an[]);
void add64(unsigned long an[], unsigned long ann[]);
void sub64(unsigned long an[], unsigned long ann[]);
boolean eq64(unsigned long an[], unsigned long ann[]);
boolean lt64(unsigned long an[], unsigned long ann[]);
void div64(unsigned long num[], unsigned long den[]);
void mul64(unsigned long an[], unsigned long ann[]);
void save();
byte load();
char * uformat(unsigned long val);
unsigned long rformat(char * val);
void editParm(byte parmIdx);
void initGuino();
unsigned long millis2();
void delay2(unsigned long ms);
void delayMicroseconds2(unsigned int us);
unsigned long readVcc(void);

void simpletx(char * string);
unsigned long parms[] = {
    30ul, 8204ul, 341695467ul,
    8ul, 420000000ul, //11888ul,
    500ul, //2400ul, 0ul,
    2ul, 0ul, 2ul };//default values
char * parmLabels[] = {
    "Contrast", "VSS Pulses/Mile", "MicroSec/Gallon",
    "Pulses/2 revs", "Timout(microSec)",      // "Tank Gal * 1000",
    "Injector DelayuS", //"Weight (lbs)", "Scratchpad(odo?)",
    "VSS Delay ms",  "InjTrg 0-Dn 1-Up", "Metric (1=yes)" };
//byte brightness[]={  255,214,171,128}; //middle button cycles through these brightness settings
byte brightness[] = {
    0, 100, 128, 255 }; //middle button cycles through these brightness settings
#define brightnessLength (sizeof(brightness)/sizeof(byte)) //array size
byte brightnessIdx = 1;



#define contrastIdx 0  //do contrast first to get display dialed in
#define vssPulsesPerMileIdx 1           //usado
volatile unsigned long distancefactor;

#define microSecondsPerGallonIdx 2      //usado
volatile unsigned long fuelfactor;

#define injPulsesPer2Revolutions 3      //usado só no rpm
#define currentTripResetTimeoutUSIdx 4  //sleep

//#define tankSizeIdx 5                   //nao usado

#define injectorSettleTimeIdx 5         //usado

//#define weightIdx 7                     // nao usado
//#define scratchpadIdx 8                 // nao usado

#define vsspauseIdx 6                   //usado
#define injEdgeIdx 7                   //usado
#define metricIdx 8                    //usado
#define parmsLength (sizeof(parms)/sizeof(unsigned long)) /*  //array size      */

unsigned long injectorSettleTime;

#define nil 3999999999ul

#define guinosigold 0b10100101
#define guinosig    0b11100111

#define vssBit ( 1 << 0 )
#define lbuttonBit ( 1 << 3 )
#define mbuttonBit ( 1 << 4 )
#define rbuttonBit ( 1 << 5 )

typedef void (* pFunc)(void);//type for display function pointers

volatile unsigned long timer2_overflow_count;

/*** Set up the Events ***
 * We have our own ISR for timer2 which gets called about once a millisecond.
 * So we define certain event functions that we can schedule by calling addEvent
 * with the event ID and the number of milliseconds to wait before calling the event.
 * The milliseconds is approximate.
 *
 * Keep the event functions SMALL!!!  This is an interrupt!
 *
 */
//event functions

void enableLButton() {
    PCMSK1 |= (1 << PCINT11);
}
void enableMButton() {
    PCMSK1 |= (1 << PCINT12);
}
void enableRButton() {
    PCMSK1 |= (1 << PCINT13);
}
//array of the event functions
pFunc eventFuncs[] = {
    enableVSS, enableLButton, enableMButton, enableRButton };
#define eventFuncSize (sizeof(eventFuncs)/sizeof(pFunc))
//define the event IDs
#define enableVSSID 0
#define enableLButtonID 1
#define enableMButtonID 2
#define enableRButtonID 3
//ms counters
unsigned int eventFuncCounts[eventFuncSize];

//schedule an event to occur ms milliseconds from now
void addEvent(byte eventID, unsigned int ms) {
    if (ms == 0)
        eventFuncs[eventID]();
    else
        eventFuncCounts[eventID] = ms;
}

/* this ISR gets called every 1.024 milliseconds, we will call that a millisecond for our purposes
 go through all the event counts,
 if any are non zero subtract 1 and call the associated function if it just turned zero.  */
ISR(TIMER2_OVF_vect)
{
    timer2_overflow_count++;
    for (byte eventID = 0; eventID < eventFuncSize; eventID++) {
        if (eventFuncCounts[eventID] != 0) {
            eventFuncCounts[eventID]--;
            if (eventFuncCounts[eventID] == 0)
                eventFuncs[eventID]();
        }
    }
}

unsigned long maxLoopLength = 0; //see if we are overutilizing the CPU


#define buttonsUp   lbuttonBit + mbuttonBit + rbuttonBit  // start with the buttons in the right state
byte buttonState = buttonsUp;

//overflow counter used by millis2()
unsigned long lastMicroSeconds = millis2() * 1000;
unsigned long microSeconds(void) {
    unsigned long tmp_timer2_overflow_count;
    unsigned long tmp;
    byte tmp_tcnt2;
    cli();  //disable interrupts
    tmp_timer2_overflow_count = timer2_overflow_count;
    tmp_tcnt2 = TCNT2;
    sei();  // enable interrupts
    tmp = ((tmp_timer2_overflow_count << 8) + tmp_tcnt2) * 4;
    if ((tmp <= lastMicroSeconds) && (lastMicroSeconds < 4290560000ul))
        return microSeconds();
    lastMicroSeconds = tmp;
    return tmp;
}

unsigned long elapsedMicroseconds(unsigned long startMicroSeconds,unsigned long currentMicroseconds) {
    if (currentMicroseconds >= startMicroSeconds)
        return currentMicroseconds - startMicroSeconds;
    return 4294967295 - (startMicroSeconds - currentMicroseconds);
}

unsigned long elapsedMicroseconds(unsigned long startMicroSeconds) {
    return elapsedMicroseconds(startMicroSeconds, microSeconds());
}

//Trip prototype
class Trip {
public:
    unsigned long loopCount; //how long has this trip been running
    unsigned long injPulses; //rpm
    unsigned long injHiSec;// seconds the injector has been open
    unsigned long injHius;// microseconds, fractional part of the injectors open
    unsigned long injIdleHiSec;// seconds the injector has been open
    unsigned long injIdleHius;// microseconds, fractional part of the injectors open
    unsigned long vssPulses;//from the speedo
    unsigned long vssEOCPulses;//from the speedo
    unsigned long vssPulseLength; // only used by instant
    //these functions actually return in thousandths,
    unsigned long miles();
    unsigned long gallons();
    unsigned long lkm();
    unsigned long mpg();
    unsigned long mph();
    unsigned long time(); //mmm.ss
    unsigned long eocMiles(); //how many "free" miles?
    unsigned long idleGallons(); //how many gallons spent at 0 mph?
    void update(Trip t);
    void reset();
//    void save();
//    void load();
    Trip();
};

//LCD prototype
namespace LCD {
    void gotoXY(byte x, byte y);
    void print(char * string);
    void init();
    void tickleEnable();
    void cmdWriteSet();
    void LcdCommandWrite(byte value);
    void LcdDataWrite(byte value);
    byte pushNibble(byte value);
}
;

//main objects we will be working with:
unsigned long injHiStart; //for timing injector pulses
Trip tmpTrip;
Trip instant;
Trip current;
Trip tank;

unsigned volatile long instInjStart = nil;
unsigned volatile long tmpInstInjStart = nil;
unsigned volatile long instInjEnd;
unsigned volatile long tmpInstInjEnd;
unsigned volatile long instInjTot;
unsigned volatile long tmpInstInjTot;
unsigned volatile long instInjCount;
unsigned volatile long tmpInstInjCount;

volatile static pFunc int0Func;
ISR(INT0_vect)
{ //processInjOpen by default
    int0Func();
}

volatile static pFunc int1Func;
ISR(INT1_vect)
{//processInjClosed
    int1Func();
}

void processInjOpen(void) {
    injHiStart = microSeconds();
}

void processInjClosed(void) {
    long t = microSeconds();
    long x = elapsedMicroseconds(injHiStart, t) - injectorSettleTime;
    if (x > 0)
        tmpTrip.injHius += x; // quantos milisegundos o injetor ficou aberto
    tmpTrip.injPulses++;
    
    if (tmpInstInjStart != nil) { //se nao for a primeira vez
        if (x > 0){
            tmpInstInjTot += x;
            tmpInstInjCount++;
        }
    }
    else {                      //na primeira vez inicializa
        tmpInstInjStart = t;
    }
    
    tmpInstInjEnd = t;
}

volatile boolean vssFlop = 0;

void enableVSS() {
    //    tmpTrip.vssPulses++;
    vssFlop = !vssFlop;
}

unsigned volatile long lastVSS1;
unsigned volatile long lastVSSTime;
unsigned volatile long lastVSS2;

volatile boolean lastVssFlop = vssFlop;

//attach the vss/buttons interrupt
ISR( PCINT1_vect )
{
    static byte vsspinstate = 0;
    byte p = PINC;//bypassing digitalRead for interrupt performance
    if ((p & vssBit) != (vsspinstate & vssBit)) {
        addEvent(enableVSSID, parms[vsspauseIdx]); //check back in a couple milli
    }
    if (lastVssFlop != vssFlop) {
        lastVSS1 = lastVSS2;
        unsigned long t = microSeconds();
        lastVSS2 = elapsedMicroseconds(lastVSSTime, t);
        lastVSSTime = t;
        tmpTrip.vssPulses++;
        tmpTrip.vssPulseLength += lastVSS2;
        lastVssFlop = vssFlop;
    }
    vsspinstate = p;
    buttonState &= p;
}

pFunc displayFuncs[] = {
    
    //doDisplayDebug1,doDisplayDebug2,
    doDisplayCustom0,
    doDisplayCustom,
    doDisplayInstantCurrent,
    doDisplayInstantTank,
    doDisplayBigInstant,
    doDisplayBigCurrent,
    doDisplayBigTank,
    doDisplayCurrentTripData,
    doDisplayTankTripData,
    doDisplayEOCIdleData,
    doDisplaySystemInfo, };

#define displayFuncSize (sizeof(displayFuncs)/sizeof(pFunc)) //array size
prog_char * displayFuncNames[displayFuncSize];
byte newRun = 0;
void setup(void) {
    
    newRun = load();//load the default parameters
    byte x = 0;
    displayFuncNames[x++] = PSTR("Custom  ");
    //displayFuncNames[x++] = PSTR("Debug1  ");
    //displayFuncNames[x++] = PSTR("Debug2  ");
    displayFuncNames[x++] = PSTR("Instant/Current ");
    displayFuncNames[x++] = PSTR("Instant/Tank ");
    displayFuncNames[x++] = PSTR("BIG Instant ");
    displayFuncNames[x++] = PSTR("BIG Current ");
    displayFuncNames[x++] = PSTR("BIG Tank ");
    displayFuncNames[x++] = PSTR("Current ");
    displayFuncNames[x++] = PSTR("Tank ");
    displayFuncNames[x++] = PSTR("EOC/Idle ");
    displayFuncNames[x++] = PSTR("CPU Monitor ");
    
    //  analogWrite(BrightnessPin,brightness[brightnessIdx]);
    sbi(TCCR1A, COM1A1); //brightness pwm enable
    
    OCR1A = brightness[brightnessIdx];
    
    DDRB = (1 << DDB5) | (1 << DDB4) | (1 << DDB1) | (1 << DDB0);
    DDRD = (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD4);
    
    delay2(200);
    
    //  analogWrite(ContrastPin,parms[contrastIdx]);
    sbi(TCCR0A, COM0A1);//contrast pwm enable
    OCR0A = parms[contrastIdx];
    
    LCD::init();
    LCD::LcdCommandWrite(0b00000001); // clear display, set cursor position to zero
    LCD::LcdCommandWrite(0b10000); // set dram to zero
    LCD::gotoXY(0, 0);
    LCD::print("               ");
    LCD::gotoXY(0, 1);
    LCD::print(getStr(PSTR("OpenGauge       ")));
    LCD::gotoXY(0, 2);
    LCD::print(getStr(PSTR(VERSION)));
    LCD::gotoXY(0, 3);
    LCD::print("               ");
    
    injectorSettleTime = injhold;
    int0Func = processInjOpen;
    int1Func = processInjClosed;
    
    //set up the external interrupts
    EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01)))
    | ((parms[injEdgeIdx] == 1 ? RISING : FALLING) << ISC00);
    EIMSK |= (1 << INT0);
    EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11)))
    | ((parms[injEdgeIdx] == 1 ? FALLING : RISING) << ISC10);
    EIMSK |= (1 << INT1);
    
    PORTC |= (1 << 5) | (1 << 4) | (1 << 3); //button pullup resistors
    
    distancefactor = parms[vssPulsesPerMileIdx];
    fuelfactor = parms[microSecondsPerGallonIdx];
    
    if ((parms[metricIdx] == 1)||(parms[metricIdx] == 2)){
        /*
         init64(tmp1, 0, 1000000000ul);
         init64(tmp2, 0, distancefactor);
         div64(tmp1, tmp2);
         voltar aqui!!!!!
         */
        distancefactor /= 1.609;
        fuelfactor /= 3.785;
    }
    
    //low level interrupt enable stuff
    PCMSK1 |= (1 << PCINT8);
    enableLButton();
    enableMButton();
    enableRButton();
    PCICR |= (1 << PCIE1);
    
    delay2(1500);
}

byte screen = 0;
byte holdDisplay = 0;

void mainloop(void) {
    // TODO: Mainloop
    if (newRun != 1)
        initGuino();//go through the initialization screen
    /*
#ifdef SAVE_TANK
    else
        tank.load();
#endif
    */
    unsigned long lastActivity = microSeconds();
    unsigned long tankHold; //state at point of last activity
    while (true) {
        unsigned long loopStart = microSeconds();
        instant.reset(); //clear instant
        cli();
        instant.update(tmpTrip); //"copy" of tmpTrip in instant now
        tmpTrip.reset(); //reset tmpTrip first so we don't lose too many interrupts
        instInjStart = tmpInstInjStart;
        instInjEnd = tmpInstInjEnd;
        instInjTot = tmpInstInjTot;
        instInjCount = tmpInstInjCount;
        
        tmpInstInjStart = nil;
        tmpInstInjEnd = nil;
        tmpInstInjTot = 0;
        tmpInstInjCount = 0;
        
        sei();
        
        //send out instantmpg * 1000, instantmph * 1000, the injector/vss raw data
        simpletx(format(instantmpg()));
        simpletx(",");
        simpletx(format(instantmph()));
        simpletx(",");
        simpletx(format(instant.injHius * 1000));
        simpletx(",");
        simpletx(format(instant.injPulses * 1000));
        simpletx(",");
        simpletx(format(instant.vssPulses * 1000));
        simpletx("\n");
        
        current.update(instant); //use instant to update current
        tank.update(instant); //use instant to update tank
        
        //currentTripResetTimeoutUS
        if (instant.vssPulses == 0 && instant.injPulses == 0 && holdDisplay == 0) {
            if (elapsedMicroseconds(lastActivity) > parms[currentTripResetTimeoutUSIdx] && lastActivity!= nil) {
                LCD::gotoXY(0, 0);
                LCD::print("                ");
                LCD::gotoXY(0, 1);
                LCD::print(" Going to sleep ");
                LCD::gotoXY(0, 2);
                LCD::print("                ");
                LCD::gotoXY(0, 3);
                LCD::print("                ");
                //        analogWrite(BrightnessPin,brightness[0]);    //nitey night
                OCR1A = brightness[0];
                /*
#ifdef SAVE_TANK
                tank.save();
#endif
                 */
                lastActivity = nil;
            }
        }
        else {
            if (lastActivity == nil) {//wake up!!!
                //@teste: PWM no pino OCR1A
                OCR1A = brightness[brightnessIdx];
                //        analogWrite(BrightnessPin,brightness[brightnessIdx]);
                lastActivity = loopStart;
                //current.reset(); // TODO: Pra que isso ????
                tank.loopCount = tankHold;
                current.update(instant);
                tank.update(instant);
            }
            else {
                lastActivity = loopStart;
                tankHold = tank.loopCount;
            }
        }
        
        if (holdDisplay == 0) {
            displayFuncs[screen](); //call the appropriate display routine
            LCD::gotoXY(0, 0);
            
            //see if any buttons were pressed, display a brief message if so
            if (!(buttonState & lbuttonBit) && !(buttonState & rbuttonBit)) {// left and right = initialize
                LCD::print(getStr(PSTR("Setup ")));
                initGuino();
                //}else if(!(buttonState&lbuttonBit) && !(buttonState&rbuttonBit)){// left and right = run lcd init = tank reset
                //    LCD::print(getStr(PSTR("Init LCD ")));
                //    LCD::init();
            }
            else if (!(buttonState & lbuttonBit) && !(buttonState&mbuttonBit)) {// left and middle = tank reset
                tank.reset();
                LCD::print(getStr(PSTR("Tank Reset ")));
            }
            else if (!(buttonState & mbuttonBit) && !(buttonState&rbuttonBit)) {// right and middle = current reset
                current.reset();
                LCD::print(getStr(PSTR("Current Reset ")));
                /*
#ifdef SAVE_TANK
                tank.save();
#endif
                 */
            }
            else if (!(buttonState & lbuttonBit)) { //left is rotate through screeens to the left
                if (screen != 0)
                    screen = (screen - 1);
                else
                    screen = displayFuncSize - 1;
                LCD::print(getStr(displayFuncNames[screen]));
            }
            else if (!(buttonState & mbuttonBit)) { //middle is cycle through brightness settings
                brightnessIdx = (brightnessIdx +1) % brightnessLength;
                OCR1A = brightness[brightnessIdx];
                //        analogWrite(BrightnessPin,brightness[brightnessIdx]);
                LCD::print(getStr(PSTR("Brightness ")));
                LCD::LcdDataWrite('0' + brightnessIdx);
                LCD::print(" ");
            }
            else if (!(buttonState & rbuttonBit)) {//right is rotate through screeens to the left
                screen = (screen + 1) % displayFuncSize;
                LCD::print(getStr(displayFuncNames[screen]));
            }
            if (buttonState != buttonsUp)
                holdDisplay = 1;
        }
        else {
            holdDisplay = 0;
        }
        buttonState = buttonsUp;//reset the buttons
        
        //keep track of how long the loops take before we go int waiting.
        unsigned long loopX = elapsedMicroseconds(loopStart);
        if (loopX > maxLoopLength)
            maxLoopLength = loopX;
        
        while (elapsedMicroseconds(loopStart) < (looptime))
            ;//wait for the end of a second to arrive
    }
    
}

char fBuff[7];//used by format

void dispv(char * usl, char * ml, unsigned long num) {
    LCD::print(((parms[metricIdx]==1)||(parms[metricIdx]==2))?ml:usl);
    LCD::print(format(num));
}
void disp(char * ml, unsigned long num) {
    LCD::print(ml);
    LCD::print(format(num));
}

char* format(unsigned long num) {
    byte dp = 3;
    
    while (num > 999999) {
        num /= 10;
        dp++;
        if (dp == 5)
            break; // We'll lose the top numbers like an odometer
    }
    if (dp == 5)
        dp = 99; // We don't need a decimal point here.
    
    // Round off the non-printed value.
    if ((num % 10) > 4)
        num += 10;
    num /= 10;
    byte x = 6;
    while (x > 0) {
        x--;
        if (x == dp) { //time to poke in the decimal point?{
            fBuff[x] = '.';
        }
        else {
            fBuff[x] = '0' + (num % 10);//poke the ascii character for the digit.
            num /= 10;
        }
    }
    fBuff[6] = 0;
    
    return fBuff;
}
//get a string from flash
char mBuff[17];//used by getStr
char * getStr(prog_char * str) {
    strcpy_P(mBuff, str);
    return mBuff;
}

/*void doDisplayCustom() {
 displayTripCombo("MG","LK", instantmpg(), " S", " S", instantmph(), "GH","LH",
 instantgph(), " C"," C", current.mpg());
 }*/
void doDisplayCustom0(){
    displayTripCombo2(
                      "VT","VT",readVcc(),   "BT","BT",batteryVoltageVcc(),
                      "LT","LT",tankLiters(),"TP","TP", batteryVoltage(),
                      "AR","AR",pinVoltage()," R"," P",instantrpm(),
                      "A1","A1",pinVoltage(),"LT ","LT ",tankLiters_norm())
    ;
}


//void doDisplayCustom(){displayTripCombo("LT","LT",tankLiters(),"SP","SP",instantmph()," R"," P",instantrpm()," C"," C",current.injIdleHiSec*1000);}
void doDisplayCustom(){
    displayTripCombo2(
                      "LT","LT",tankLiters(),   "BT","BT",batteryVoltage(),
                      "D ","D ",current.miles(),"SP","SP",instantmph(),
                      "IM","IL", instantmpg(),  "TM","TL",tank.mpg(),
                      " R"," P",instantrpm(),   " C"," C",instant.injPulses*1000*parms[injPulsesPer2Revolutions])
    ;
}



void doDisplayDebug1(){
    displayTripDebug(
                     "LC",instant.loopCount,    "iP",instant.injPulses,
                     "iS",instant.injHiSec,     "iu",instant.injHius,
                     "id", instant.injIdleHiSec,"id",instant.injIdleHius,
                     "vP",instant.vssPulses,    "vL",instant.vssPulseLength)
    ;
}


void doDisplayDebug2(){
    displayTripDebug(
                     "mg",instantmpg(),             "mh",instantmph(),
                     "iS",instant.injHiSec,         "iu",instant.injHius*1000,
                     "iP",instant.injPulses*1000,   "vP",instant.vssPulses*1000,
                     "id", instant.injIdleHiSec,    "id",instant.injIdleHius*10000)
    ;
}
//void doDisplayCustom(){displayTripCombo('I','M',995,'S',994,'R','P',999994,'C',999995);}


void doDisplayEOCIdleData() {
    displayTripCombo("CE","CE", current.eocMiles(), " G"," L", current.idleGallons(),
                     "TE","TE",    tank.eocMiles(), " G"," L", tank.idleGallons());
}
void doDisplayInstantCurrent() {
    displayTripCombo3("LT",tankLiters_norm(),  "       ",
                      "IM","IL", instantmpg(),      " S"," S", instantmph(),
                      "CM","CL", current.mpg(),     " D"," D", current.miles(),
                      "TL", tank.gallons(),    "       "
                      );
}

void doDisplayInstantTank() {
    displayTripCombo("TM","TL", tank.gallons(), "TT","TT", tank.time(),
                     "MK","KL",     tank.mpg(), " D"," D", tank.miles());
}

void doDisplayBigInstant() {
    if (parms[metricIdx]==1)
        bigNum(instantmpg(), "INST","L/K ");
    else if (parms[metricIdx]==2)
        bigNum(instantmpg(), "INST","K/L ");
    else
        bigNum(instantmpg(), "INST","MPG ");
}

void doDisplayBigCurrent() {
    if (parms[metricIdx]==1)
        bigNum(current.mpg(), "CURR","L/K ");
    else if (parms[metricIdx]==2)
        bigNum(current.mpg(), "CURR","K/L ");
    else
        bigNum(current.mpg(), "CURR","MPG ");
}
void doDisplayBigTank() {
    if (parms[metricIdx]==1)
        bigNum(tank.mpg(), "TANK","L/K ");
    else if (parms[metricIdx]==2)
        bigNum(tank.mpg(), "TANK","K/L ");
    else
        bigNum(tank.mpg(), "TANK","MPG ");
}


void doDisplayCurrentTripData(void) {
    tDisplay(&current);
} //display current trip formatted data.
void doDisplayTankTripData(void) {
    tDisplay(&tank);
} //display tank trip formatted data.



void doDisplaySystemInfo(void) {
    LCD::gotoXY(0, 0);
    LCD::print("C%");
    LCD::print(format(maxLoopLength * 1000 / (looptime / 100)));
    LCD::print(" T");
    LCD::print(format(tank.time()));
    unsigned long mem = memoryTest();
    mem *= 1000;
    LCD::gotoXY(0, 1);
    LCD::print("FREE MEM:");
    LCD::print(format(mem));
    LCD::gotoXY(0, 2);
    LCD::print("                ");
    LCD::gotoXY(0, 3);
    LCD::print("                ");
    //	LCD::print(" D");
    //	LCD::print(format(readTemp()));
} //display max cpu utilization and ram.

void displayTripCombo(char *lu1, char * lm1, unsigned long v1, char * lu2, char * lm2, unsigned long v2,
                      char * lu3, char * lm3, unsigned long v3, char * lu4, char * lm4, unsigned long v4) {
    LCD::gotoXY(0, 0);
    LCD::print("                ");
    LCD::gotoXY(0, 1);
    dispv(lu1, lm1, v1);
    dispv(lu2, lm2, v2);
    LCD::gotoXY(0, 2);
    dispv(lu3, lm3, v3);
    dispv(lu4, lm4, v4);
    LCD::gotoXY(0, 3);
    LCD::print("                ");
    
}
void displayTripCombo2(char *lu1, char * lm1, unsigned long v1, char * lu2, char * lm2, unsigned long v2,
                       char * lu3, char * lm3, unsigned long v3, char * lu4, char * lm4, unsigned long v4,
                       char *lu5, char * lm5, unsigned long v5, char * lu6, char * lm6, unsigned long v6,
                       char * lu7, char * lm7, unsigned long v7, char * lu8, char * lm8, unsigned long v8) {
    LCD::gotoXY(0, 0);
    dispv(lu1, lm1, v1);
    dispv(lu2, lm2, v2);
    LCD::gotoXY(0, 1);
    dispv(lu3, lm3, v3);
    dispv(lu4, lm4, v4);
    LCD::gotoXY(0, 2);
    dispv(lu5, lm5, v5);
    dispv(lu6, lm6, v6);
    LCD::gotoXY(0, 3);
    dispv(lu7, lm7, v7);
    dispv(lu8, lm8, v8);
}

void displayTripCombo3(char *lu1, unsigned long v1, char * lu2,
                       char * lu3, char * lm3, unsigned long v3, char * lu4, char * lm4, unsigned long v4,
                       char *lu5, char * lm5, unsigned long v5, char * lu6, char * lm6, unsigned long v6,
                       char * lu7, unsigned long v7, char * lu8) {
    LCD::gotoXY(0, 0);
    //LCD::print(lu1);
    disp(lu1, v1);
    LCD::print(lu2);
    LCD::gotoXY(0, 1);
    dispv(lu3, lm3, v3);
    dispv(lu4, lm4, v4);
    LCD::gotoXY(0, 2);
    dispv(lu5, lm5, v5);
    dispv(lu6, lm6, v6);
    LCD::gotoXY(0, 3);
    disp(lu7, v7);
    LCD::print(lu8);
}

void displayTripDebug(char * lm1, unsigned long v1, char * lm2, unsigned long v2,
                      char * lm3, unsigned long v3, char * lm4, unsigned long v4,
                      char * lm5, unsigned long v5, char * lm6, unsigned long v6,
                      char * lm7, unsigned long v7, char * lm8, unsigned long v8) {
    LCD::gotoXY(0, 0);
    disp(lm1, v1);
    disp(lm2, v2);
    LCD::gotoXY(0, 1);
    disp(lm3, v3);
    disp(lm4, v4);
    LCD::gotoXY(0, 2);
    disp(lm5, v5);
    disp(lm6, v6);
    LCD::gotoXY(0, 3);
    disp(lm7, v7);
    disp(lm8, v8);
}

//arduino doesn't do well with types defined in a script as parameters, so have to pass as void * and use -> notation.
void tDisplay(void * r) { //display trip functions.
    Trip *t = (Trip *) r;
    LCD::gotoXY(0, 0);
    LCD::print("                ");
    LCD::gotoXY(0, 1);
    dispv("MH","KH",t->mph());
    dispv("MG","LK",t->mpg());
    LCD::gotoXY(0, 2);
    dispv("MI","KM",t->miles());
    dispv("GA"," L",t->gallons());
    LCD::gotoXY(0,3);
    LCD::print("                ");
}



//x=0..16, y= 0..1
void LCD::gotoXY(byte x, byte y) {
    byte dr = x + 0x80;
    if (y == 1)
        dr += 0x40;
    if (y == 2)
        dr += 0x10;
    if (y == 3)
        dr += 0x50;
    LCD::LcdCommandWrite(dr);
}

void LCD::print(char * string) {
    byte x = 0;
    char c = string[x];
    while (c != 0) {
        LCD::LcdDataWrite(c);
        x++;
        c = string[x];
    }
}

void LCD::init() {
    delay2(16); // wait for more than 15 msec
    pushNibble(0b00110000); // send (B0011) to DB7-4
    cmdWriteSet();
    tickleEnable();
    delay2(5); // wait for more than 4.1 msec
    pushNibble(0b00110000); // send (B0011) to DB7-4
    cmdWriteSet();
    tickleEnable();
    delay2(1); // wait for more than 100 usec
    pushNibble(0b00110000); // send (B0011) to DB7-4
    cmdWriteSet();
    tickleEnable();
    delay2(1); // wait for more than 100 usec
    pushNibble(0b00100000); // send (B0010) to DB7-4 for 4bit
    cmdWriteSet();
    tickleEnable();
    delay2(1); // wait for more than 100 usec
    // ready to use normal LcdCommandWrite() function now!
    LcdCommandWrite(0b00101000); // 4-bit interface, 2 display lines, 5x8 font
    LcdCommandWrite(0b00001100); // display control:
    LcdCommandWrite(0b00000110); // entry mode set: increment automatically, no display shift
    
    //creating the custom fonts:
    LcdCommandWrite(0b01001000); // set cgram
    static byte chars[] PROGMEM = {
        0b11111, 0b00000, 0b11111, 0b11111,
        0b00000, 0b11111, 0b00000, 0b11111, 0b11111, 0b00000, 0b00000,
        0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000, 0b00000,
        0b11111, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111, 0b00000,
        0b00000, 0b00000, 0b00000, 0b11111, 0b01110, 0b00000, 0b11111,
        0b11111, 0b11111, 0b01110, 0b00000, 0b11111, 0b11111, 0b11111,
        0b01110               };
    
    for (byte x = 0; x < 5; x++)
        for (byte y = 0; y < 8; y++)
            LcdDataWrite(pgm_read_byte(&chars[y*5+x])); //write the character data to the character generator ram
    LcdCommandWrite(0b00000001); // clear display, set cursor position to zero
    LcdCommandWrite(0b10000000); // set dram to zero
    
}

void LCD::tickleEnable() {
    // send a pulse to enable  PD5
    PORTD |= (1 << 5);
    delayMicroseconds2(1); // pause 1 ms according to datasheet
    PORTD &= ~(1 << 5);
    delayMicroseconds2(1); // pause 1 ms according to datasheet
}

void LCD::cmdWriteSet() { //set enable (PD5) low and DI(PD4) low
    PORTD &= ~(1 << 5);
    delayMicroseconds2(1); // pause 1 ms according to datasheet
    PORTD &= ~(1 << 4);
}

byte LCD::pushNibble(byte value) { //db7=PB5, db6=PB4, db5 = PB0, db4  = PD7
    value & 128 ? PORTB |= (1 << 5) : PORTB &= ~(1 << 5);
    value <<= 1;
    value & 128 ? PORTB |= (1 << 4) : PORTB &= ~(1 << 4);
    value <<= 1;
    value & 128 ? PORTB |= (1 << 0) : PORTB &= ~(1 << 0);
    value <<= 1;
    value & 128 ? PORTD |= (1 << 7) : PORTD &= ~(1 << 7);
    value <<= 1;
    return value;
}

void LCD::LcdCommandWrite(byte value) {
    value = pushNibble(value);
    cmdWriteSet();
    tickleEnable();
    value = pushNibble(value);
    cmdWriteSet();
    tickleEnable();
    delay2(5);
}

void LCD::LcdDataWrite(byte value) {
    PORTD |= (1 << 4); //di on pd4
    value = pushNibble(value);
    tickleEnable();
    value = pushNibble(value);
    tickleEnable();
    delay2(5);
}

// this function will return the number of bytes currently free in RAM
extern int __bss_end;
extern int *__brkval;
int memoryTest() {
    int free_memory;
    if ((int) __brkval == 0)
        free_memory = ((int) &free_memory) - ((int) &__bss_end);
    else
        free_memory = ((int) &free_memory) - ((int) __brkval);
    return free_memory;
}

Trip::Trip() {
}

//for display computing
unsigned long tmp1[2];
unsigned long tmp2[2];
unsigned long tmp3[2];

unsigned long instantmph() {
    //unsigned long vssPulseTimeuS = (lastVSS1 + lastVSS2) / 2;
    unsigned long vssPulseTimeuS = instant.vssPulseLength / instant.vssPulses;
    
    init64(tmp1, 0, 1000000000ul);
    init64(tmp2, 0, distancefactor);
    div64(tmp1, tmp2);
    init64(tmp2, 0, cyclesperhour);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, vssPulseTimeuS);
    div64(tmp1, tmp2);
    return tmp1[1];
}

unsigned long instantlkm() {
    unsigned long imph = instantmph();
    unsigned long igph = instantgph();
    if (igph == 0)
        return 0;
    if (imph == 0)
        return 999999000;
    init64(tmp1, 0, 100000ul);
    init64(tmp2, 0, igph);
    mul64(tmp2, tmp1);
    init64(tmp1, 0, imph);
    div64(tmp2,tmp1);
    return tmp2[1];
}
unsigned long instantkml() {
    unsigned long imph = instantmph();
    unsigned long igph = instantgph();
    if (igph == 0)
        return 0;
    if (imph == 0)
        return 999999000;
    init64(tmp1, 0, 1000ul);
    init64(tmp2, 0, imph);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, igph);
    div64(tmp1,tmp2);
    return tmp2[1];
}

unsigned long instantmpg() {
    if(parms[metricIdx]==1)
        return instantlkm();
    //    if(parms[metricIdx]==2)
    //		return instantkml();
    
    unsigned long imph = instantmph();
    unsigned long igph = instantgph();
    if (imph == 0)
        return 0;
    if (igph == 0)
        return 999999000;
    init64(tmp1, 0, 1000ul);
    init64(tmp2, 0, imph);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, igph);
    div64(tmp1,tmp2);
    return tmp1[1];
    
}

unsigned long instantgph() {
    //  unsigned long vssPulseTimeuS = instant.vssPulseLength/instant.vssPulses;
    
    //  unsigned long instInjStart=nil;
    //unsigned long instInjEnd;
    //unsigned long instInjTot;
    init64(tmp1, 0, instInjTot);
    init64(tmp2, 0, 3600000000ul);
    
    mul64(tmp1, tmp2);
    init64(tmp2, 0, 1000ul);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, fuelfactor);
    div64(tmp1, tmp2);
    init64(tmp2, 0, instInjEnd - instInjStart);
    div64(tmp1, tmp2);
    return tmp1[1];
}

unsigned long instantrpm(){
    //TODO: Acertar o rpm!!!
    init64(tmp1,0,instInjCount); // praticamente a mesma coisa... numero de aberturas do injetor no tempo abaixo...
    //init64(tmp1,0,instant.injPulses); // a cada instante mas não deveria ser por 2 minutos tb ????
    
    
    // revoluçoes por minuto -> rot/1 min -> se o vss sao 4 por revoluca temos as rev e nao precisamos do inj.... ?!?!?!?
    init64(tmp2,0,120000000ul); //2 minutos pois sao 2 revoluçoes ??
    mul64(tmp1,tmp2);
    init64(tmp2,0,1000ul);
    mul64(tmp1,tmp2);
    init64(tmp2,0,parms[injPulsesPer2Revolutions]);
    div64(tmp1,tmp2);
    init64(tmp2,0,instInjEnd-instInjStart); // tempo do injector open...
    div64(tmp1,tmp2);
    return tmp1[1]*10;
    
}

unsigned long readTemp(void) {
    unsigned long result;
    // Read temperature sensor against 1.1V reference
    ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
    delay(2); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Convert
    while (bit_is_set(ADCSRA,ADSC));
    result = ADCL;
    result |= ADCH<<8;
    result = (result - 125) * 1075;
    return result;
}

unsigned long readVcc(void) {
    unsigned long result;
    // Read 1.1V reference against AVcc
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(2); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Convert
    while (bit_is_set(ADCSRA,ADSC));
    result = ADCL;
    result |= ADCH<<8;
    result = 1126400L / result; // Back-calculate AVcc in mV
    return result;
}

unsigned long batteryVoltage(void){
    analogReference(EXTERNAL);
    delay(2);
    float vout = 0.0;
    float vin = 0.0;
    float R1 = 22000.0;    // !! resistance of R1 !!
    float R2 = 9100.0;     // !! resistance of R2 !!
    int value = 0;
    
    value = analogRead(voltagePin);
    vout = (value *5.0) / 1024.0;
    vin = vout / (R2/(R1+R2));
    analogReference(INTERNAL);
    return vin * 1000;
}

unsigned long pinVoltage(void){
    float vout = 0.0;
    float R1 = 22000.0;    // !! resistance of R1 !!
    float R2 = 9100.0;     // !! resistance of R2 !!
    int value = 0;
    
    value = analogRead(voltagePin);
    vout = (value *5.0) / 1024.0;
    
    return vout * 1000;
}
unsigned long pinVoltage1(void){
    float vout = 0.0;
    float R1 = 22000.0;    // !! resistance of R1 !!
    float R2 = 9100.0;     // !! resistance of R2 !!
    int value = 0;
    
    value = analogRead(voltagePin);
    vout = (value *5.0) / 1023.0;
    
    return vout * 1000;
}
unsigned long batteryVoltageVcc(void){
    float vout = 0.0;
    float vin = 0.0;
    float R1 = 22000.0;    // !! resistance of R1 !!
    float R2 = 9100.0;     // !! resistance of R2 !!
    int value = 0;
    
    value = analogRead(voltagePin);
    vout = (value /1023.0)*(readVcc()/1000);
    vin = vout / (R2/(R1+R2));
    
    return vin * 1000;
}
unsigned long tankLiters(void){
    
    float vout = 0.0;
    float vin = 0.0;
    float R1 = 22000.0;    // !! resistance of R1 !!
    float R2 = 9100.0;     // !! resistance of R2 !!
    int value = 0;
    
    value = analogRead(tankPin);
    vout = (value /1023.0)*(readVcc()/1000);
    vin = vout / (R2/(R1+R2));
    
    return vin * 1000;
}

unsigned long tankLiters_norm(void){
    
    float vout = 0.0;
    float vin = 0.0;
    float R1 = 22000.0;    // !! resistance of R1 !!
    float R2 = 9100.0;     // !! resistance of R2 !!
    int value = 0;
    
    value = analogRead(tankPin);
    vout = (value /1023.0)*(readVcc()/1000);
    vin = vout / (R2/(R1+R2));
    
    // vin=C*Vbat
    float C_float=1/((vin*1000)/batteryVoltageVcc());
    //unsigned long C=C_float*1000;
    /*
     //int ret=map(vin * 1000, 0, 5, 50, 1);
     unsigned long ret=(5-(vin*1000))*10;//abs(5-(constrain(vin,0,5)*1000))*10;
     //TODO: Acerto no valor
     return  ret;//vin*1000;//map(vin * 1000, 0, 5, 50, 1);
     */
    return C_float*100000;
}

unsigned long Trip::eocMiles() {
    init64(tmp1, 0, vssEOCPulses);
    init64(tmp2, 0, 1000);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, distancefactor);
    div64(tmp1, tmp2);
    return tmp1[1];
}


unsigned long Trip::idleGallons() {
    init64(tmp1, 0, injIdleHiSec);
    init64(tmp2, 0, 1000000);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, injIdleHius);
    add64(tmp1, tmp2);
    init64(tmp2, 0, dispadj2);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, fuelfactor);
    div64(tmp1, tmp2);
    return tmp1[1];
}

//eocMiles
//idleGallons

unsigned long Trip::lkm() {
    if (injPulses == 0)
        return 0;
    if (vssPulses == 0)
        return 999999000; //who doesn't like to see 999999?  :)
    
    
    init64(tmp1, 0, injHiSec);
    init64(tmp3, 0, 1000000ul);
    mul64(tmp3, tmp1);
    init64(tmp1, 0, injHius);
    add64(tmp3, tmp1);
    init64(tmp1, 0, distancefactor);
    mul64(tmp3, tmp1);
    init64(tmp1, 0, 80000ul);
    //	init64(tmp1, 0, 100000000);
    mul64(tmp3, tmp1);
    
    init64(tmp1, 0, fuelfactor);
    //	init64(tmp2, 0, dispadj2);
    //	mul64(tmp1, tmp2);
    init64(tmp2, 0, vssPulses);
    mul64(tmp1, tmp2);
    
    div64(tmp3, tmp1);
    return tmp3[1];
}

unsigned long Trip::mph() {
    if (loopCount == 0)
        return 0;
    
    // loopsPerSecond*3600          vssPulses * 1000
    // ----------------------- * -------------------------
    //       loopCount               distancefactor
    
    init64(tmp1, 0, loopsPerSecond);
    init64(tmp2, 0, vssPulses);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, 3600000ul);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, distancefactor);
    div64(tmp1, tmp2);
    init64(tmp2, 0, loopCount);
    div64(tmp1, tmp2);
    return tmp1[1];
}

unsigned long Trip::miles() {
    
    // vssPules * 1000
    // --------------
    // distancefactor
    
    init64(tmp1, 0, vssPulses);
    init64(tmp2, 0, 1000);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, distancefactor);
    div64(tmp1, tmp2);
    return tmp1[1];
}
unsigned long Trip::gallons() {
    
    // (injHiSec*1.000.000+injHius)
    // -----------------------------------
    //       fuelfactor*dispadj2
    
    init64(tmp1, 0, injHiSec);
    init64(tmp2, 0, 1000000);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, injHius);
    add64(tmp1, tmp2);
    //  init64(tmp2,0,1250);
    init64(tmp2, 0, dispadj2);
    div64(tmp1, tmp2);
    init64(tmp2, 0, fuelfactor);
    div64(tmp1, tmp2);
    return tmp1[1];
}
unsigned long Trip::mpg() {
    if(parms[metricIdx]==1)
        return lkm();
    if (vssPulses == 0)
        return 0;
    if (injPulses == 0)
        return 999999000; //who doesn't like to see 999999?  :)
    
    //         fuelfactor*dispadj2                 vssPulses
    // ----------------------------------- * ------------------------
    //     (injHiSec*1.000.000+injHius)          distancefactor
    
    init64(tmp1, 0, injHiSec);
    init64(tmp3, 0, 1000000);
    mul64(tmp3, tmp1);
    init64(tmp1, 0, injHius);
    add64(tmp3, tmp1);
    init64(tmp1, 0, distancefactor);
    mul64(tmp3, tmp1);
    
    init64(tmp1, 0, fuelfactor);
    init64(tmp2, 0, dispadj2);
    mul64(tmp1, tmp2);
    init64(tmp2, 0, vssPulses);
    mul64(tmp1, tmp2);
    
    div64(tmp1, tmp3);
    return tmp1[1];
}



//return the seconds as a time mmm.ss, eventually hhh:mm too
unsigned long Trip::time() {
    //  return seconds*1000;
    byte d = 60;
    unsigned long seconds = loopCount / loopsPerSecond;
    if(seconds/60 > 999) d = 3600; //scale up to hours.minutes if we get past 999 minutes
    return ((seconds / d) * 1000) + ((seconds % d) * 10);
}
/*
void Trip::save() {
    byte inicio = parmsLength+4;
    byte classSize = 9;
    byte x=inicio;
    
    unsigned long parm=loopCount;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    x+=4;
    
    parm=injPulses;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    x+=4;
    
    parm=injHius;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    x+=4;
    
    parm=injHiSec;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    x+=4;
    
    parm=vssPulses;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    x+=4;
    
    parm=vssPulseLength;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    x+=4;
    
    parm=injIdleHiSec;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    x+=4;
    
    parm=injIdleHius;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    x+=4;
    
    parm=vssEOCPulses;
    eeprom_write_byte((unsigned char *) x, (parm >> 24) & 255);
    eeprom_write_byte((unsigned char *) x + 1, (parm >> 16) & 255);
    eeprom_write_byte((unsigned char *) x + 2, (parm >> 8) & 255);
    eeprom_write_byte((unsigned char *) x + 3, (parm) & 255);
    
    
}
void Trip::load() { //return 1 if loaded ok
    byte inicio = parmsLength+4;
    byte classSize = 9;
    unsigned long parm;
    byte x=inicio;
    
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    loopCount = parm;
    x+=4;
    
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    injPulses = parm;
    x+=4;
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    injHius = parm;
    x+=4;
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    injHiSec = parm;
    x+=4;
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    vssPulses = parm;
    x+=4;
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    vssPulseLength = parm;
    x+=4;
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    injIdleHiSec = parm;
    x+=4;
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    injIdleHius = parm;
    x+=4;
    
    parm=eeprom_read_byte((unsigned char *) x);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 1);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 2);
    parm = (parm << 8) + eeprom_read_byte((unsigned char *) x + 3);
    vssEOCPulses = parm;
    
    
    
}
*/
void Trip::reset() {
    loopCount = 0;
    injPulses = 0;
    injHius = 0;
    injHiSec = 0;
    vssPulses = 0;
    vssPulseLength = 0;
    injIdleHiSec = 0;
    injIdleHius = 0;
    vssEOCPulses = 0;
}

void Trip::update(Trip t) {
    loopCount++; //we call update once per loop
    vssPulses += t.vssPulses;
    vssPulseLength += t.vssPulseLength;
    if (t.injPulses == 0) //track distance traveled with engine off
        vssEOCPulses += t.vssPulses;
    
    if (t.injPulses > 2 && t.injHius < 500000) {//chasing ghosts
        injPulses += t.injPulses;
        injHius += t.injHius;
        if (injHius >= 1000000) { //rollover into the injHiSec counter
            injHiSec++;
            injHius -= 1000000;
        }
        if (t.vssPulses == 0) { //track gallons spent sitting still
            
            injIdleHius += t.injHius;
            if (injIdleHius >= 1000000) { //r
                injIdleHiSec++;
                injIdleHius -= 1000000;
            }
        }
    }
}

char bignumchars1[] = {
    4, 1, 4, 0, 1, 4, 32, 0, 3, 3, 4, 0, 1, 3, 4, 0, 4, 2,
    4, 0, 4, 3, 3, 0, 4, 3, 3, 0, 1, 1, 4, 0, 4, 3, 4, 0, 4, 3, 4, 0 };
char bignumchars2[] = {
    4, 2, 4, 0, 2, 4, 2, 0, 4, 2, 2, 0, 2, 2, 4, 0, 32, 32,
    4, 0, 2, 2, 4, 0, 4, 2, 4, 0, 32, 4, 32, 0, 4, 2, 4, 0, 2, 2, 4, 0 };

void bigNum(unsigned long t, char * txt1, const char * txt2) {
    //  unsigned long t = 98550ul;//number in thousandths
    //  unsigned long t = 9855ul;//number in thousandths
    //  char * txt1="INST";
    //  char * txt2="MPG ";
    char dp1 = 32;
    char dp2 = 32;
    
    //	return format2(num,4,9999);
    
    
    char * r = "009.99"; //default to 999
    if (t <= 9950) {
        r = format(t ); //009.86
        dp1 = 5;
    }
    else if (t <= 99500) {
        r = format(t / 10); //009.86
        dp2 = 5;
    }
    else if (t <= 999500) {
        r = format(t / 100); //009.86
    }
    LCD::gotoXY(0, 0);
    LCD::print("                ");
    LCD::gotoXY(0, 1);
    LCD::print(bignumchars1 + (r[2] - '0') * 4);
    LCD::print(" ");
    LCD::print(bignumchars1 + (r[4] - '0') * 4);
    LCD::print(" ");
    LCD::print(bignumchars1 + (r[5] - '0') * 4);
    LCD::print(" ");
    LCD::print(txt1);
    
    LCD::gotoXY(0, 2);
    LCD::print(bignumchars2 + (r[2] - '0') * 4);
    LCD::LcdDataWrite(dp1);
    LCD::print(bignumchars2 + (r[4] - '0') * 4);
    LCD::LcdDataWrite(dp2);
    LCD::print(bignumchars2 + (r[5] - '0') * 4);
    LCD::print(" ");
    LCD::print((char *)txt2);
    LCD::gotoXY(0, 3);
    LCD::print("                ");
}

//the standard 64 bit math brings in  5000+ bytes
//these bring in 1214 bytes, and everything is pass by reference
unsigned long zero64[] = {
    0, 0 };

void init64(unsigned long an[], unsigned long bigPart, unsigned long littlePart) {
    an[0] = bigPart;
    an[1] = littlePart;
}

//left shift 64 bit "number"
void shl64(unsigned long an[]) {
    an[0] <<= 1;
    if (an[1] & 0x80000000)
        an[0]++;
    an[1] <<= 1;
}

//right shift 64 bit "number"
void shr64(unsigned long an[]) {
    an[1] >>= 1;
    if (an[0] & 0x1)
        an[1] += 0x80000000;
    an[0] >>= 1;
}

//add ann to an
void add64(unsigned long an[], unsigned long ann[]) {
    an[0] += ann[0];
    if (an[1] + ann[1] < ann[1])
        an[0]++;
    an[1] += ann[1];
}

//subtract ann from an
void sub64(unsigned long an[], unsigned long ann[]) {
    an[0] -= ann[0];
    if (an[1] < ann[1]) {
        an[0]--;
    }
    an[1] -= ann[1];
}

//true if an == ann
boolean eq64(unsigned long an[], unsigned long ann[]) {
    return (an[0] == ann[0]) && (an[1] == ann[1]);
}

//true if an < ann
boolean lt64(unsigned long an[], unsigned long ann[]) {
    if (an[0] > ann[0])
        return false;
    return (an[0] < ann[0]) || (an[1] < ann[1]);
}

//divide num by den
void div64(unsigned long num[], unsigned long den[]) {
    unsigned long quot[2];
    unsigned long qbit[2];
    unsigned long tmp[2];
    init64(quot, 0, 0);
    init64(qbit, 0, 1);
    
    if (eq64(num, zero64)) { //numerator 0, call it 0
        init64(num, 0, 0);
        return;
    }
    
    if (eq64(den, zero64)) { //numerator not zero, denominator 0, infinity in my book.
        init64(num, 0xffffffff, 0xffffffff);
        return;
    }
    
    init64(tmp, 0x80000000, 0);
    while (lt64(den, tmp)) {
        shl64(den);
        shl64(qbit);
    }
    
    while (!eq64(qbit, zero64)) {
        if (lt64(den, num) || eq64(den, num)) {
            sub64(num, den);
            add64(quot, qbit);
        }
        shr64(den);
        shr64(qbit);
    }
    
    //remainder now in num, but using it to return quotient for now
    init64(num, quot[0], quot[1]);
}

//multiply num by den
void mul64(unsigned long an[], unsigned long ann[]) {
    unsigned long p[2] = {
        0, 0               };
    unsigned long y[2] = {
        ann[0], ann[1]               };
    while (!eq64(y, zero64)) {
        if (y[1] & 1)
            add64(p, an);
        shl64(an);
        shr64(y);
    }
    init64(an, p[0], p[1]);
}

void save() {
    eeprom_write_byte((unsigned char *) 0, VER);
    eeprom_write_byte((unsigned char *) 1, parmsLength);
    byte parmPos = 0;
    for (int x = 4; parmPos < parmsLength; x += 4) {
        unsigned long byte_tmp = parms[parmPos];
        eeprom_write_byte((unsigned char *) x, (byte_tmp >> 24) & 255);
        eeprom_write_byte((unsigned char *) x + 1, (byte_tmp >> 16) & 255);
        eeprom_write_byte((unsigned char *) x + 2, (byte_tmp >> 8) & 255);
        eeprom_write_byte((unsigned char *) x + 3, (byte_tmp) & 255);
        parmPos++;
    }
}

byte load() { //return 1 if loaded ok
    
    //TODO: LOAD
    // PARA Mapear a versao
#ifdef usedefaults
    return 1;
#endif
    byte ver = eeprom_read_byte((unsigned char *) 0); // versao dos dados
    byte parmsQtd = eeprom_read_byte((unsigned char *) 1); // tamanho dos dados
    byte parmPos= 0;
    if (ver == VER){//guinosig || b == guinosigold) {
        for (int x = 4; parmPos < parmsQtd; x += 4) {
            unsigned long byte_tmp;
            byte_tmp = eeprom_read_byte((unsigned char *) x);
            byte_tmp = (byte_tmp << 8) + eeprom_read_byte((unsigned char *) x + 1);
            byte_tmp = (byte_tmp << 8) + eeprom_read_byte((unsigned char *) x + 2);
            byte_tmp = (byte_tmp << 8) + eeprom_read_byte((unsigned char *) x + 3); //!! Crap sempre retorna 0
            parms[parmPos] = byte_tmp;
            parmPos++;
        }
        return 1;
    }
    return 0;
}

char * uformat(unsigned long val) {
    unsigned long d = 1000000000ul;
    for (byte p = 0; p < 10; p++) {
        mBuff[p] = '0' + (val / d);
        val = val - (val / d * d);
        d /= 10;
    }
    mBuff[10] = 0;
    return mBuff;
}

unsigned long rformat(char * val) {
    unsigned long d = 1000000000ul;
    unsigned long v = 0ul;
    for (byte p = 0; p < 10; p++) {
        v = v + (d * (val[p] - '0'));
        d /= 10;
    }
    return v;
}

void editParm(byte parmIdx) {
    unsigned long v = parms[parmIdx];
    byte p = 9; //right end of 10 digit number
    //display label on top line
    //set cursor visible
    //set pos = 0
    //display v
    
    LCD::gotoXY(8, 0);
    LCD::print("        ");
    LCD::gotoXY(0, 0);
    LCD::print(parmLabels[parmIdx]);
    LCD::gotoXY(0, 1);
    char * fmtv = uformat(v);
    LCD::print(fmtv);
    LCD::print(" OK XX");
    LCD::LcdCommandWrite(0b00001110);
    
    for (int x = 9; x >= 0; x--) { //do a nice thing and put the cursor at the first non zero number
        if (fmtv[x] != '0')
            p = x;
    }
    byte keyLock = 1;
    while (true) {
        
        if (p < 10)
            LCD::gotoXY(p, 1);
        if (p == 10)
            LCD::gotoXY(11, 1);
        if (p == 11)
            LCD::gotoXY(14, 1);
        
        if (keyLock == 0) {
            if (!(buttonState & lbuttonBit) && !(buttonState & rbuttonBit)) {// left & right
                LCD::LcdCommandWrite(0b00001100);
                return;
            }
            else if (!(buttonState & lbuttonBit)) {// left
                p = p - 1;
                if (p == 255)
                    p = 11;
            }
            else if (!(buttonState & rbuttonBit)) {// right
                p = p + 1;
                if (p == 12)
                    p = 0;
            }
            else if (!(buttonState & mbuttonBit)) {// middle
                if (p == 11) { //cancel selected
                    LCD::LcdCommandWrite(0b00001100);
                    return;
                }
                if (p == 10) { //ok selected
                    LCD::LcdCommandWrite(0b00001100);
                    parms[parmIdx] = rformat(fmtv);
                    return;
                }
                
                byte n = fmtv[p] - '0';
                n++;
                if (n > 9)
                    n = 0;
                if (p == 0 && n > 3)
                    n = 0;
                fmtv[p] = '0' + n;
                LCD::gotoXY(0, 1);
                LCD::print(fmtv);
                LCD::gotoXY(p, 1);
                if (parmIdx == contrastIdx)//adjust contrast dynamically
                    OCR0A = rformat(fmtv);
                
                //                 analogWrite(ContrastPin,rformat(fmtv));
                
                
            }
            
            if (buttonState != buttonsUp)
                keyLock = 1;
        }
        else {
            keyLock = 0;
        }
        buttonState = buttonsUp;
        delay2(125);
    }
    
}

void initGuino() { //edit all the parameters
    
/*
#ifdef SAVE_TANK
    tank.save();
#endif
 */
    for (int x = 0; x < parmsLength; x++)
        editParm(x);
    save();
    injectorSettleTime = injhold;
    
    int0Func = processInjOpen;
    int1Func = processInjClosed;
    EIMSK &= ~(1 << INT0);
    EIMSK &= ~(1 << INT1);
    
    EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01)))
    | ((parms[injEdgeIdx] == 1 ? RISING : FALLING) << ISC00);
    EIMSK |= (1 << INT0);
    EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11)))
    | ((parms[injEdgeIdx] == 1 ? FALLING : RISING) << ISC10);
    EIMSK |= (1 << INT1);
    
    distancefactor = parms[vssPulsesPerMileIdx];
    fuelfactor = parms[microSecondsPerGallonIdx];
    if ((parms[metricIdx]==1)||(parms[metricIdx]==2)){
        distancefactor /= 1.609;
        fuelfactor /= 3.785;
    }
    
    holdDisplay = 1;
}

unsigned long millis2() {
    return timer2_overflow_count * 64UL * 2 / (16000000UL / 128000UL);
}

void delay2(unsigned long ms) {
    unsigned long start = millis2();
    while (millis2() - start < ms)
        ;
}

/* Delay for the given number of microseconds.  Assumes a 16 MHz clock.
 * Disables interrupts, which will disrupt the millis2() function if used
 * too frequently. */
void delayMicroseconds2(unsigned int us) {
    uint8_t oldSREG;
    if (--us == 0)
        return;
    us <<= 2;
    us -= 2;
    oldSREG = SREG;
    cli();
    // busy wait
    __asm__ __volatile__ (
                          "1: sbiw %0,1" "\n\t" // 2 cycles
                          "brne 1b" :
                          "=w" (us) :
                          "0" (us) // 2 cycles
                          );
    // reenable interrupts.
    SREG = oldSREG;
}

void simpletx(char * string) {
    if (UCSR0B != (1 << TXEN0)) { //do we need to init the uart?
        UBRR0H = (unsigned char) (myubbr >> 8);
        UBRR0L = (unsigned char) myubbr;
        UCSR0B = (1 << TXEN0);//Enable transmitter
        UCSR0C = (3 << UCSZ00);//N81
    }
    while (*string) {
        while (!(UCSR0A & (1 << UDRE0)))
            ;
        UDR0 = *string++; //send the data
    }
}

int main(void) {
    sei();
    sbi(TCCR0A, WGM01);
    sbi(TCCR0A, WGM00);
    sbi(TCCR0B, CS01);
    sbi(TCCR0B, CS00);
    sbi(TIMSK0, TOIE0);
    
    // set timer 1 prescale factor to 64
    sbi(TCCR1B, CS11);
    sbi(TCCR1B, CS10);
    // put timer 1 in 8-bit phase correct pwm mode
    sbi(TCCR1A, WGM10);
    // set timer 2 prescale factor to 64
    sbi(TCCR2B, CS22);
    // configure timer 2 for phase correct pwm (8-bit)
    sbi(TCCR2A, WGM20);
    
    // set a2d prescale factor to 128
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
    
    // enable a2d conversions
    sbi(ADCSRA, ADEN);
    
    UCSR0B = 0;
    
    sei();
    
    timer2_overflow_count = 0;
    
    TCCR2A = 1 << WGM20 | 1 << WGM21;
    // set timer 2 prescale factor to 64
    TCCR2B = 1 << CS22;
    TIMSK2 |= 1 << TOIE2;
    TIMSK0 &= !(1 << TOIE0);
    
    setup();
    mainloop();
    
    return 0;
}

//#define DTE_CFG                    1  /* 0=Off 1=On        */
/*
 pFunc displayFuncs[] ={
 doDisplayCustom,
 doDisplayInstantCurrent,
 doDisplayInstantTank,
 doDisplayBigInstant,
 doDisplayBigCurrent,
 doDisplayBigTank,
 doDisplayCurrentTripData,
 doDisplayTankTripData,
 doDisplayEOCIdleData,
 doDisplaySystemInfo,
 #if (BARGRAPH_DISPLAY_CFG == 1)
 doDisplayBigPeriodic,
 doDisplayBarGraph,
 #endif
 #if (DTE_CFG == 1)
 doDisplayBigDTE,
 #endif
 };
 displayFuncNames[x++]=  PSTR("Custom  ");
 displayFuncNames[x++]=  PSTR("Instant/Current ");
 displayFuncNames[x++]=  PSTR("Instant/Tank ");
 displayFuncNames[x++]=  PSTR("BIG Instant ");
 displayFuncNames[x++]=  PSTR("BIG Current ");
 displayFuncNames[x++]=  PSTR("BIG Tank ");
 displayFuncNames[x++]=  PSTR("Current ");
 displayFuncNames[x++]=  PSTR("Tank ");
 displayFuncNames[x++]=  PSTR("EOC mi/Idle gal ");
 displayFuncNames[x++]=  PSTR("CPU Monitor ");
 #if (BARGRAPH_DISPLAY_CFG == 1)
 displayFuncNames[x++]=  PSTR("BIG Periodic ");
 displayFuncNames[x++]=  PSTR("Bargraph ");
 #endif
 #if (DTE_CFG == 1)
 displayFuncNames[x++]=  PSTR("BIG DTE ");
 #endif
 
 //distance to empty....
 #if (DTE_CFG == 1)
 void doDisplayBigDTE(void) {
 unsigned long dte;
 signed long gals_remaining;
 // TODO: user configurable safety factor see minus zero below
 gals_remaining = (parms[tankSizeIdx] - tank.gallons()) - 0;
 gals_remaining = MAX(gals_remaining, 0);
 dte = gals_remaining * (tank.mpg()/100);
 dte /= 10; /* divide by 10 here to avoid precision loss
 // dividing a signed long by 10 for some reason adds 100 bytes to program size?
 // otherwise I would've divided gals by 10 earlier!
 bigNum(dte, "DIST", "TO E");
 }
 
 #define MAX(value2, value1)\
 (((value1)>=(value2)) ? (value1) : (value2))
 
 
 */







