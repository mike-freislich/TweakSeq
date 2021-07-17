#ifndef MY_SPLINES
#define MY_SPLINES

/*
struct Curve
{
  float x[3], y[3];
};

enum CurveType
{
  CURVE_A = 0,
  CURVE_B,
  CURVE_C
};


//#include <InterpolationLib.h>

class Spline
{
private:
  uint8_t amp = 1;
  uint8_t samples = 30;
  float* xValues;
  float* yValues;
  CurveType curveType;
  
  Curve curve[3] = {
      {{0.0, 0.8, 1.0}, {0.0, 0.2, 1.0}}, // curve A
      {{0.0, 0.2, 1.0}, {0.0, 0.8, 1.0}}, // curve B
      {{0.0, 0.5, 1.0}, {0.0, 0.5, 1.0}}  // curve C
  };

  void splineFromPoints(int n, float Px[], float Py[], int w, int h, float *xValues, float *yValues)
  {
    double step = 1.0 / w, t = step;
    double Pxi[n], Pyi[n];
    double scPx[n], scPy[n];

    for (int i = 0; i < n; i++)
    {
      scPx[i] = Px[i] * w;
      scPy[i] = Py[i] * h;
    }

    for (int k = 0; k < w; k++)
    {
      for (int s = 0; s < n; s++)
      {
        Pxi[s] = scPx[s];
        Pyi[s] = scPy[s];
      }

      for (int j = 2; j > 0; j--)
      {
        for (int i = 0; i < j; i++)
        {
          Pxi[i] = ((1.0 - t) * Pxi[i]) + (t * Pxi[i + 1]);
          Pyi[i] = ((1.0 - t) * Pyi[i]) + (t * Pyi[i + 1]);
        }
      }
      xValues[k] = Pxi[0];
      yValues[k] = Pyi[0];
      t += step;
    }
  }

public:
  Spline(int resolution)
  {
    samples = resolution;
    xValues = new float[samples];
    yValues = new float[samples];
    setCurve((CurveType)1);
  }

  void setCurve(CurveType curveIndex)
  {
    if (curveType == curveIndex)
      return;
    curveType = curveIndex;    
    splineFromPoints(3, curve[curveType].x, curve[curveType].y, samples, amp, xValues, yValues);

    //Serial.print("x: ");
    for (uint8_t x = 0; x < 30; x++)
    {
      Serial.print(xValues[x]);
      Serial.print(",");
    }
    Serial.println();

    Serial.print("y: ");
    for (uint8_t y = 0; y < 30; y++)
    {
      Serial.print(yValues[y]);
      Serial.print(",");
    }
    Serial.println();
  }

  double getPointAtX(double xPercent)
  {
    return Interpolation::ConstrainedSpline((double *)xValues, (double *)yValues, samples, xPercent * samples);
  }
};
*/
#endif