#include "escalation/simulation.hpp"
#include <chrono>
#include <iostream>
using namespace escalation;
// Run from the repository root; this is an opt-in diagnostic, not a timing assertion.
int main(){
  World world;world.load("data/world.geojson","data/histories.json");
  for(bool opposing:{false,true})for(int count:{100,500,1000}){
    Simulation sim(world);sim.start(world.find("FRA"));
    sim.state.units.clear();sim.state.war=true;
    std::vector<int> invasionRegions;
    if(opposing){
      for(size_t r=0;r<sim.state.regions.size();++r)if(sim.state.regions[r].nation==sim.state.enemy)invasionRegions.push_back(int(r));
      if(invasionRegions.empty())return 2;
    }
    for(int i=0;i<count;++i){
      Unit unit;unit.owner=opposing&&i%2?sim.state.enemy:sim.state.player;unit.region=i%sim.state.regions.size();
      if(opposing)unit.region=invasionRegions[(i/2)%invasionRegions.size()];
      unit.position=unit.destination=sim.state.regions[unit.region].center;
      unit.type=UnitType::Infantry;
      if(opposing){unit.position.x+=(i%2?0.15:-0.15);unit.destination=unit.position;}
      sim.state.units.push_back(unit);
    }
    if(opposing)for(size_t i=0;i<sim.state.units.size();i+=2){Point target=sim.state.units[i].position;target.x+=.1;sim.orderUnitAt(i,target);}
    sim.tick();
    auto begin=std::chrono::steady_clock::now();
    const int ticks=opposing?20:100;
    for(int i=0;i<ticks;++i)sim.tick();
    double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    int moving=0,alive=0;for(const auto& u:sim.state.units){moving+=u.moving;alive+=u.health>0;}
    std::cout<<(opposing?"opposing moving incursion: ":"friendly stationary: ")<<count<<" units: "<<elapsed/ticks<<" ms/tick; "<<alive<<" alive, "<<moving<<" moving, "<<sim.state.engagements.size()<<" engagements\n";
  }
}
