#ifndef MY_DAC
#define MY_DAC
/*
   FUNCTION      NANO   MCP4822
   MOSI           11     4
   MISO           12     5
   SCK            13     3
   CS             10     2
*/

#include <SPI.h>

class MP4822 {
  public:
    MP4822() {}

    void init() {
      pinMode (DAC_CS, OUTPUT);      
      SPI.begin();
    }

    //function to set state of DAC - input value between 0-4095
    void DAC_set(unsigned int inputVoltage, byte DAC_sel, byte Gain_sel, byte CS_pin, String &errmsg)
    {
      
      unsigned int input = constrain(inputVoltage, 0, 4000);
     
      //DAC_sel choose which DAC channel you want to write to A or B
      //Gain_sel choose your gain: H=2xVref and L=1xVref
      byte MSB, LSB; //most sig, least sig bytes and config info

      //clear error messages
      errmsg = "";

      //only run the rest of the code if binary is in range.
      if (input < 0 || input > 4095)
        errmsg += "input out of range. 0-4095.";
      else
      {
        //convert decimal input to binary stored in two bytes
        MSB = (input >> 8) & 0xFF;  //most sig byte
        LSB = input & 0xFF;         //least sig byte

        //apply config bits to the front of MSB
        if (DAC_sel == 0)
          MSB &= 0x7F; //writing a 0 to bit 7.
        else if (DAC_sel == 1)
          MSB |= 0x80; //writing a 1 to bit 7.
        else
          errmsg += "DAC selection out of range. input A or B.";

        if (Gain_sel == 0)
          MSB |= 0x20;
        else if (Gain_sel == 1)
          MSB &= 0x1F;
        else
          errmsg += "Gain selection out of range. input H or L.";
        //delay(10);
        //get out of shutdown mode to active state
        MSB |= 0x10;
        
        //now write to DAC
        // take the CS pin low to select the chip:
        digitalWrite(CS_pin, LOW);
        //delay(1);
        //  send in the address and value via SPI:
        SPI.transfer(MSB);
        SPI.transfer(LSB);
        //delay(1);
        // take the CS pin high to de-select the chip:
        digitalWrite(CS_pin, HIGH);
      }
    }
};

#endif