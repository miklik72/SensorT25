# T25-443MHz-RFtemperature-sensor
Arduino library for receiving temperature sent by commercial
temperature sensor T25 and compatible ones with inside bord TX1-3

Data world
R - random sensor ID 8b 0-255
B - batery state 2b     
C - channel number 2b   0-2 = 1-3
T - temperature 12b     -2048-2047
E - end / extension 12b


     0 ------------------------------------- 35
XX S RRRRRRRR BB CC TTTT TTTTTTTT EEEEEEEE EEEE
XX S 11101001 10 00 0000 11100111 11110000 0000 P1 11101001 1000 0000 11100111 11110000 0000 P2 ... P12
                                       
LOW LONG - space                       
S = 9000us start
1 = 2000us 1 bit        
0 = 1000us 0 bit        
P = 4000us pause        
X = 500us  initial
HIGH IMPULSE                   
I = 500us          
                               
                         bit 0      ->          35
 500 500       9000         1000  2000               4000
 |I|X|I|------- S --------|I|-0|I|-1--|I          I|---P----|I
  _   _                    _    _      _          _          _
 | | | |                  | |  | |    | |        | |        | |
_| |_| |__________________| |__| |____| |_......_| |________| |_

Library SensorT25.h

Library reading data from sensors T25 by channel 1,2 and 3. T25 don't have button for change channel, but inside are contacts next of button for change C/F.
Data for sensors are stored in array and ispossible read it by get functions with channel number as parameter..

Enjoy,
Martin Mikala
dev@miklik.cz