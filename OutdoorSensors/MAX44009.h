#ifndef MAX44009_h
#define MAX44009_h
#include <Arduino.h>
//#include <I2C.h> // comment this line if using Wire.h

// uncomment this line if you want to use Wire.h instead
#include <Wire.h>

//#define MAX_ADDR 203
#define MAX_ADDR 74 // This really saved me, because it showed that the real address of the MAX44009 was 203 (I forgot to note the address when AD is low, so I’ll document that when I make the improved version of the PCB). 

class MAX44009 {
    public:
        MAX44009();
		
		void begin();
		        
		float get_lux(void);
		
    private:
	
       uint8_t read_register(uint8_t addr);
};



#endif
