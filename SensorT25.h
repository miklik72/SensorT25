/*
Thermo sensor Sencor T25 reader via RXB^receiver
Initial version for use with functions, preparation for develop T25 library
Output to serial console
Miklik, 20.6.2016
v0.2.0 - copy from 0.1.9 and remove coments and debug data
v0.2.1 - add new funcion - enable, disable and remove some variables
v0.2.2 - rename data array to buffer, and use data array for data per chanel
v0.2.3 - save data directly to data array
v0.2.4 - rename get functions to decode and create new getfunctions and some isFFF functions
v0.2.5 - correct some daty types
v0.2.6 - set prefix _ for global private objects
v0.3.0 - do it like class
v0.3.1 - change all to static
v0.3.2 - rename class to SensorT25
v0.4.0 - save it like library
v0.4.1 - rename BIT0 and 1 to TBIT0 and 1, user IRAM_ATTR for IRQ handler
v0.4.2 - ESP32 is not supporting float in IRQ handler, use int for array
v0.4.2 - treshold 100 for impuls
*/

#ifndef SensorT25_h
#define SensorT25_h

#include <Arduino.h>

#define SENSCOUNT 3           // Sensors count
#define START  9000           // start space time
#define PAUSE  4020           // space between sentences
#define TBIT1   1980           // 1
#define TBIT0   1000           // 0
#define IMPULSE 410           // impulse
#define ITRESHOLD 100          // impulse treshold +- for duration
#define STRESHOLD 50          // space treshold +- for duration
#define BUFF_ROW 3             // row in buffer for data
#define DATA_LONG 32           // buffer long 32bits, last 4 bits are droped

// masks for data decode
#define SID_MASK 0xFF000000    // mask sensore id bits 31-24
#define SID_SHIFT 24           // move SID bits to low bits
#define BAT_MASK 0x00C00000    // mask for batery status bits 23 (22?)
#define BAT_SHIFT 22           // move BAT bits to low bits
#define CHA_MASK 0x00300000    // mask for chanel bits 21-20
#define CHA_SHIFT 20           // move chanel bits
#define TEM_MASK 0x000FFF00    // mask for temperature 12b 19 - 8
#define TEM_SHIFT 8            // move temperature bits

class SensorT25 {
  private:
    static unsigned long _buffer[BUFF_ROW]; // raw data buffer
    // sensors data - index 0 for channel 1,...
    static boolean _valid[SENSCOUNT]; // valid dta forsensor
    static byte _sid[SENSCOUNT];           // sensor ID
    //static float _temperature[SENSCOUNT];  // temperature
    static int _itemperature[SENSCOUNT];   // temperature in integer (float * 10)
    static unsigned long _time[SENSCOUNT]; // last set time
    static boolean _start;             // start data reading flag
    static boolean _space;             // space is starting
    static boolean _bit0;
    static boolean _bit1;
    static byte _bits;                   // bits counter in data word
    static byte _repeat;                 // counter for repeated reading
    static unsigned long _lastTime;      // variable for last time point
    static byte _irq_pin;

    static unsigned int _decodeBits(unsigned long, unsigned long, byte );
    static byte _decodeSID(unsigned long);
    static byte _decodeChanel(unsigned long);
    //static float _decodeTemp(unsigned long);
    static int _idecodeTemp(unsigned long);
    static void _irqHandler();
    static void _resetTWord ();                                  // reset reading
    static void _newTWord ();                                    // reset temp word counter
    static boolean _isImpuls(int, byte);       // is it valid impulse

  public:
    static void enable(byte);                    //enable data reading - INT0,INT1 name, number 2 or 3 for UNO
    static void disable(byte);                   //disable daata reading
    static boolean isValid(byte);
    static void setInValid(byte);
    static byte getSID(byte);
    static float getTemperature(byte);
    static long getValueAge(byte);
    static boolean isValidChannel(byte);
};

#endif
