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
*/
#define IRQ_PIN 2             // RF input with irq
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

class mmT25 {
  private:
    unsigned long _buffer[BUFF_ROW] = {0,0,0}; // raw data buffer
    // sensors data - index 0 for channel 1,...
    boolean _valid[SENSCOUNT] = {false,false,false}; // valid dta forsensor
    byte _sid[SENSCOUNT];           // sensor ID
    float _temperature[SENSCOUNT];  // temperature
    unsigned long _time[SENSCOUNT]; // last set time
    boolean _start = LOW;             // start data reading flag
    boolean _space = LOW;             // space is starting 
    boolean _bit0 = LOW;
    boolean _bit1 = LOW;
    byte _bits = 0;                   // bits counter in data word
    byte _repeat = 0;                 // counter for repeated reading
    //byte count = 0;                  // count temperature sentences 
    unsigned long _lastTime = 0;      // variable for last time point
   
    unsigned int _decodeBits(unsigned long, unsigned long, byte );
    byte _decodeSID(unsigned long);
    byte _decodeChanel(unsigned long);
    float _decodeTemp(unsigned long);
    void _irqHandler();
    void _resetTWord ();                                  // reset reading
    void _newTWord ();                                    // reset temp word counter
    boolean _isImpuls(int, byte);       // is it valid impulse
    
  public:
    mmT25(byte);
    void enable(byte);                    //enable data reading - INT0,INT1 name, number 2 or 3 for UNO 
    void disable(byte);                   //disable daata reading
    boolean isValid(byte);
    void setInValid(byte);
    byte getSID(byte);
    float getTemperature(byte);
    long getValueAge(byte);
    boolean isValidChannel(byte);
};

 // methods
    mmT25::mmT25(byte irq_pin)
    {
      delay(100);
    }
    
    void mmT25::enable(byte irq_pin)               //enable data reading - INT0,INT1 name, number 2 or 3 for UNO 
            {
               pinMode(irq_pin, INPUT_PULLUP);    // input for RF receiver
               attachInterrupt(digitalPinToInterrupt(irq_pin), _irqHandler, CHANGE); // IRQ handler for RF receiver
            }
                 
    void mmT25::disable(byte irq_pin)                      //disable daata reading
            {
               detachInterrupt(digitalPinToInterrupt(irq_pin));          // stop IRQ
            }
    
    //are data valid        
    boolean mmT25::isValid(byte channel)
    {
      return (isValidChannel(channel) ? _valid[channel] : false);
    }
    
    // set data as invalid for channel
    void mmT25::setInValid(byte channel)
    {
      if(isValidChannel(channel))
      {
        _valid[channel] = false;
      }
    }
    
    //get sensor ID for channel
    byte mmT25::getSID(byte channel)
    {
      return (isValidChannel(channel) ? _sid[channel] : 0);  
    }
    
    //get temperature for channel
    float mmT25::getTemperature(byte channel)
    {
      return (isValidChannel(channel) ? _temperature[channel] : 0);
    }
    
    //get temperature value age in s
    long mmT25::getValueAge(byte channel)
    {
      return (isValidChannel(channel) ? (millis() - _time[channel])/1000 : 0);  
    }
    
    // channel number validation
    boolean mmT25::isValidChannel(byte channel)
    {
      return ( channel >= 0 && channel < SENSCOUNT);
    }

    // return specified bits from sensor word 
    unsigned int mmT25::_decodeBits(unsigned long buffer, unsigned long mask, byte shift )
    {
        return ((buffer & mask) >> shift);
    }
    
    // decode sensor ID from data word
    byte mmT25::_decodeSID(unsigned long buffer)
    {
      return _decodeBits(buffer, SID_MASK, SID_SHIFT);
    }
    
    // decode sensor channel from data word , 0 to 2
    byte mmT25::_decodeChanel(unsigned long buffer)
    {
      return _decodeBits(buffer, CHA_MASK, CHA_SHIFT);
    }
    
    // decode temperature from data word
    float mmT25::_decodeTemp(unsigned long buffer)
    {
      int t = _decodeBits(buffer, TEM_MASK, TEM_SHIFT);
      if (t > 2047) {t = t - 4095;}
      return float(t)/10;
    }
    
    // IRQ handler for decode bits
    void mmT25::_irqHandler()
    {
      unsigned long currentTime = micros();        // shot time
      int duration = currentTime - _lastTime;       // calculate duration
      _lastTime = currentTime;                      // memory curent time
      boolean state = digitalRead(IRQ_PIN);        // read input status
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
          _bit0 = _isImpuls(duration - BIT0, STRESHOLD);  // is it bit 0
          _bit1 = _isImpuls(duration - BIT1, STRESHOLD);  // is it bit 1
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
      _temperature[ch] = (_decodeTemp(_buffer[0]));           // save temperature
      _time[ch] = millis();                              //save data timestamp
      _resetTWord ();                                    // reset reading
      }  
    }
    
    void mmT25::_resetTWord ()                                  // reset reading
    {
      _newTWord();                                       // reset counter and flags
      _repeat = 0;                                       // reset temp word counter
    }
    
    void mmT25::_newTWord ()                                    // reset temp word counter
    {
      _bits = 0;                                         // bits counter
      _space = LOW;                                      // space is measured
      _start = LOW;                                      // starting data reading
    }
    
    boolean mmT25::_isImpuls(int duration, byte TRESHOLD)          // is it valid impulse
    {
        return (abs(duration) <= TRESHOLD);
    }


mmT25 t25(2);

void setup()
{
  Serial.begin(9600);
  t25.enable(IRQ_PIN);
  Serial.println("Start");           
}

void loop()
{  
  for(int i = 0; i < SENSCOUNT; i++) {
    Serial.print("CH ");
    Serial.print(i+1);                //channel
    Serial.print("-V ");
    Serial.print(t25.isValid(i));         //valid data
    Serial.print("-SID ");
    Serial.print(t25.getSID(i));          //sensor ID
    Serial.print("-TEMP ");
    Serial.print(t25.getTemperature(i),1);
    Serial.print("-AGE ");
    Serial.print(t25.getValueAge(i));
    Serial.println();
  }
  delay(10000);
}
