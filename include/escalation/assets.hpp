#pragma once
#include "world.hpp"
#include "gl.hpp"
#include <vector>
namespace escalation {
struct FlagUV {Point min,max;};
class SurfaceAssets {
 std::vector<int> indices_;
public:
 gl::UInt terrain=0,flags=0;
 int terrainWidth=0,terrainHeight=0;
 void load(const std::string& directory,const World& world);
 FlagUV flag(int country) const;
 void shutdown();
};
}
