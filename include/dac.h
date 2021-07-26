#ifndef MY_DAC
#define MY_DAC

#include <SPI.h>

const uint8_t DAC_CS   = 10;   // Chip select pin for the DAC

class MP4822
{
public:
  MP4822()
  {
    pinMode(DAC_CS, OUTPUT);
    SPI.begin();
  }

  //function to set state of DAC - input value between 0-4095
  void DAC_set(uint8_t channel, int16_t inputVoltage, uint8_t gain = 0)
  {
    uint16_t input = constrain(inputVoltage, 0, 4000);

    //DAC_sel choose which DAC channel you want to write to A or B
    //Gain_sel choose your gain: H=2xVref and L=1xVref
    uint8_t MSB, LSB; //most sig, least sig bytes and config info

    //convert decimal input to binary stored in two bytes
    MSB = (input >> 8) & 0xFF; //most sig byte
    LSB = input & 0xFF;        //least sig byte

    //apply config bits to the front of MSB
    if (channel == 0)
      MSB &= 0x7F; //writing a 0 to bit 7.
    else if (channel == 1)
      MSB |= 0x80; //writing a 1 to bit 7.

    if (gain == 0)
      MSB |= 0x20;
    else if (gain == 1)
      MSB &= 0x1F;

    MSB |= 0x10; //get out of shutdown mode to active state

    //now write to DAC
    digitalWrite(DAC_CS, LOW); // take the CS pin low to select the chip:
    SPI.transfer(MSB);         //  send in the address and value via SPI:
    SPI.transfer(LSB);
    digitalWrite(DAC_CS, HIGH); // take the CS pin high to de-select the chip:
  }
};

#endif