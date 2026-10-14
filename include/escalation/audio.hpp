#pragma once
#include <memory>
namespace escalation {
enum class SoundCue { Select, Order, Success, Invalid, Complete, Launch, Impact, Click, Count };
// UI/game events only: no device is a normal, silent condition (e.g. headless runs).
class Audio {
public:
 Audio(); ~Audio();
 Audio(const Audio&)=delete; Audio& operator=(const Audio&)=delete;
 bool start();
 void shutdown();
 void setVolume(float volume);
 void setMuted(bool muted);
 void play(SoundCue cue);
 bool available() const;
private:
 struct Impl; std::unique_ptr<Impl> impl_;
};
}
