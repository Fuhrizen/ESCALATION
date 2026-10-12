#include "escalation/simulation.hpp"
#include <chrono>
#include <iostream>
#ifndef ESCALATION_DATA_DIR
#define ESCALATION_DATA_DIR "data"
#endif
int main(int argc,char** argv){
  try{
    escalation::World world;
    world.load(std::string(ESCALATION_DATA_DIR)+"/world.geojson",std::string(ESCALATION_DATA_DIR)+"/histories.json");
    escalation::Simulation sim(world);
    sim.start(world.find(argc>1?argv[1]:"FRA"));
    int days=argc>2?std::stoi(argv[2]):365*135;
    auto begin=std::chrono::steady_clock::now();
    for(int i=0;i<days;++i){
      for(size_t t=0;t<sim.techs.size();++t)if(sim.available(int(t)))sim.queueResearch(int(t));
      sim.tick();
    }
    double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    std::cout<<"Countries: "<<world.countries.size()<<"; simulated regions: "<<sim.state.regions.size()<<"; technologies: "<<sim.techs.size()<<"\nDays: "<<sim.state.day<<"; year: "<<sim.year()<<"; treasury: "<<sim.state.money<<"; ticks/sec: "<<days/seconds<<"\n";
    return 0;
  }
  catch(const std::exception& e){
    std::cerr<<e.what()<<"\n";
    return 1;
  }
}
