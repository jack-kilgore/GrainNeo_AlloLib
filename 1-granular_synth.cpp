#include <iostream>
#include <string>
#include "al/core.hpp"
#include "Gamma/Oscillator.h"
#include "Gamma/Envelope.h"
#include "Gamma/Domain.h"
#include "synths.h"

using namespace al;
using namespace std;
using namespace diy;
using namespace osc;


const int SAMPLE_RATE = 44100;
const int BLOCK_SIZE = 512;
const int OUTPUT_CHANNELS = 2;
const int INPUT_CHANNELS = 2;


// define new classes (aka struct) here
//
//SoundFile soundFile;

struct MyApp : public App {
   double frequency;
   gam::Sine<> sig;
   Line line;
   Edge edge;
   Recv server;

  // put "class" variables here
  //
  


  // called once, before any other callbacks
  //
  void onCreate() override {
     gam::Domain::master().spu(audioIO().framesPerSecond());
     sig.freq(120);
     line.set(1,0,0.3);
     edge.period(0.2);
     server.open(9020,"localhost",0.05);
     server.handler(*this);
     server.start();
     frequency = 100;
  }

  // called once per graphics frame ~30Hz
  //
  void onDraw(Graphics& g) override {
    g.clear(0);
    //
  }

  // called once per audio frame ~86Hz (SAMPLE_RATE / BLOCK_SIZE)
  //
  void onSound(AudioIOData& io) override {
    while (io()) {
      // this inner code block runs once per sample
      sig.freq(440);
      float s = sig()*0.2;
      io.out(0) = s;
      io.out(1) = s;
    }
  }

  void onMessage(osc::Message& m) override {
    //m.print();
    if(m.addressPattern() == "/quneo/vSliders/3/location")
    {
		int val;
		m >> val;
      cout << "vSlider3 Location: "<< val << endl;
      //m >> frequency;

    }
    if(m.addressPattern() == "/quneo/longSlider/width")
    {
      int val;
      m >> val;
      cout << "longSlider Width: "<< val << endl;
    }
    if(m.addressPattern() == "/quneo/pads/3/drum/pressure")
    {
      int val;
      m >> val;
      cout << "drumPad Pressure: "<< val << endl;
    }

  }
};


int main() {
  MyApp app;
  app.initAudio(::SAMPLE_RATE, ::BLOCK_SIZE, ::OUTPUT_CHANNELS, ::INPUT_CHANNELS);
  app.start();
  return 0;
}






 