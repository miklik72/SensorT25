/*
 * T25 Water Temperature Receiver with insight board TX1-3
 * 
 * This library receives and decodes data sent by
 * remote water sensor T25 and compatible ones.
 * 
 * Copyright 2016 by Martin Mikala
 *
 * License: GPLv3. See LICENSE.md
 */

#include <T25-TempSensor.h>

        static byte T25-TempSensor::IrgPin;
        static long T25-TempSensor::buffer[BUFF_ROW];                              // data buffer
        static int T25-TempSensor::data[SENSOR_COUNT] = {CHA_MASK_1,CHA_MASK_2,CHA_MASK_3};   // raw data word for each channel, init for channel 1-3(0-2)
        static long T25-TempSensor::age[SENSOR_COUNT] = {0,0,0};
        static boolean T25-TempSensor::start = LOW;                                      // start data reading flag
        static boolean T25-TempSensor::space = LOW;                                      // space is starting 
        static boolean T25-TempSensor::bit0 = LOW;
        static boolean T25-TempSensor::bit1 = LOW;
        static byte T25-TempSensor::bits = 0;                                        // bits counter in data word
        static byte T25-TempSensor::repeat = 0;                                      // counter for repeated reading
        static unsigned long T25-TempSensor::lastTime = 0;                           // variable for last time point

        static void T25-TempSensor::enable(byte _pin)               //enable data reading - INT0,INT1 name, number 2 or 3 for UNO 
        {
           IrqPin = _pin;
           pinMode(_pin, INPUT_PULLUP);    // input for RF receiver
           attachInterrupt(digitalPinToInterrupt(_pin), IRQhandler, CHANGE); // IRQ handler for RF receiver
        }
             
        static void T25-TempSensor::disable()                      //disable daata reading
        {
           detachInterrupt(digitalPinToInterrupt(IrqPin));          // stop IRQ
        }
        
        static boolean T25-TempSensor::isData(byte channel)         // are read data for sensor with channel
        {
          if(data[channel-1] & 0xFFCFFFFF)                          // are set any data
          {
            return TRUE;
          }
          else 
          {
            return FALSE;
          }
        }
        
        static byte T25-TempSensor::getSID(byte channel);           //get sensor ID for sensor with set channel
        {
          return decodeSID(data[channel-1]);
        }
        
        static float T25-TempSensor::getTemperature(byte channel);  //get temperature for sensor with set channel
        {
          return decodeTemp(data[channel-1]);
        }
        
        static long T25-TempSensor::getDataAge(byte channel);       //get age of data
        {
          return (millis() - age[channel-1])/1000;                  // age in seconds
        }
        
        static void T25-TempSensor::IRQhandler()          // IRQ handler
        {
            unsigned long _currentTime = micros();        // shot time
            int _duration = _currentTime - lastTime;      // calculate duration
            lastTime = _currentTime;                      // memory curent time
            boolean _state = digitalRead(IRQ_PIN);        // read input status
            if (!_state)                                  // goes from HIGH to LOW 
            {                                            
                space = isImpuls(_duration - IMPULSE, ITRESHOLD);     // if impuls time is valid
            }    
            else if (space)                              // goes from LOW to HIGH
            {                                        
              if (!start)                                // start flag is false
              {                                         
                start = isImpuls(_duration - PAUSE, STRESHOLD);  // start temperature data it was pause signal
              } 
              else                                       // start flag is true
              {
                bit0 = isImpuls(_duration - BIT0, STRESHOLD);  // is it bit 0
                bit1 = isImpuls(_duration - BIT1, STRESHOLD);  // is it bit 1
                if(bit0 ^ bit1)                               // XOR / only one is true
                {
                  bit1 && bitSet(buffer[repeat],DATA_LONG - bits - 1);   // write bit 1
                  bit0 && bitClear(buffer[repeat],DATA_LONG - bits - 1); // write bit 0
                  bits++;                                              // increment bit counter
                } 
                else                                          // both or nothing bit 
                {
                  resetTWord ();                              // reset reading
                }
                if(bits >= DATA_LONG)                         // last bit was reading
                {
                  if(repeat > 0)                              // is it temp sentence hienr then 0
                  {
                    if (buffer[repeat] != buffer[repeat-1])       // current temp word is differnt than previous 
                    {
                      resetTWord ();                          // reset reading
                    }
                  }
                  repeat++;                                   // increment temp word reading
                  if (repeat >= BUFF_ROW)                     // buffer is full, save data word
                  {
                    for(int i = 0; i < SENSOR_COUNT;i++)
                    {
                      if((data[i] & CHA_MASK) == (buffer[0] & CHA_MASK))
                      {
                        data[i] == buffer[0];
                        age[i] == millis(); 
                      }  
                    }
                    resetTWord();
                  }                  
                  newTWord();                                 // reset counters and flags
                }
              }  
            }
          }
        } 


        static T25-TempSensor::getBits(unsigned long buffer, unsigned long mask, byte shift)    // get bit by mask from data word and shist it to right
        {
          return ((data & mask) >> shift);  
        }
        
        static byte decodeSID(unsigned long buff)                  // decode sensor ID from raw data
        {
          return getBits(buff, SID_MASK, SID_SHIFT);
        }
        
        static byte decodeCHAN(unsigned long buff)                 // decode sensor channel from raw data
        {
          return getBits(buff, CHA_MASK, CHA_SHIFT) + 1;
        }  
        
        static float decodeTEMP(unsigned long buff)                // decode temperature from raw data
        {
          int t = getBits(buff, TEM_MASK, TEM_SHIFT);
          if (t > 2047) {t = t - 4095;}
          return float(t)/10;
        }
        
        static void T25-TempSensor::resetTWord ()                                  // reset reading
        {
          newTWord();                                       // reset counter and flags
          repeat = 0;                                       // reset temp word counter

        }
        
        static void T25-TempSensor::newTWord ()                                    // reset temp word counter
        {
          bits = 0;                                         // bits counter
          space = LOW;                                      // space is measured
          start = LOW;                                      // starting data reading
        }
        
        static bool T25-TempSensor::isImpuls(int _duration, byte TRESHOLD)          // is it valid impulse
        {
          return (abs(duration) <= TRESHOLD);
        }