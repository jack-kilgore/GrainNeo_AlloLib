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

struct FloatPair {
  float l, r;
};

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
    float pan;

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
///quneo/upButton/0/note_velocity
  //tweakable parameters 
  /*
    ParameterInt whichClip{"note_velocity", "upButton/0", 0, "quneo", 0, 8};
    //ParameterInt whichClip{"note_velocity", "downButton/0", 0, "quneo", 0, 8};
    //(0, source.size())
    ParameterInt grainDuration{"width", "longSlider", 60, "quneo", 0, 127};
    ParameterInt startPosition{"location", "longSlider", 60, "quneo", 0, 127};
    Parameter peakPosition{"/envelope", "", 0.1, "", 0.0, 1.0};
    Parameter amplitudePeak{"/amplitude", "", 0.707, "", 0.0, 1.0};
    Parameter panPosition{"/pan", "", 0.5, "", 0.0, 1.0};
    ParameterInt playbackRate{"location", "vSliders/3", 0.0, "quneo", 0, 127};
    Parameter birthRate{"/frequency", "", 55, "", 0, 200};
    ParameterInt PositionRandRange{"location", "hSliders/3", 0, "quneo", 0, 127};
  */

  int whichClip = 0;   //(0, source.size())
  float grainDuration = 0.01; //in seconds 
  float startPosition = 0.25; // (0,1)
  float peakPosition = 0.1;   // (0,1)
  float amplitudePeak = 0.9;  // (0,1)
  float playbackRate = 0;     // (-1,1)
  float PositionRandRange = 1.1;
  float panPosition = 0.5;
  float birthRate = 55;


  //this governs the rate at which grains are created 
  Edge grainBirth; 

  //this method makes a new grain out of a dead/inactive one. 
  // 
  void recycle(Grain& g) {
    // choose which sound clip this grain pulls from
    g.source = soundClip[whichClip];
    // startTime and endTime are in units of sample
    float startTime = g.source->size * startPosition * rnd::uniform(1.0,PositionRandRange/1.0);
    float endTime =
        startTime + grainDuration * ::SAMPLE_RATE * powf(2.0, playbackRate);

    g.index.set(startTime, endTime, grainDuration);

    // riseTime and fallTime are in units of second
    float riseTime = (grainDuration) * peakPosition;
    float fallTime = (grainDuration) - riseTime;
    g.envelope.set(riseTime, fallTime, amplitudePeak);

    g.pan = panPosition;

    // permit this grain to sound!
    g.active = true;
  }

  //make the next sample 
  FloatPair operator()() {
    // figure out if we should generate (recycle) more grains; then do so.
    //
    grainBirth.frequency(birthRate);
    if (grainBirth()) {
      for (Grain& g : grain)
        if (!g.active) {
          recycle(g);
          break;
        }
    }

    // figure out which grains are active. for each active grain, get the next
    // sample; sum all these up and return that sum.
    //
    float left = 0, right = 0;
    activeGrainCount = 0;
    for (Grain& g : grain)
      if (g.active) {
        activeGrainCount++;
        float f = g();
        left += f * (1 - g.pan);
        right += f * g.pan;
      }
    return {left, right};
  }

};




struct MyApp : public App {
   bool show_gui = true;
   float background = 0.21;
   Granulator granulator;
   ControlGUI gui;
   PresetHandler presetHandler{"GranulatorPresets"};
   PresetServer presetServer{"0.0.0.0", 9011};
   Recv server;

