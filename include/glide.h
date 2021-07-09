#ifndef MY_GLIDE
#define MY_GLIDE
//#include "spline.h"

const uint8_t CURVE_RESOLUTION = 30;

const uint16_t curveDataAx[CURVE_RESOLUTION] PROGMEM = {158, 312, 462, 608, 750, 888, 1022, 1152, 1278, 1400, 1518, 1632, 1742, 1848, 1950, 2048, 2142, 2232, 2318, 2400, 2478, 2552, 2622, 2688, 2750, 2808, 2862, 2912, 2958, 3000};
const uint16_t curveDataAy[CURVE_RESOLUTION] PROGMEM = {1, 3, 5, 6, 8, 10, 13, 15, 17, 20, 23, 26, 29, 32, 35, 38, 42, 46, 49, 53, 57, 62, 66, 70, 75, 80, 85, 90, 95, 100};
const uint16_t curveDataBx[CURVE_RESOLUTION] PROGMEM = {42, 88, 138, 192, 250, 312, 378, 448, 522, 600, 682, 768, 858, 952, 1050, 1152, 1258, 1368, 1482, 1600, 1722, 1848, 1978, 2112, 2250, 2392, 2538, 2688, 2842, 3000};
const uint16_t curveDataBy[CURVE_RESOLUTION] PROGMEM = {5, 10, 15, 20, 25, 30, 34, 38, 43, 47, 51, 54, 58, 62, 65, 68, 71, 74, 77, 80, 83, 85, 87, 90, 92, 94, 95, 97, 99, 100};
const uint16_t curveDataCx[CURVE_RESOLUTION] PROGMEM = {1, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000};
const uint16_t curveDataCy[CURVE_RESOLUTION] PROGMEM = {3, 7, 10, 13, 17, 20, 23, 27, 30, 33, 37, 40, 43, 47, 50, 53, 57, 60, 63, 67, 70, 73, 77, 80, 83, 87, 90, 93, 97, 100};

enum CurveType
{
  CURVE_A = 0,
  CURVE_B,
  CURVE_C,
  CURVE_D
};

class Glide
{
private:
  uint32_t startTime;  // when the note started playing
  uint16_t noteTime;   // delay before starting glide
  uint16_t portamento; // how long to glide/bend for
  int16_t pitch1;      // initial pitch
  int16_t pitch2;      // final to pitch
  int16_t glideScale;  // pitch range
  CurveType curveType = CurveType::CURVE_B;

  //uint16_t *curveData = pgm_get_far_address(curveDataAx);
  //Spline* spline;
public:
  Glide()
  {
    //spline = new Spline(CURVE_RESOLUTION);
    //this->setCurve(CURVE_A);
  }

  void setCurve(CurveType curveType)
  {
    this->curveType = curveType;
    //spline->setCurve(curveType);
  }

  void viewCurveData()
  {
    for (byte x = 0; x < 30; x++)
    {
      Serial.print(curveDataAx[x]);
      Serial.print(",");
    }
  }

  void begin(int gate, float portoPercent, int pitch1, int pitch2)
  {
    this->pitch1 = pitch1;
    this->pitch2 = pitch2;
    this->startTime = millis();

    // setup timings
    this->portamento = gate * portoPercent;
    this->noteTime = gate - this->portamento;
    this->glideScale = pitch2 - pitch1;
  }

  uint16_t getPitch()
  {
    if ((glideScale == 0) || (portamento == 0))
      return pitch2;

    unsigned long now = millis();
    unsigned long elapsed = now - startTime;

    if (elapsed <= portamento)
    { // ---- gliding
      float position = (float)elapsed / (float)portamento;
      //int pitch = pitch1 + spline->getPointAtX(position) * glideScale;
      uint16_t pitch = pitch1 + getPointAtX(position) * glideScale;
      return pitch;
    }
    else
    {
      return pitch2; // ---- glide complete
    }
  }

  double getPointAtX(double xPercent)
  {
    //return ConstrainedSpline((double *)curveDataAx, (double *)curveDataAy, CURVE_RESOLUTION, xPercent * CURVE_RESOLUTION);
    return SmoothStep(xPercent * CURVE_RESOLUTION);
  }

  float getCurveX(uint8_t k)
  {
    switch (curveType)
    {
    case CURVE_A:
      return pgm_read_word_near(curveDataAx + k) / 100.0;
    case CURVE_B:
      return pgm_read_word_near(curveDataBx + k) / 100.0;
    case CURVE_C:
      return pgm_read_word_near(curveDataCx + k) / 100.0;
    case CURVE_D:
      break;
    }
    return 0;
  }

  float getCurveY(uint8_t k)
  {
    switch (curveType)
    {
    case CURVE_A:
      return pgm_read_word_near(curveDataAy + k) / 100.0;
    case CURVE_B:
      return pgm_read_word_near(curveDataBy + k) / 100.0;
    case CURVE_C:
      return pgm_read_word_near(curveDataCy + k) / 100.0;
    case CURVE_D:
      break;
    }
    return 0;
  }

  double SmoothStep(float pointX, bool trim = true)
  {
    if (trim)
    {
      if (pointX <= getCurveX(0))
        return getCurveY(0);
      if (pointX >= getCurveX(CURVE_RESOLUTION - 1))
        return getCurveY(CURVE_RESOLUTION - 1);
    }

    auto i = 0;
    if (pointX <= getCurveX(0))
      i = 0;
    else if (pointX >= getCurveX(CURVE_RESOLUTION - 1))
      i = CURVE_RESOLUTION - 1;
    else
      while (pointX >= getCurveX(i + 1))
        i++;
    if (pointX == getCurveX(i + 1))
      return getCurveY(i + 1);

    auto t = (pointX - getCurveX(i)) / (getCurveX(i + 1) - getCurveX(i));
    t = t * t * (3 - 2 * t);
    return getCurveY(i) * (1 - t) + getCurveY(i + 1) * t;
  }
};

#endif