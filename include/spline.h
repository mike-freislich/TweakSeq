#ifndef MY_SPLINES
#define MY_SPLINES

#include <InterpolationLib.h>

struct Curve {
  double x[3], y[3];
};

enum CurveType {
  CURVE_A = 0,
  CURVE_B,
  CURVE_C
};

class Spline {
  private:
    int amp = 1;
    int samples = 30;
    double *xValues;
    double *yValues;
    CurveType curveType;

    Curve curve[3] = {
      { {0.0, 0.8, 1.0}, {0.0, 0.2, 1.0} }, // curve A
      { {0.0, 0.2, 1.0}, {0.0, 0.8, 1.0} }, // curve B
      { {0.0, 0.5, 1.0}, {0.0, 0.5, 1.0} }  // curve C
    };

    void splineFromPoints(int n, double Px[], double Py[], int w, int h, double *xValues, double *yValues) {
      double step = 1.0 / w, t = step;
      double Pxi[n], Pyi[n];
      double scPx[n], scPy[n];

      for (int i = 0; i < n; i++) {
        scPx[i] = Px[i] * w;
        scPy[i] = Py[i] * h;
      }

      for (int k = 0; k < w; k++) {
        for (int s = 0; s < n; s ++) {
          Pxi[s] = scPx[s]; // Pxi.set(scPx);
          Pyi[s] = scPy[s]; // Pyi.set(scPy);
        }

        for (int j = 2; j > 0 ; j--) {
          for (int i = 0; i < j; i++) {
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
    Spline(int resolution) {
      samples = resolution;
      xValues = new double[samples];
      yValues = new double[samples];
      setCurve((CurveType)0);
    }

    void setCurve(CurveType curveIndex) {
      if (curveType == curveIndex) return;
      curveType = curveIndex;
      splineFromPoints(3, curve[curveType].x, curve[curveType].y, samples, amp, xValues, yValues);
    }

    double getPointAtX(double xPercent) {
      return Interpolation::ConstrainedSpline(xValues, yValues, samples, xPercent * samples);
    }
};


#endif