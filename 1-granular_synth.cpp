//make a load_dir
// git pull
// git submodule update --init --recursive
 

 //problems with clip drag and drop 
  // cant put bufnum into array size 
  // cant toggle any other sample except 1st 

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

#include <forward_list>
#include <unordered_set>
#include <vector>
#include <typeinfo>
using namespace std;

const int SAMPLE_RATE = 44100;
const int BLOCK_SIZE = 512;
const int OUTPUT_CHANNELS = 2;
const int INPUT_CHANNELS = 2;

template <typename T>
class Bag {
  forward_list<T*> remove, inactive;
  unordered_set<T*> active;

 public:
  void insert_inactive(T& t) { inactive.push_front(&t); }

  bool has_inactive() { return !inactive.empty(); }

  T& get_next_inactive() {
    T* t = inactive.front();
    active.insert(t);
    inactive.pop_front();
    return *t;
  }

  void for_each_active(function<void(T& t)> f) {
    for (auto& t : active) f(*t);
  }

  void schedule_for_deactivation(T& t) { remove.push_front(&t); }

  void execute_deactivation() {
    for (auto e : remove) {
      active.erase(e);
      inactive.push_front(e);
    }
    remove.clear();
  }
};

struct FloatPair {
  float l, r;
};


struct Granulator {
  int bufnum = 0;
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
    bufnum++;
    soundFile.close();
  }
  
  

  //keep some sound clips in memory 
  vector<Array*> soundClip;
  vector<int> activeClipsIndex;
  bool arr[6];
  void make_bool_arr(){
    for(int i=0; i < bufnum; i++) arr[i] = false;
  }

  void clipIndex(){
    //activeClipsIndex.push_back(1);
    if (arr[bufnum]){
      activeClipsIndex.push_back(bufnum);
    }
    if (!arr[bufnum]){
      activeClipsIndex.erase(std::remove(activeClipsIndex.begin(), activeClipsIndex.end(), bufnum), activeClipsIndex.end()); 
    }
    
  }
  
  
  
  

  struct Grain{ 
    Array* source = nullptr;
    Line index;
    AttackDecay envelope;
    float pan;
    float operator()(){
      return envelope() * source->get(index());
    }
  };

  //storing grains 
  vector<Grain> grain;
  Bag<Grain> bag;


  Granulator(){
    //arbitrary fixed number for how many grains we will allocate 
    grain.resize(1000); //wont create more than 1000 grains 
    for (auto& g : grain) bag.insert_inactive(g);
  }

  int activeGrainCount = 0;

  int whichClip = 0;   //(0, source.size())
  float grainDuration = 0.01; //in seconds 
  float startPosition = 0.25; // (0,1)
  float peakPosition = 0.1;   // (0,1)
  float amplitudePeak = 0.9;  // (0,1)
  float playbackRate = 0;     // (-1,1)
  float PositionRandRange = 0.5;
  float panPosition = 0.5;
  float panPosRand = 0;
  float birthRate = 8;
  float *birthRatep = &birthRate;
  float grainRateRand = 0.0;
    float freq = 1;
    float *frequency = &freq;
  float grainDurRand = 0;
 


  //this governs the rate at which grains are created 
  Edge grainBirth; 
  
  //this method makes a new grain out of a dead/inactive one. 
  // 
  void recycle(Grain& g) {
    // //
     if(activeClipsIndex.size()==0) g.source = soundClip[whichClip];
     else g.source = soundClip[activeClipsIndex[0]]; //+ rand()%(activeClipsIndex.size()-1)
    // startTime and endTime are in units of sample
    float startTime = g.source->size * startPosition * rnd::uniformS(PositionRandRange/1.0);
    float endTime =
        startTime + (grainDuration+rnd::uniformS(grainDurRand/1.0)) * ::SAMPLE_RATE * powf(2.0, playbackRate);

    g.index.set(startTime, endTime, grainDuration + rnd::uniformS(grainDurRand/1.0));

    // riseTime and fallTime are in units of second
    float riseTime = (grainDuration+ rnd::uniformS(grainDurRand/1.0)) * peakPosition;
    float fallTime = (grainDuration+ rnd::uniformS(grainDurRand/1.0)) - riseTime;
    g.envelope.set(riseTime, fallTime, amplitudePeak);

    g.pan = panPosition + rnd::uniformS(panPosRand); 
    if (grainRateRand >= birthRate)  grainRateRand = birthRate-1;
    *frequency = *birthRatep + (rnd::uniformS(grainRateRand/1.0));

  }

  //make the next sample 
  FloatPair operator()() {
    // figure out if we should generate (recycle) more grains; then do so.
    //
    grainBirth.frequency(freq); //
     if (grainBirth())
      if (bag.has_inactive()) recycle(bag.get_next_inactive());

    // figure out which grains are active. for each active grain, get the next
    // sample; sum all these up and return that sum.
    //
    float left = 0, right = 0;
    bag.for_each_active([&](Grain& g) {
      float f = g();
      left += f * (1 - g.pan);
      right += f * g.pan;
      if (g.index.done()) bag.schedule_for_deactivation(g);
    });
    bag.execute_deactivation();

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
   Send client;

  void onCreate() override {
    initIMGUI();
    parameterServer().print();
    
    //why cant u load stereo wav, but can load stereo aiff 
    
    granulator.load("magnet3.aiff");
    granulator.load("panlinespoonDrop.aiff");
    granulator.load("steel-wool-on-stove-top.aiff");
    granulator.load("ceramic_shards1-mono.wav");
    granulator.load("TASCAM_0812.wav");
    granulator.load("0.aiff");

    //granulator.make_bool_arr();
    
    client.open(9012,"127.0.0.1");
  }

  void onAnimate(double dt) override {
    // pass show_gui for use_input param to turn off interactions
    // when not showing gui
    beginIMGUI_minimal(show_gui);
    navControl().active(!gui.usingInput());
  }

  void onDraw(Graphics& g) override {
    g.clear(background);
    //granulator.clipIndex();
    //for(int i=0; i < granulator.bufnum; i++) ImGui::Checkbox("Sample", &(granulator.arr[i]));
    ImGui::SliderInt("Sound Clip", &granulator.whichClip, 0,granulator.bufnum-1);

    ImGui::SliderFloat("Pan Position", &granulator.panPosition, 0, 1);
    ImGui::SliderFloat("Randomize Pan Position", &granulator.panPosRand, 0,1);
    ImGui::SliderFloat("Birth Frequency", &granulator.birthRate, 0, 100);
    ImGui::SliderFloat("Randomize Birth Frequency", &granulator.grainRateRand, 0, 100);
    ImGui::SliderFloat("Grain Duration", &granulator.grainDuration, 0.001, 1);
    ImGui::SliderFloat("Randomize Grain Duration", &granulator.grainDurRand, 0, 2);
    ImGui::SliderFloat("Start Position", &granulator.startPosition, 0.0, 1.0);
    ImGui::SliderFloat("Randomize Start Positon", &granulator.PositionRandRange,0, 1);
    ImGui::SliderFloat("Playback Rate", &granulator.playbackRate, -5, 5);
    ImGui::SliderFloat("Loudness", &granulator.amplitudePeak, 0.0, 1.0);
    ImGui::SliderFloat("Envelop Parameter", &granulator.peakPosition, 0, 1);

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


  void onMessage(osc::Message& m) override {
    //m.print(); 

  
    if(m.addressPattern() == "/quneo/hSliders/0/location")
    {
      int val;
      m >> val;
      float v = ((val/127.0f));
      //cout << "Loudness: "<< v << endl;
      granulator.panPosition = v;
    }
    if(m.addressPattern() == "/quneo/hSliders/1/location")
    {
      int val;
      m >> val;
      float v = (val/127.0)*100;
      if(v < 0.5) v=0.5; 
     // cout << "grainBirth " << v << endl;
      granulator.birthRate = v;
    }
    if(m.addressPattern() == "/quneo/hSliders/2/location")
    {
		int val;
		m >> val;
    float v = (val/127.0f);
    if(v < 0.001) v = 0.001;
      //cout << "Grain_Duration: "<< v << endl;
      granulator.grainDuration = v;

    }
    if(m.addressPattern() == "/quneo/hSliders/3/location")
    {
      int val;
      m >> val;
      float v = (10*((val/127.0f))-5);
      //cout << "Loudness: "<< v << endl;
      granulator.playbackRate = v;
    }

    if(m.addressPattern() == "/quneo/rotary/0/direction")
    {
      int val;
      m >> val;
      float v = (val/127.0f);
      if(v < 0.001) v = 0.001;
      //cout << "Loudness: "<< v << endl;
      granulator.amplitudePeak = v;
    }

    if(m.addressPattern() == "/quneo/rotary/1/direction")
    {
      int val;
      m >> val;
      float v = (val/127.0f);
      if(v < 0.001) v = 0.001;
      //cout << "Loudness: "<< v << endl;
      granulator.peakPosition = v;
    }


    if(m.addressPattern() == "/quneo/vSliders/0/location")
    {
      int val;
      m >> val;
      float v = (val/127.0f);
      granulator.panPosRand = v;
    }
    if(m.addressPattern() == "/quneo/vSliders/1/location")
    {
      int val;
      m >> val;
      float v = 100*(val/127.0f);
      
      //cout << "BirthRateRand: "<< v << endl;
      granulator.grainRateRand= v;
    }
    if(m.addressPattern() == "/quneo/vSliders/2/location")
    {
      int val;
      m >> val;
      float v = 2*(val/127.0f);
      
      //cout << "GrainDurRand: "<< v << endl;
      granulator.grainDurRand = v;
    }

    if(m.addressPattern() == "/quneo/vSliders/3/location")
    {
      int val;
      m >> val;
      float v = (val/127.0f);
      
      //cout << "PositionRandRange: "<< v << endl;
      granulator.PositionRandRange = v;
    }
    
    if(m.addressPattern() == "/quneo/longSlider/location")
    {
      int val;
      m >> val;
      float v = (val/127.0f); 
      //cout << "startPosition: "<< v << endl;
      granulator.startPosition = v;

    }

    //---------SampleChange 
    if(m.addressPattern() == "/quneo/upButton/0/note_velocity")
    {
      int val; 
      m >> val; 
      if(granulator.whichClip == granulator.bufnum-1 && val == 127) granulator.whichClip = 0;
      else if(val == 127) granulator.whichClip++;
      
    }

    if(m.addressPattern() == "/quneo/downButton/0/note_velocity")
    {
      int val; 
      m >> val; 
      if(granulator.whichClip == 0 && val == 127) granulator.whichClip = granulator.bufnum-1;
      else if(val == 127) granulator.whichClip--;
    }

    //----------------pad12
    if(m.addressPattern() == "/quneo/pads/12/drum/x")
    {
      int val;
      m >> val;
      float v = (val/127.0)*100;
      if(v < 0.5) v=0.5; 
     // cout << "grainBirth " << v << endl;
      granulator.birthRate = v;
    } 
    if(m.addressPattern() == "/quneo/pads/12/drum/y")
    {
      int val;
      m >> val;
      float v = 100*(val/127.0f);
      
      //cout << "BirthRateRand: "<< v << endl;
      granulator.grainRateRand= v;
    }

    //-----------------pad13
    //broken, sticks to its middle values 
    if(m.addressPattern() == "/quneo/pads/13/drum/x")
    {
      int val;
      m >> val;
      float v = (val/127.0)*100;
      if(v < 0.5) v=0.5; 
     // cout << "grainBirth " << v << endl;
      granulator.birthRate = v;
    } 

    if(m.addressPattern() == "/quneo/pads/13/drum/y")
    {
		int val;
		m >> val;
    float v = (val/127.0f);
    if(v < 0.001) v = 0.001;
      //cout << "Grain_Duration: "<< v << endl;
      granulator.grainDuration = v;

    }

    //--------------pad14
    if(m.addressPattern() == "/quneo/pads/14/drum/x")
    {
      int val;
      m >> val;
      float v = (val/127.0)*100;
      if(v < 0.5) v=0.5; 
     // cout << "grainBirth " << v << endl;
      granulator.birthRate = v;
    } 
    if(m.addressPattern() == "/quneo/pads/14/drum/y")
    {
      int val;
      m >> val;
      float v = (val/127.0f); 
      //cout << "startPosition: "<< v << endl;
      granulator.startPosition = v;

    }

    //--------------pad15
   if(m.addressPattern() == "/quneo/pads/15/drum/x")
    {
      int val;
      m >> val;
      float v = (val/127.0)*100;
      if(v < 0.5) v=0.5; 
     // cout << "grainBirth " << v << endl;
      granulator.birthRate = v;
    } 
    if(m.addressPattern() == "/quneo/pads/15/drum/y")
    {
      int val;
      m >> val;
      float v = (10*((val/127.0f))-5);
      //cout << "Loudness: "<< v << endl;
      granulator.playbackRate = v;
    }

    //-------pad8
    if(m.addressPattern() == "/quneo/pads/8/drum/x")
    {
		int val;
		m >> val;
    float v = (val/127.0f);
    if(v < 0.001) v = 0.001;
      //cout << "Grain_Duration: "<< v << endl;
      granulator.grainDuration = v;

    }
    if(m.addressPattern() == "/quneo/pads/8/drum/y")
    {
      int val;
      m >> val;
      float v = 2*(val/127.0f);
      
      //cout << "GrainDurRand: "<< v << endl;
      granulator.grainDurRand = v;
    }

    //-------------pad9
    if(m.addressPattern() == "/quneo/pads/9/drum/x")
    {
		int val;
		m >> val;
    float v = (val/127.0f);
    if(v < 0.001) v = 0.001;
      //cout << "Grain_Duration: "<< v << endl;
      granulator.grainDuration = v;

    }
    if(m.addressPattern() == "/quneo/pads/9/drum/y")
    {
      int val;
      m >> val;
      float v = (val/127.0f); 
      //cout << "startPosition: "<< v << endl;
      granulator.startPosition = v;

    }

    //---------pad10
    if(m.addressPattern() == "/quneo/pads/10/drum/x")
    {
		int val;
		m >> val;
    float v = (val/127.0f);
    if(v < 0.001) v = 0.001;
      //cout << "Grain_Duration: "<< v << endl;
      granulator.grainDuration = v;

    }
    if(m.addressPattern() == "/quneo/pads/10/drum/y")
    {
      int val;
      m >> val;
      float v = (10*((val/127.0f))-5);
      //cout << "Loudness: "<< v << endl;
      granulator.playbackRate = v;
    }

    //------------pad4
    if(m.addressPattern() == "/quneo/pads/4/drum/x")
    {
      int val;
      m >> val;
      float v = (val/127.0f); 
      //cout << "startPosition: "<< v << endl;
      granulator.startPosition = v;
    }
    if(m.addressPattern() == "/quneo/pads/4/drum/y")
    {
      int val;
      m >> val;
      float v = (val/127.0f);
      
      //cout << "PositionRandRange: "<< v << endl;
      granulator.PositionRandRange = v;
    }
    //------------pad5
    if(m.addressPattern() == "/quneo/pads/5/drum/x")
    {
      int val;
      m >> val;
      float v = (val/127.0f); 
      //cout << "startPosition: "<< v << endl;
      granulator.startPosition = v;
    }
    if(m.addressPattern() == "/quneo/pads/5/drum/y")
    {
      int val;
      m >> val;
      float v = (10*((val/127.0f))-5);
      //cout << "Loudness: "<< v << endl;
      granulator.playbackRate = v;
    }

    //------------pad0
    if(m.addressPattern() == "/quneo/pads/0/drum/x")
    {
      int val;
      m >> val;
      float v = (10*((val/127.0f))-5);
      //cout << "Loudness: "<< v << endl;
      granulator.playbackRate = v;
    }
    if(m.addressPattern() == "/quneo/pads/0/drum/y")
    {
      int val;
      m >> val;
      float v = (val/127.0f);
      granulator.panPosRand = v;
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


 