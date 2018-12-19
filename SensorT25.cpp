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
v0.4.2 - ESP32 is not supporting float in IRQ handler, use int instead
v1.1.0 - sync version with git releases
v1.1.2 - add again IRAM_ATTR for _irqHandler function
v1.1.3 - add condition for ESP32 and other platforms for IRAM_ATTR
*/

#include <SensorT25.h>

unsigned long SensorT25::_buffer[BUFF_ROW] = {0L,0L,0L}; // raw data buffer
boolean SensorT25::_valid[SENSCOUNT] = {false,false,false}; // valid dta forsensor
byte SensorT25::_sid[SENSCOUNT];           // sensor ID
//float SensorT25::_temperature[SENSCOUNT];  // temperature
int SensorT25::_itemperature[SENSCOUNT];   // temperature
unsigned long SensorT25::_time[SENSCOUNT]; // last set time
boolean SensorT25::_start = LOW;             // start data reading flag
boolean SensorT25::_space = LOW;             // space is starting
boolean SensorT25::_bit0 = LOW;
boolean SensorT25::_bit1 = LOW;
byte SensorT25::_bits = 0;                   // bits counter in data word
byte SensorT25::_repeat = 0;                 // counter for repeated reading
unsigned long SensorT25::_lastTime = 0;      // variable for last time point
byte SensorT25::_irq_pin;                    // number of used IRQ PIN
// methods
void SensorT25::enable(byte irq_pin)               //enable data reading - INT0,INT1 name, number 2 or 3 for UNO
{
  _irq_pin = irq_pin;
  pinMode(irq_pin, INPUT_PULLUP);                                       // input for RF receiver
  attachInterrupt(digitalPinToInterrupt(irq_pin), _irqHandler, CHANGE); // IRQ handler for RF receiver
}

void SensorT25::disable(byte irq_pin)                      //disable daata reading
{
  detachInterrupt(digitalPinToInterrupt(irq_pin));         // stop IRQ
}

// are data valid for channel
boolean SensorT25::isValid(byte channel)
{
  return (isValidChannel(channel) ? _valid[channel] : false);
}

// set data as invalid for channel
void SensorT25::setInValid(byte channel)
{
  if(isValidChannel(channel))
  {
    _valid[channel] = false;
  }
}

//get sensor ID for channel
byte SensorT25::getSID(byte channel)
{
  return (isValidChannel(channel) ? _sid[channel] : 0);
}

//get temperature for channel
float SensorT25::getTemperature(byte channel)
{
  //return (isValidChannel(channel) ? _temperature[channel] : 0);
  return (isValidChannel(channel) ? float(_itemperature[channel])/10 : 0);
}

//get temperature value age in s
long SensorT25::getValueAge(byte channel)
{
  return (isValidChannel(channel) ? (millis() - _time[channel])/1000 : 0);
}

// channel number validation
boolean SensorT25::isValidChannel(byte channel)
{
  return ( channel >= 0 && channel < SENSCOUNT);
}

// return specified bits from sensor word
unsigned int SensorT25::_decodeBits(unsigned long buffer, unsigned long mask, byte shift )
{
    return ((buffer & mask) >> shift);
}

// decode sensor ID from data word
byte SensorT25::_decodeSID(unsigned long buffer)
{
  return _decodeBits(buffer, SID_MASK, SID_SHIFT);
}

// decode sensor channel from data word , 0 to 2
byte SensorT25::_decodeChanel(unsigned long buffer)
{
  return _decodeBits(buffer, CHA_MASK, CHA_SHIFT);
}

// decode temperature from data word
// float SensorT25::_decodeTemp(unsigned long buffer)
// {
//   int t = _decodeBits(buffer, TEM_MASK, TEM_SHIFT);
//   if (t > 2047) {t = t - 4095;}
//   return float(t)/10;
// }

// decode temperature from data word
int SensorT25::_idecodeTemp(unsigned long buffer)
{
  int t = _decodeBits(buffer, TEM_MASK, TEM_SHIFT);
  if (t > 2047) {t = t - 4095;}
  return t;
}

// IRQ handler for decode bits ...
#ifdef ESP32
  void IRAM_ATTR SensorT25::_irqHandler()
  #warning "sT25: ESP32 platform"
#else
  void SensorT25::_irqHandler()
  #warning "sT25: other platform"
#endif
{
  unsigned long currentTime = micros();        // shot time
  int duration = currentTime - _lastTime;       // calculate duration
  _lastTime = currentTime;                      // memory curent time
  boolean state = digitalRead(_irq_pin);        // read input status
  if (!state)                                  // goes from HIGH to LOW
  {
      _space = _isImpuls(duration - IMPULSE, ITRESHOLD);     // if impuls time is valid
  }
  else if (_space)                              // goes from LOW to HIGH
  {
    if (!_start)                                // start flag is false
    {
      _start = _isImpuls(duration - PAUSE, STRESHOLD);  // start temperature data it was pause signal
    }
    else                                            // start flag is true
    {
      _bit0 = _isImpuls(duration - TBIT0, STRESHOLD);  // is it bit 0
      _bit1 = _isImpuls(duration - TBIT1, STRESHOLD);  // is it bit 1
      if(_bit0 ^ _bit1)                               // XOR / only one is true
      {
        _bit1 && bitSet(_buffer[_repeat],DATA_LONG - _bits - 1);   // write bit 1
        _bit0 && bitClear(_buffer[_repeat],DATA_LONG - _bits - 1); // write bit 0
        _bits++;                                                // increment bit counter
      }
      else                                          // both or nothing bit
      {
        _resetTWord ();                              // reset reading
      }
      if(_bits >= DATA_LONG)                         // last bit was reading
      {
        if(_repeat > 0)                              // is it temp sentence hienr then 0
        {
          if (_buffer[_repeat] != _buffer[_repeat-1])       // current temp word is differnt than previous
          {
            _resetTWord ();                          // reset reading
          }
        }
        _repeat++;                                   // increment temp word reading
        _newTWord();                                 // reset counters and flags
      }
    }
  }

 // write valid data to data array
 if (_repeat >= BUFF_ROW)                            // buffer is full
  {
  byte ch = (_decodeChanel(_buffer[0]));                 // set channel
  _valid[ch] = true;                                 // set data valid
  _sid[ch] = (_decodeSID(_buffer[0])); //sensor ID        // save sensor ID
  _itemperature[ch] = (_idecodeTemp(_buffer[0]));           // save temperature
  _time[ch] = millis();                              //save data timestamp
  _resetTWord ();                                    // reset reading
  }
}

void SensorT25::_resetTWord ()                                  // reset reading
{
  _newTWord();                                       // reset counter and flags
  _repeat = 0;                                       // reset temp word counter
}

void SensorT25::_newTWord ()                                    // reset temp word counter
{
  _bits = 0;                                         // bits counter
  _space = LOW;                                      // space is measured
  _start = LOW;                                      // starting data reading
}

boolean SensorT25::_isImpuls(int duration, byte TRESHOLD)          // is it valid impulse
{
    return (abs(duration) <= TRESHOLD);
}
