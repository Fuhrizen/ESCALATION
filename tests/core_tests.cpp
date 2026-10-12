#include "escalation/simulation.hpp"
#include "escalation/json.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace escalation;
static int checks=0;
void check(bool v,const char* reason){
  ++checks;
  if(!v)throw std::runtime_error(reason);
}
int main(){
  try{
    World w;
    w.load("data/world.geojson","data/histories.json");
    check(w.countries.size()==177,"full world loaded");
    int fra=w.find("FRA"),deu=w.find("DEU");
    check(fra>=0&&deu>=0,"nation IDs");
    check(w.pick({2.35,48.85})==fra,"Paris picking");
    check(w.pick({-30,0})==-1,"ocean picking");
    check(w.countries[fra].contains({2.35,48.85}),"point containment");
    check(std::find(w.countries[fra].neighbours.begin(),w.countries[fra].neighbours.end(),deu)!=w.countries[fra].neighbours.end(),"real adjacent country");
    for(auto& country:w.countries)if(!country.contains(country.center))throw std::runtime_error("Invalid land seed: "+country.code);
    else check(true,"all country seed sites are on land");
    auto shapes=w.subdivide(fra);
    check(shapes.size()==24,"region partition");
    double originalArea=0,partitionArea=0;
    for(auto t:w.countries[fra].triangles)originalArea+=std::abs((t[1].x-t[0].x)*(t[2].y-t[0].y)-(t[2].x-t[0].x)*(t[1].y-t[0].y))/2;
    for(auto& shape:shapes)for(auto& patch:shape.patches){
      double area=0;
      for(size_t i=0;i<patch.size();++i)area+=patch[i].x*patch[(i+1)%patch.size()].y-patch[(i+1)%patch.size()].x*patch[i].y;
      partitionArea+=std::abs(area)/2;
    }
    check(std::abs(originalArea-partitionArea)<1e-5,"partition conserves country land area");
    for(auto& r:shapes){
      check(w.countries[fra].contains(r.center),"site on land");
      for(auto& patch:r.patches)for(auto p:patch)check(std::isfinite(p.x)&&std::isfinite(p.y),"finite clipped land");
    }
    Simulation s(w);
    s.start(fra);
    check(s.state.regions.size()==48,"one neighbour active");
    check(s.techs.size()==112,"expansive technology tree");
    check(s.connection(s.state.capital)>0,"capital connected");
    check(!s.queueResearch(1),"era and prerequisite gate");
    check(s.queueResearch(0),"starting research available");
    check(!s.queueResearch(0),"no duplicate research");
    for(int i=0;i<200;++i)s.tick();
    check(s.state.completed[0],"research completes");
    check(s.state.stock[0]>=0&&s.state.stock[2]>=0,"bounded inventories");
    check(!s.produceWarhead(),"nuclear gated");
    check(!s.launchNuclear(24),"no free nuclear launch");
    int remote=-1;
    for(int i=0;i<24;++i)if(s.connection(i)==0){
      remote=i;
      break;
    }
    check(remote>=0,"unconnected land available");
    s.state.money=10000;
    s.state.stock[2]=500;
    check(s.build(remote,Facility::Factory),"site construction");
    for(int i=0;i<40;++i)s.tick();
    check(s.state.sites.back().remaining==35,"construction requires transport");
    check(s.connect(s.state.capital,remote),"connect route");
    check(!s.connect(s.state.capital,remote),"reject duplicate route");
    for(int i=0;i<40;++i)s.tick();
    check(s.state.sites.back().remaining==0,"construction completes");
    check(s.state.sites.back().efficiency>0,"production connected");
    s.state.stock[0]=0;
    for(auto& site:s.state.sites)if(site.type==Facility::Mine)site.enabled=false;
    s.tick();
    for(auto& site:s.state.sites)if(site.type==Facility::Steelworks&&s.state.regions[site.region].owner==fra)check(site.efficiency==0,"input starvation reduces output");
    auto save=(std::filesystem::temp_directory_path()/"escalation-test-save.json").string();
    s.save(save);
    Simulation loaded(w);
    loaded.load(save);
    check(loaded.state.day==s.state.day,"save day roundtrip");
    check(loaded.state.money==s.state.money,"save treasury roundtrip");
    check(loaded.state.sites.size()==s.state.sites.size(),"save facilities roundtrip");
    check(loaded.state.completed==s.state.completed,"save research roundtrip");
    for(int i=0;i<50;++i){
      s.tick();
      loaded.tick();
    }
    check(std::abs(loaded.state.money-s.state.money)<1e-8,"deterministic restored simulation");
#ifndef ESCALATION_SKIP_EXCEPTION_TESTS
    {
      std::ofstream bad(save);
      bad<<"{\"version\":2}";
    }
    int previous=loaded.state.day;
    bool rejected=false;
    try{
      loaded.load(save);
    }
    catch(const std::exception&){
      rejected=true;
    }
    check(rejected,"corrupt save must reject");
    check(loaded.state.day==previous,"failed load transactional");
    s.save(save);
    auto malformed=readJson(save);
    malformed.get<Object>()["stock"]=str("broken");
    {
      std::ofstream bad(save);
      bad<<malformed.serialize();
    }
    rejected=false;
    try{
      loaded.load(save);
    }
    catch(const std::exception&){
      rejected=true;
    }
    check(rejected,"wrong type save must reject");
    check(loaded.state.day==previous,"wrong type load transactional");
#endif
    std::filesystem::remove(save);
    std::filesystem::remove(save+".bak");
    s.state.stock[3]=2000;
    s.state.stock[4]=2000;
    s.state.brigades=30;
    check(s.declareWar(),"war declaration");
    int target=-1;
    for(int i=24;i<48;++i)for(int n:s.state.regions[i].neighbours)if(n<24){
      s.state.money=10000;
      s.state.stock[2]=100;
      s.connect(s.state.capital,n);
      target=i;
    }
    check(target>=0&&s.attack(target),"frontier attack");
    for(int i=0;i<250&&s.state.regions[target].owner!=fra;++i)s.tick();
    check(s.state.regions[target].owner==fra,"region captured by supplied offensive");
    Simulation sea(w);
    sea.start(w.find("JPN"));
    sea.state.money=10000;
    sea.state.stock[2]=500;
    sea.declareWar();
    check(!sea.attack(24),"unresearched amphibious operation blocked");
    sea.state.levels[2]=1;
    check(sea.build(sea.state.capital,Facility::Shipyard),"shipyard construction unlocked");
    sea.state.sites.back().remaining=0;
    sea.state.stock[3]=1000;
    sea.state.stock[4]=1000;
    sea.state.brigades=30;
    check(sea.attack(24),"supplied shipyard permits landing");
    for(int i=0;i<250&&sea.state.regions[24].owner!=sea.state.player;++i)sea.tick();
    check(sea.state.regions[24].owner==sea.state.player,"amphibious capture advances");
    Simulation mad(w);
    mad.start(fra);
    mad.state.day=365*50;
    mad.state.levels[4]=5;
    mad.state.levels[6]=4;
    mad.state.money=100000;
    mad.state.stock[2]=1000;
    mad.state.stock[3]=1000;
    check(mad.produceWarhead(),"nuclear industrial gate");
    check(mad.build(mad.state.capital,Facility::Silo),"silo build gate");
    mad.state.sites.back().remaining=0;
    mad.declareWar();
    check(mad.launchNuclear(24),"nuclear delivery");
    check(mad.state.defeat&&mad.state.nuclearUsed&&mad.state.stability<50,"MAD campaign cost");
    std::cout<<"Passed "<<checks<<" checks\n";
    return 0;
  }
  catch(const std::exception& e){
    std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<"\n";
    return 1;
  }
}
