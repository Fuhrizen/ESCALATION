#include "escalation/audio.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#else
#include <dlfcn.h>
#endif
namespace escalation {
namespace {
constexpr int rate=24000, chunk=600, count=int(SoundCue::Count);
using Clock=std::chrono::steady_clock;
std::vector<float> makeCue(int cue) {
 const double durations[count]={.075,.12,.23,.14,.42,.44,.5,.035};
 const double frequencies[count]={740,480,660,180,540,130,75,1100};
 std::vector<float> samples(int(rate*durations[cue])); uint32_t random=0x87312345u+cue;
 for(size_t i=0;i<samples.size();++i) {
  double t=double(i)/rate, u=t/durations[cue]; random=1664525u*random+1013904223u;
  double noise=double(random)/4294967295.0*2-1;
  double env=std::min(1.0,t/.006)*std::pow(1-u,2.0);
  double frequency=frequencies[cue];
  if(cue==int(SoundCue::Success)||cue==int(SoundCue::Complete)) frequency*=int(u*3)==0?1:(int(u*3)==1?1.25:1.5);
  double signal=std::sin(6.28318530718*frequency*t);
  if(cue==int(SoundCue::Launch)) signal=.65*noise+.35*std::sin(6.28318530718*(130*t+450*t*t));
  if(cue==int(SoundCue::Impact)) signal=.65*noise+.35*std::sin(6.28318530718*75*t);
  samples[i]=float(signal*env*.22);
 }
 return samples;
}
struct Device {
#ifdef _WIN32
 HWAVEOUT output=nullptr;
 bool open() { WAVEFORMATEX f{}; f.wFormatTag=WAVE_FORMAT_PCM; f.nChannels=1; f.nSamplesPerSec=rate; f.wBitsPerSample=16; f.nBlockAlign=2; f.nAvgBytesPerSec=rate*2; return waveOutOpen(&output,WAVE_MAPPER,&f,0,0,CALLBACK_NULL)==MMSYSERR_NOERROR; }
 bool write(int16_t* data,int frames) { WAVEHDR h{}; h.lpData=reinterpret_cast<LPSTR>(data); h.dwBufferLength=frames*2; if(waveOutPrepareHeader(output,&h,sizeof h)!=MMSYSERR_NOERROR)return false; if(waveOutWrite(output,&h,sizeof h)!=MMSYSERR_NOERROR){waveOutUnprepareHeader(output,&h,sizeof h);return false;} auto deadline=Clock::now()+std::chrono::milliseconds(2000+frames*1000/rate); while(!(h.dwFlags&WHDR_DONE)){if(Clock::now()>=deadline){waveOutReset(output);waveOutUnprepareHeader(output,&h,sizeof h);return false;}std::this_thread::sleep_for(std::chrono::milliseconds(2));} waveOutUnprepareHeader(output,&h,sizeof h); return true; }
 void close(){if(output){waveOutReset(output);waveOutClose(output);output=nullptr;}}
#else
 void* library=nullptr; void* output=nullptr;
 int (*pcmOpen)(void**,const char*,int,int)=nullptr;
 int (*params)(void*,int,int,unsigned,unsigned,int,unsigned)=nullptr;
 long (*writeFrames)(void*,const void*,unsigned long)=nullptr;
 int (*recover)(void*,int,int)=nullptr; int (*pcmClose)(void*)=nullptr;
 template<class T> bool bind(T& fn,const char* name){fn=reinterpret_cast<T>(dlsym(library,name));return fn!=nullptr;}
 bool open(){library=dlopen("libasound.so.2",RTLD_NOW|RTLD_LOCAL); if(!library)return false; if(!bind(pcmOpen,"snd_pcm_open")||!bind(params,"snd_pcm_set_params")||!bind(writeFrames,"snd_pcm_writei")||!bind(recover,"snd_pcm_recover")||!bind(pcmClose,"snd_pcm_close"))return false; if(pcmOpen(&output,"default",0,0)<0){output=nullptr;return false;} // ALSA ABI enum values: S16_LE=2, RW_INTERLEAVED=3, PLAYBACK=0.
 return params(output,2,3,1,rate,1,50000)>=0;}
 bool write(int16_t* data,int frames){int consumed=0, retries=0;while(consumed<frames){long n=writeFrames(output,data+consumed,frames-consumed);if(n<0){if(++retries>4||recover(output,int(n),1)<0)return false;}else if(n==0)return false;else {consumed+=int(n);retries=0;}}return true;}
 void close(){if(output&&pcmClose){pcmClose(output);output=nullptr;}if(library){dlclose(library);library=nullptr;}}
#endif
 ~Device(){close();}
};
}
struct Audio::Impl {
 std::atomic<bool> running{false}, ready{false}, muted{false}; std::atomic<float> volume{.65f};
 std::thread worker; std::mutex mutex; std::condition_variable changed;
 std::vector<int> pending; std::array<Clock::time_point,count> last{};
 std::array<std::vector<float>,count> samples;
 struct Voice{int cue;size_t position;};
 void run(){Device device;if(!device.open()){running=false;return;} ready=true;
  std::vector<Voice> voices;std::array<int16_t,chunk> pcm{};
  while(running){
   {std::unique_lock<std::mutex> lock(mutex);if(voices.empty()&&pending.empty())changed.wait(lock,[&]{return !running||!pending.empty();}); for(int cue:pending)if(voices.size()<12)voices.push_back({cue,0});pending.clear();}
   if(!running)break;
   float gain=muted?0.f:volume.load();
   for(int i=0;i<chunk;++i){float mixed=0;for(auto& voice:voices){const auto& sample=samples[voice.cue];if(voice.position<sample.size())mixed+=sample[voice.position++];}pcm[i]=int16_t(std::clamp(mixed*gain,-.9f,.9f)*32767);}
   voices.erase(std::remove_if(voices.begin(),voices.end(),[&](const Voice& v){return v.position>=samples[v.cue].size();}),voices.end());
   if(!device.write(pcm.data(),chunk))break;
  }ready=false;running=false;
 }
};
Audio::Audio():impl_(new Impl){} Audio::~Audio(){shutdown();}
bool Audio::start(){shutdown();for(int i=0;i<count;++i)impl_->samples[i]=makeCue(i);impl_->running=true;try{impl_->worker=std::thread([this]{impl_->run();});}catch(...){impl_->running=false;return false;}return true;}
void Audio::shutdown(){impl_->running=false;impl_->changed.notify_all();if(impl_->worker.joinable())impl_->worker.join();impl_->ready=false;std::lock_guard<std::mutex> lock(impl_->mutex);impl_->pending.clear();impl_->last.fill({});}
void Audio::setVolume(float value){impl_->volume=std::isfinite(value)?std::clamp(value,0.f,1.f):.65f;}
void Audio::setMuted(bool value){impl_->muted=value;}
bool Audio::available()const{return impl_->ready;}
void Audio::play(SoundCue cue){int index=int(cue);if(index<0||index>=count||!impl_->ready||impl_->muted||impl_->volume<=0)return;auto now=Clock::now();std::lock_guard<std::mutex> lock(impl_->mutex);int interval=(cue==SoundCue::Launch||cue==SoundCue::Impact)?150:55;if(now-impl_->last[index]<std::chrono::milliseconds(interval)||impl_->pending.size()>=16)return;impl_->last[index]=now;impl_->pending.push_back(index);impl_->changed.notify_one();}
}
