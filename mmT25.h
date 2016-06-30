/*
 * T25 Water Temperature Receiver with insight board TX1-3
 * 
 * This library receives and decodes data sent by
 * remote water sensor T25 and compatible ones.
 * 
 * Tested with Arduino UNO R3
 * 
 * v0.1.0 2016/06/02 by Martin Mikala (miklik72)
 *
 * License: GPLv3. See LICENSE.md
 */

#ifndef mmT25_h
#define mmT25_h

#if ARDUINO >= 100
 #include "Arduino.h"
#endif

#define SENSCOUNT 3           // Sensors count
#define START  9000           // start space time
#define PAUSE  4020           // space between sentences
#define BIT1   1980           // 1
#define BIT0   1000           // 0
#define IMPULSE 410           // impulse
#define ITRESHOLD 70          // impulse treshold +- for duration
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

class mmT25
{
  public:
    mmT25();
    void      enable(uint8_t irq_pin);               //enable data reading - INT0,INT1 name, number 2 or 3 for UNO 
    void      disable(uint8_t irq_pin);              //disable daata reading
    bool      isValid(uint8_t channel);                  //are data valid
    void      setInValid(uint8_t channel);                  // set data as invalid for channel
    float     getTemperature(uint8_t channel);             //get temperature for channel
    uint8_t   getSID(uint8_t channel);                   //get sensor ID for channel
    uint16_t  getValueAge(uint8_t channel);              //get temperature value age in s
    bool      isValidChannel(uint8_t channel);           // channel number validation

    
  private:
    uint32_t  _buffer[BUFF_ROW]; // raw data buffer
    bool      _valid[SENSCOUNT]; // valid dta forsensor
    uint8_t   _sid[SENSCOUNT];           // sensor ID
    float     _temperature[SENSCOUNT];  // temperature
    uint32_t  _time[SENSCOUNT]; // last set time
    bool      _start;             // start data reading flag
    bool      _space;             // space is starting 
    bool      _bit0;
    bool      _bit1;
    uint8_t   _bits;                   // bits counter in data word
    uint8_t   _repeat;                 // counter for repeated reading
    uint32_t  _lastTime;      // variable for last time point
    uint16_t  _decodeBits(uint32_t buffer, uint32_t mask, uint8_t shift ); // return specified bits from sensor word 
    uint8_t   _decodeSID(uint32_t buffer);    // decode sensor ID from data word
    uint8_t   _decodeChanel(uint32_t buffer); // decode sensor channel from data word , 0 to 2
    float     _decodeTemp(uint32_t buffer);     // decode temperature from data word
    void      _irqHandler();                     // IRQ handler for decode bits
    void      _resetTWord ()                                  // reset reading
    void      _newTWord ()                                    // reset temp word counter
    bool      _isImpuls(uint16_t duration, uint8_t treshold)          // is it valid impulse
};s 

#endif
