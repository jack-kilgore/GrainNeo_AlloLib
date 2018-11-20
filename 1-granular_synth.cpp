//make a load_dir 
//make a randomizer when playback pos is idol 
// conenct QuNeo to paramters along with a GUI 
  //can send feedback back to QUNEO LEDS???? 

#include "Gamma/SoundFile.h"
using namespace gam;

#include "al/core.hpp"
#include "al/util/imgui/al_Imgui.hpp"
#include "al/util/ui/al_Parameter.hpp"
using namespace al;
using namespace osc;

#include "synths.h"
using namespace diy;

#include <vector>
using namespace std;




const int SAMPLE_RATE = 44100;
const int BLOCK_SIZE = 512;
const int OUTPUT_CHANNELS = 2;
const int INPUT_CHANNELS = 2;

struct Granulator {

  void load(string fileName){
    SearchPaths searchPaths; 
    searchPaths.addSearchPath("..");

    string filePath = searchPaths.find(fileName).filepath();
    SoundFile soundFile;
    soundFile.path(filePath);
    if(!soundFile.openRead()){
      cout << "We could not read " << fileName << "1" << endl;
      exit(1);
    }
    if(soundFile.channels() != 1){
      cout << fileName << " is not a mono file." << endl;
      exit(1);
    }

    Array *a = new Array();
    a->size = soundFile.frames();
    a->data = new float[a->size];
    soundFile.read(a->data, a->size);
    this->soundClip.push_back(a);

    soundFile.close();
  }

  //keep some sound clips in memory 
  vector<Array*> soundClip;

  struct Grain{ 
    Array* source = nullptr;
    Line index;
    AttackDecay envelope;
    bool active = false;

    float operator()(){
      float f = envelope() * source->get(index());

      if(index.done()) active = false;

      return f;
    }
  };

  //storing grains 
  vector<Grain> grain;

  Granulator(){
    //arbitrary fixed number for how many grains we will allocate 
    grain.resize(1000);
  }

  int activeGrainCount = 0;

  //tweakable parameters 
  int whichClip = 0;   //(0, source.size())
  float grainDuration = 0.25; //in seconds 
  float startPosition = 0.25; // (0,1)
  float peakPosition = 0.1;   // (0,1)
  float amplitudePeak = 0.9;  // (0,1)
  float playbackRate = 0;     // (-1,1)

  //this governs the rate at which grains are created 
  Edge grainBirth; 

  //this method makes a new grain out of a dead/inactive one. 
  // 
  void recycle(Grain& g){
    //choose sound clip the grain pulls from 
    g.source = soundClip[whichClip];

    //startTime and endTime are in units of sample 
    float startTime = g.source->size * startPosition;
    float endTime = startTime + grainDuration * ::SAMPLE_RATE;
    float t = pow(2.0, playbackRate) * grainDuration * ::SAMPLE_RATE;
    startTime -= t/2;  //ask Karl about these two lines
    endTime =+ t/2;
    g.index.set(startTime, endTime, grainDuration); //index refers to the line fn within grain 

    //riseTime and fallTime are in units of second 
    float riseTime = grainDuration * peakPosition;
    float fallTime = grainDuration - riseTime;
    g.envelope.set(riseTime, fallTime, amplitudePeak);

    //permit grain to sound! 
    g.active = true;
  }

  //make the next sample 
  float operator()(){
    //figure out if we should generate more grains
    if(grainBirth()){
      for (Grain& g : grain) //what does this for loop line mean?
        if(!g.active){       //iterate through the vector containing grains?
          recycle(g);
          break;
        }                  

    }

    //figure out which grains are active. for each active grain, get the 
    //next sample; sum all these up and return the sum;

    float f = 0;
    activeGrainCount = 0;
    for(Grain& g : grain)
      if(g.active){
        activeGrainCount++;
        f+= g(); //ask Karl to go through data flow of this line 
      }
    return f;
  }

};

struct MyApp : public App {
   Granulator granulator;
   Recv server;

  void onCreate() override {
     server.open(9020,"localhost",0.05);
     server.handler(*this);
     server.start();
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
      float s = 0;
      io.out(0) = s;
      io.out(1) = s;
    }
  }

//look at interaction tutorial by Andres
  // PARAMETER class 
    //look into how to fit the below name space of QuNeo into 
    // Parameter p {"name", "group", 0.0, "prefix", -1.0f, 1.0f}
  void onMessage(osc::Message& m) override {
    m.print();
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







 