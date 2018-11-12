#ifndef __240C_SYNTHS__
#define __240C_SYNTHS__

#include <cmath>

namespace diy {

const int SAMPLE_RATE = 44100;
const int BLOCK_SIZE = 512;
const int OUTPUT_CHANNELS = 2;
const int INPUT_CHANNELS = 2;

struct Phasor {
  float phase = 0.0;        // on the interval [0, 1)
  float increment = 0.001;  // led to an low F

  void frequency(float hertz) { increment = hertz / SAMPLE_RATE; }

  float operator()() {
    phase += increment;
    if (phase > 1) phase -= 1;
    if (phase < 0) phase += 1;
    return phase;
  }
};

struct Saw : Phasor {
  float operator()() { return Phasor::operator()() * 2 - 1; }
};
struct Tri : Phasor {
  float operator()() {
    float f = Phasor::operator()();
    return ((f < 0.5) ? f : 1 - f) * 4 - 1;
  }
};
struct Rect : Phasor {
  float dutyCycle = 0.5;
  float operator()() { return (Phasor::operator()() < dutyCycle) ? -1 : 1; }
};
struct Sine : Phasor {
  // how much memory does this cost?
  float data[200000];
  // a float is 4 bytes
  // 200_000 * 4 = 800_000 bytes or 0.8 MB.

  Sine() {
    for (int i = 0; i < 200000; i++) data[i] = sin(2 * M_PI * i / 200000);
  }

  float operator()() {
    float phase = Phasor::operator()();
    return data[int(phase * 200000)];
  }
};
struct Biquad {
  // x[n-1], x[n-2], y[n-1], y[n-2]
  float x1, x2, y1, y2;
  // (normalized) filter coefficients
  float b0, b1, b2, a1, a2;
  float operator()(float x0) {
    // Direct Form 1 with normalized coefficients
    float y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    y2 = y1;
    y1 = y0;
    x2 = x1;
    x1 = x0;
    return y0;
  }
  void normalize(float a0) {
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
  }
  // but.... How do we choose the magic numbers, the coefficients?
  // We can look in the "audio eq cookbook" and/or we can open Max

  void lpf(float f0, float Q) {
    float w0 = 2 * M_PI * f0 / SAMPLE_RATE;
    float alpha = sin(w0) / (2 * Q);

    b0 = (1 - cos(w0)) / 2;
    b1 = 1 - cos(w0);
    b2 = (1 - cos(w0)) / 2;
    a1 = -2 * cos(w0);
    a2 = 1 - alpha;

    normalize(1 + alpha);
  }
};

struct Edge {
  float phase = 0.0;        // on the interval [0, 1)
  float increment = 0.001;  // led to an low F

  void frequency(float hertz) { increment = hertz / SAMPLE_RATE; }
  void period(float seconds) { frequency(1 / seconds); }

  bool operator()() {
    phase += increment;
    if (phase > 1) {
      phase -= 1;
      return true;
    }
    return false;
  }
};

struct Line {
  float value, target, seconds;
  float increment;

  void set() {
    // slope per sample
    increment = (target - value) / (seconds * SAMPLE_RATE);
  }
  void set(float v, float t, float s) {
    value = v;
    target = t;
    seconds = s;
    set();
  }
  void set(float t, float s) {
    target = t;
    seconds = s;
    set();
  }
  void set(float t) {
    target = t;
    set();
  }

  float operator()() {
    if (value != target) {
      value += increment;
      if ((increment < 0) ? (value < target) : (value > target)) value = target;
    }
    return value;
  }
};

/*
// pattern for later
struct Reverb {
  float operator()(float f) { return makeReverbHappenTo(f); }
}
*/



}  // namespace diy
#endif  // __240C_SYNTHS__
