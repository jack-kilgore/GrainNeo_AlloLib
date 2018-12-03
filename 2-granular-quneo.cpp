//make a load_dir
// git pull
// git submodule update --init --recursive
  // in KARLS MAT240 repo, this will update the allolib repo within it 
//make a randomizer when playback pos is idol 
// conenct QuNeo to paramters along with a GUI 
  //can send feedback back to QUNEO LEDS???? 
//ask karl how to output speaker separation 
//use type int Parameter 
  //ParameterInt 
    //ABOVE IS HOW YOU DO THIS 

#include "Gamma/SoundFile.h"
using namespace gam;

#include "al/core.hpp"
#include "al/util/imgui/al_Imgui.hpp"
#include "al/util/ui/al_ControlGUI.hpp"
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
      cout << "We could not read " << fileName << "!" << endl;
      exit(1);
    }

    Array *buffer = new Array();
    buffer->size = soundFile.frames()*soundFile.channels();
    buffer->data = new float[buffer->size];
    soundFile.read(buffer->data,buffer->size);
    this->soundClip.push_back(buffer);
      
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
    grain.resize(1000); //wont create more than 1000 grains 
  }

  int activeGrainCount = 0;

  //tweakable parameters 
  int whichClip = 0;   //(0, source.size())
  float grainDuration = 0.25; //in seconds 
  float startPosition = 0.25; // (0,1)
  float peakPosition = 0.1;   // (0,1)
  float amplitudePeak = 0.9;  // (0,1)
  float playbackRate = 0;     // (-1,1)
  float startRandomRange = 1.0;

  //this governs the rate at which grains are created 
  Edge grainBirth; 

  //this method makes a new grain out of a dead/inactive one. 
  // 
  void recycle(Grain& g){
    //choose sound clip the grain pulls from 
    g.source = soundClip[whichClip];

    //startTime and endTime are in units of sample 
    float startTime = g.source->size * startPosition; // rnd::uniformi(1.0,startRandomRange); //how to put var in here
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

ParameterInt Y {"location", "vSliders/3", 0, "quneo", -1, 1};


struct MyApp : public App {
   bool show_gui = true;
   float background = 0.21;
   Granulator granulator;
   Recv server;

  void onCreate() override {
     initIMGUI();
     server.open(9020,"localhost",0.05);
     server.handler(*this);
     server.start();
     //ParameterServer() << Y;
     parameterServer().addListener("127.0.0.1", 9020);


     granulator.load("0.wav");
     granulator.load("1.wav");
     granulator.load("2.wav");
     granulator.load("3.wav");
     granulator.load("4.wav");
     granulator.load("panlinespoonDrop.aiff");
  }

  void onAnimate(double dt) override {
    // pass show_gui for use_input param to turn off interactions
    // when not showing gui
    beginIMGUI_minimal(show_gui);
    navControl().active(!imgui_is_using_input());
  }

  void onDraw(Graphics& g) override {
    g.clear(background);
    ImGui::Text("Active Grains: %3d", granulator.activeGrainCount);
    ImGui::SliderFloat("Background", &background, 0, 1);

    ImGui::SliderInt("Sound Clip", &granulator.whichClip, 0, 5);
    ImGui::SliderFloat("Start Position", &granulator.startPosition, 0, 1);
    ImGui::SliderFloat("Playback Rate", &granulator.playbackRate, -1, 1);
    ImGui::SliderFloat("Start Positon Randomness", &granulator.startRandomRange,1, 12);

    static float volume = -7;
    ImGui::SliderFloat("Loudness", &volume, -42, 0);
    granulator.amplitudePeak = dbtoa(volume);

    ImGui::SliderFloat("Envelop Parameter", &granulator.peakPosition, 0, 1);
    ImGui::SliderFloat("Grain Duration", &granulator.grainDuration, 0.001, 3);

    static float midi = 10;
    ImGui::SliderFloat("Birth Frequency", &midi, -16, 85);
    granulator.grainBirth.frequency(mtof(midi));

    endIMGUI_minimal(show_gui);
  }

  void onSound(AudioIOData& io) override {
    while (io()) {
      // this inner code block runs once per sample
      float s = granulator();
      io.out(0) = s;
      io.out(1) = s;
    }
  }

  void onKeyDown(const Keyboard& k) override {
    if (k.key() == 'g') {
      show_gui = !show_gui;
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
    float v = (val/127.0f)*3;
    if(v < 0.0001) v = 0.0001;
      cout << "vSlider3 Location: "<< v << endl;
      granulator.grainDuration = v;

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

  void onExit() override { shutdownIMGUI(); }
};


int main() {
  MyApp app;
  app.initAudio(::SAMPLE_RATE, ::BLOCK_SIZE, ::OUTPUT_CHANNELS, ::INPUT_CHANNELS);
  app.start();
  return 0;
}