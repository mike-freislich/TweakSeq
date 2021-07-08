#ifndef MY_GLIDE
#define MY_GLIDE
#include "spline.h"
const byte CURVE_RESOLUTION=20;
class Glide {
  private:
    unsigned long startTime;    // when the note started playing
    unsigned int noteTime;      // delay before starting glide
    unsigned int portamento;     // how long to glide/bend for
    unsigned int pitch1;        // initial pitch
    unsigned int pitch2;        // final to pitch
    int glideScale;             // pitch range
    Spline* spline;
  public:
    

    Glide() {
      spline = new Spline(CURVE_RESOLUTION);
      this->setCurve(CURVE_A);
    }

    void setCurve(CurveType curveType) {
      spline->setCurve(curveType);
    }

    void begin(int gate, float portoPercent, int pitch1, int pitch2) {      
      this->pitch1    = pitch1;
      this->pitch2    = pitch2;
      this->startTime = millis();
      
      // setup timings
      this->portamento = gate * portoPercent;
      this->noteTime = gate - this->portamento;
      this->glideScale = pitch2 - pitch1;
    }

    int getPitch() {
      if ((glideScale == 0) || (portamento == 0)) return pitch2;

      unsigned long now = millis();
      unsigned long elapsed = now - startTime;

      if (elapsed <= portamento) {        // ---- gliding
        float position = (float)elapsed / (float)portamento;
        int pitch = pitch1 + spline->getPointAtX(position) * glideScale;
        return pitch;
      } else {
        return pitch2;                    // ---- glide complete
      }
    }
};

#endif