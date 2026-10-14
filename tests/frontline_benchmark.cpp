#include "escalation/simulation.hpp"
#include <chrono>
#include <iostream>
using namespace escalation;
// Run from the repository root; this is an opt-in diagnostic, not a timing assertion.
int main(){
  World world;world.load("data/world.geojson","data/histories.json");
  for(int count:{100,500,1000}){
    Simulation sim(world);sim.start(world.find("FRA"));
    sim.state.units.clear();sim.state.war=true;
    for(int i=0;i<count;++i){
      Unit unit;unit.owner=sim.state.player;unit.region=i%sim.state.regions.size();
      unit.position=unit.destination=sim.state.regions[unit.region].center;
      unit.type=UnitType::Infantry;sim.state.units.push_back(unit);
    }
    sim.tick();
    auto begin=std::chrono::steady_clock::now();
    for(int i=0;i<100;++i)sim.tick();
    double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    std::cout<<count<<" units: "<<elapsed/100<<" ms/tick\n";
  }
}
