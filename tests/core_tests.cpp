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
    s.start(fra,Field::Economy,false);
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
    sea.start(w.find("JPN"),Field::Economy,false);
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
    mad.start(fra,Field::Economy,false);
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

    // The default campaign pays only currency and supports direct map placement.
    Simulation modern(w);
    modern.start(fra);
    check(modern.state.simpleEconomy,"default currency economy");
    check(modern.state.units.size()==8,"visible starting forces for both nations");
    check(w.canonical(w.find("LUX"))==w.find("BEL"),"small-state gameplay grouping");
    check(w.pick(w.countries[w.find("LUX")].center)==w.find("BEL"),"grouped land map picking");
    auto flags=readJson("data/flags.json");
    for(auto& c:w.countries)check(flags.get<Object>().at("nations").get<Object>().count(c.code)==1,"flag coverage for every map feature");
    int home=modern.state.capital;
    modern.state.stock.fill(0);
    double funds=modern.state.money;
    check(!modern.buildAt(home,Facility::Farm,{-30,0}),"construction rejects ocean placement");
    check(modern.state.money==funds,"invalid placement does not charge treasury");
    check(!modern.buildAt(24,Facility::Farm,modern.state.regions[24].center),"construction rejects rival land");
    auto point=modern.state.regions[home].center;
    check(modern.buildAt(home,Facility::Farm,point),"currency-only map construction");
    check(modern.state.money==funds-modern.buildCost(Facility::Farm),"construction exact currency charge");
    auto farm=modern.state.sites.size()-1;
    check(modern.state.sites[farm].position.x==point.x,"construction retains clicked position");
    for(int n=0;n<36;++n)modern.tick();
    check(modern.state.sites[farm].remaining==0,"construction completes without hidden stock requirements");
    double income=modern.regionIncome(home);
    check(income==8,"completed farm generates declared income");
    check(modern.upgradeRoad(home),"regional infrastructure currency upgrade");
    check(std::abs(modern.regionIncome(home)-income*1.15)<1e-8,"roads increase regional income");
    check(!modern.upgradeRoad(24),"rival roads cannot be upgraded");
    check(modern.build(home,Facility::Farm),"second construction queued");
    auto cancelled=modern.state.sites.size()-1;
    funds=modern.state.money;
    check(modern.cancelSite(cancelled),"construction can be cancelled");
    check(modern.state.money==funds+modern.buildCost(Facility::Farm)*0.5,"construction partial refund");
    check(!modern.cancelSite(cancelled),"cancelled construction cannot refund twice");
    int base=-1;
    for(size_t i=0;i<modern.state.sites.size();++i)if(modern.state.sites[i].type==Facility::ArmyBase&&modern.state.regions[modern.state.sites[i].region].owner==fra)base=int(i);
    check(base>=0,"starting army production site");
    check(!modern.queueUnit(UnitType::Armour,base),"advanced unit research gate");
    check(modern.queueUnit(UnitType::Infantry,base),"unit production paid from currency");
    auto recruit=modern.state.units.size()-1;
    check(!modern.orderUnit(recruit,home),"unfinished unit cannot move");
    for(int n=0;n<12;++n)modern.tick();
    check(modern.state.units[recruit].remaining==0,"unit production completes");
    check(!modern.orderUnit(recruit,24),"no invasion during peace");
    check(!modern.orderUnitAt(recruit,{-30,0}),"ground forces cannot move into ocean");
    check(modern.orderUnit(recruit,home),"ground movement follows friendly region route");
    for(int n=0;n<200&&modern.state.units[recruit].moving;++n)modern.tick();
    check(modern.state.units[recruit].region==home&&!modern.state.units[recruit].moving,"ground unit reaches selected region");
    check(modern.queueResearch(0),"currency-funded research queued");
    for(int n=0;n<80&&!modern.state.completed[0];++n)modern.tick();
    check(modern.state.completed[0]&&modern.state.levels[0]==1,"research completes and applies unit unlock");
    check(modern.queueUnit(UnitType::Armour,base),"research unlocks armoured production");
    check(modern.cancelUnit(modern.state.units.size()-1),"unit production cancellable");
    check(!modern.cancelUnit(modern.state.units.size()-1),"unit cancellation cannot refund twice");
    modern.state.levels[1]=1;
    int airbase=-1;for(size_t i=0;i<modern.state.sites.size();++i)if(modern.state.sites[i].type==Facility::Airbase&&modern.state.regions[modern.state.sites[i].region].owner==fra)airbase=int(i);
    check(modern.queueUnit(UnitType::Fighter,airbase),"air unit production site and unlock");
    size_t fighter=modern.state.units.size()-1;
    for(int n=0;n<24;++n)modern.tick();
    check(modern.orderUnitAt(fighter,{-10,45}),"air unit can move over water");
    modern.save(save);
    Simulation restored(w);restored.load(save);
    check(restored.state.simpleEconomy&&restored.state.regions[home].roads==1,"economy and infrastructure saved");
    check(restored.state.sites[cancelled].destroyed,"cancelled construction saved");
    check(restored.state.units.size()==modern.state.units.size()&&restored.state.units[fighter].moving,"unit production and orders saved");
    for(int n=0;n<50;++n){modern.tick();restored.tick();}
    check(std::abs(modern.state.money-restored.state.money)<1e-8,"currency save deterministic continuation");
    check(distance(modern.state.units[fighter].position,restored.state.units[fighter].position)<1e-8,"unit movement save deterministic continuation");
    check(modern.declareWar(),"new campaign war declaration");
    int frontier=-1;for(int r=24;r<48;++r)for(int n:modern.state.regions[r].neighbours)if(n<24)frontier=r;
    check(frontier>=0,"ground frontier exists");
    check(modern.attack(frontier),"frontier offensive dispatches visible units");
    for(int n=0;n<500&&modern.state.regions[frontier].owner!=fra;++n)modern.tick();
    check(modern.state.regions[frontier].owner==fra,"arriving formations capture a region");
    std::filesystem::remove(save);std::filesystem::remove(save+".bak");
    std::cout<<"Passed "<<checks<<" checks\n";
    return 0;
  }
  catch(const std::exception& e){
    std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<"\n";
    return 1;
  }
}