  void onCreate() override {
    initIMGUI();
    parameterServer().print();
    
    granulator.load("0.wav");
    granulator.load("1.wav");
    granulator.load("2.wav");
    granulator.load("3.wav");
    granulator.load("4.wav");
    granulator.load("panlinespoonDrop.aiff");

    server.open(9020,"localhost",0.05);
    server.handler(*this);
    server.start();
    parameterServer().addListener("127.0.0.1", 9020);



    /*gui.init();
    gui << granulator.whichClip << granulator.grainDuration
        << granulator.startPosition << granulator.peakPosition
        << granulator.amplitudePeak << granulator.panPosition
        << granulator.playbackRate << granulator.birthRate 
        << granulator.PositionRandRange;

    parameterServer() << granulator.whichClip << granulator.grainDuration
                      << granulator.startPosition << granulator.peakPosition
                      << granulator.amplitudePeak << granulator.panPosition
                      << granulator.playbackRate << granulator.birthRate
                      << granulator.PositionRandRange;
                      */
    
  }

  void onAnimate(double dt) override {
    // pass show_gui for use_input param to turn off interactions
    // when not showing gui
    beginIMGUI_minimal(show_gui);
    navControl().active(!gui.usingInput());
  }

  void onDraw(Graphics& g) override {
    g.clear(background);

    ImGui::Text("Active Grains: %3d", granulator.activeGrainCount);
    ImGui::SliderFloat("Background", &background, 0, 1);

    ImGui::SliderInt("Sound Clip", &granulator.whichClip, 0, 5);
    ImGui::SliderFloat("Start Position", &granulator.startPosition, 0.0, 1.0);
    ImGui::SliderFloat("Playback Rate", &granulator.playbackRate, -5, 5);
    ImGui::SliderFloat("Start Positon Randomness", &granulator.PositionRandRange,1, 2);

    static float volume = -7;
    ImGui::SliderFloat("Loudness", &granulator.amplitudePeak, 0.0, 1.0);
    //granulator.amplitudePeak = dbtoa(volume);
    
    ImGui::SliderFloat("Envelop Parameter", &granulator.peakPosition, 0, 1);
    ImGui::SliderFloat("Grain Duration", &granulator.grainDuration, 0.001, 3.0);

    static float midi = 10;
    ImGui::SliderFloat("Birth Frequency", &midi, -16, 85);
    granulator.grainBirth.frequency(mtof(midi));

    endIMGUI_minimal(show_gui);
    //gui.draw(g);
  }

  void onSound(AudioIOData& io) override {
    while (io()) {
      FloatPair p = granulator();
      io.out(0) = p.l;
      io.out(1) = p.r;
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
    // map certain positions on the pads 
  void onMessage(osc::Message& m) override {
    m.print();
    if(m.addressPattern() == "/quneo/vSliders/0/location")
    {
      int val;
      m >> val;
      float v = (val/127.0f);
      if(v < 0.001) v = 0.001;
      //cout << "Loudness: "<< v << endl;
      granulator.amplitudePeak = v;
    }
    if(m.addressPattern() == "/quneo/vSliders/3/location")
    {
      int val;
      m >> val;
      float v = (val/127.0f)+1.0;
      
      //cout << "PositionRandRange: "<< v << endl;
      granulator.PositionRandRange = v;
    }
     if(m.addressPattern() == "/quneo/vSliders/2/location")
    {
      int val;
      m >> val;
      float v = val/1.0;
     // cout << "grainBirth " << v << endl;
      granulator.grainBirth.frequency(mtof(v));
    }
    if(m.addressPattern() == "/quneo/hSliders/3/location")
    {
      int val;
      m >> val;
      float v = (10*((val/127.0f))-5);
      //if(v < 0.001) v = 0.001;
      //cout << "Loudness: "<< v << endl;
      granulator.playbackRate = v;
    }
    if(m.addressPattern() == "/quneo/vSliders/1/location")
    {
		int val;
		m >> val;
    float v = (val/127.0f);
    if(v < 0.001) v = 0.001;
      //cout << "Grain_Duration: "<< v << endl;
      granulator.grainDuration = v;

    }
    if(m.addressPattern() == "/quneo/longSlider/location")
    {
      int val;
      m >> val;
      float v = (val/127.0f); 
      //cout << "startPosition: "<< v << endl;
      granulator.startPosition = v;

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







 