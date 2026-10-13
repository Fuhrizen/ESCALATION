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
void check(bool v,const char* reason){++checks;if(!v)throw std::runtime_error(reason);}
void near(double a,double b,const char* reason){check(std::abs(a-b)<1e-7,reason);}
int producer(const Simulation& s,Facility type){
  for(size_t i=0;i<s.state.sites.size();++i)if(s.state.sites[i].type==type&&s.state.regions[s.state.sites[i].region].owner==s.state.player)return int(i);
  return -1;
}
void completeField(Simulation& s,int field,int tier){
  for(int n=0;n<=tier;++n)s.state.completed[field*14+n]=true;
  s.state.levels[field]=tier+1;
}
void writeSave(const std::string& path,const Json& value){std::ofstream file(path);file<<value.serialize();}
int main(){try{
  World w;w.load("data/world.geojson","data/histories.json");
  check(w.countries.size()==177,"full world loaded");
  check(std::any_of(w.cities.begin(),w.cities.end(),[](const WorldCity& city){return city.name=="Juba"&&city.countryCode=="SDS";}),"Natural Earth South Sudan country alias imported");
  World antimeridian;antimeridian.roads.push_back({{{179,0},{-179,0}}});antimeridian.rebuildRoadIndex();
  check(antimeridian.nearbyRoads({179.9,0},0.1)==std::vector<int>{0},"east antimeridian road index");
  check(antimeridian.nearbyRoads({-179.9,0},0.1)==std::vector<int>{0},"west antimeridian road index");
  check(antimeridian.nearbyRoads({0,0},0.1).empty(),"antimeridian road does not span Greenwich");
  int fra=w.find("FRA"),deu=w.find("DEU");
  check(fra>=0&&deu>=0,"nation IDs");
  check(w.pick({2.35,48.85})==fra,"Paris picking");
  check(w.pick({-30,0})==-1,"ocean picking");
  check(w.countries[fra].contains({2.35,48.85}),"point containment");
  check(std::find(w.countries[fra].neighbours.begin(),w.countries[fra].neighbours.end(),deu)!=w.countries[fra].neighbours.end(),"real adjacent country");
  for(auto& country:w.countries){if(!country.contains(country.center))throw std::runtime_error("Invalid land seed: "+country.code);check(true,"country seed on land");}
  auto shapes=w.subdivide(fra);check(shapes.size()==24,"region partition");
  double originalArea=0,partitionArea=0;
  for(auto t:w.countries[fra].triangles)originalArea+=std::abs((t[1].x-t[0].x)*(t[2].y-t[0].y)-(t[2].x-t[0].x)*(t[1].y-t[0].y))/2;
  for(auto& shape:shapes)for(auto& patch:shape.patches){double area=0;for(size_t i=0;i<patch.size();++i)area+=patch[i].x*patch[(i+1)%patch.size()].y-patch[(i+1)%patch.size()].x*patch[i].y;partitionArea+=std::abs(area)/2;}
  near(originalArea,partitionArea,"partition conserves country area");
  for(auto& r:shapes){check(w.countries[fra].contains(r.center),"region site on land");for(auto& patch:r.patches)for(auto p:patch)check(std::isfinite(p.x)&&std::isfinite(p.y),"finite clipped land");}
  check(w.canonical(w.find("LUX"))==w.find("BEL"),"small-state gameplay grouping");
  check(w.pick(w.countries[w.find("LUX")].center)==w.find("BEL"),"grouped land picking");
  auto flags=readJson("data/flags.json");for(auto& c:w.countries)check(flags.get<Object>().at("nations").get<Object>().count(c.code)==1,"flag coverage");

  Simulation s(w);s.start(fra,Field::Army,false);
  check(s.state.regions.size()==48&&s.techs.size()==112,"campaign and tech catalogue initialized");
  check(s.state.simpleEconomy&&s.state.money==0,"regional output replaces currency economy");
  check(s.state.units.size()==8,"both nations have starting forces");
  check(s.state.outputRate>0,"initial output estimate available without advancing time");
  check(!s.queueResearch(1),"research prerequisites enforced");
  check(s.queueResearch(0)&&!s.queueResearch(0),"research queues once without payment");
  check(!s.build(s.state.capital,Facility::Farm),"retired economy buildings unavailable");
  check(!s.connect(0,1)&&!s.upgradeRoad(0)&&!s.recruit()&&!s.attack(24),"retired route and offensive commands unavailable");
  for(size_t r=0;r<s.state.regions.size();++r){check(std::any_of(s.state.settlements.begin(),s.state.settlements.end(),[&](const Settlement& city){return city.region==int(r);}),"every region has an objective");}
  check(std::any_of(s.state.settlements.begin(),s.state.settlements.end(),[](const Settlement& city){return city.name=="Paris"&&city.population>100000;}),"real-world major city included");
  int realCities=0,campaignCities=0;
  for(const auto& city:w.cities){int n=w.canonical(w.find(city.countryCode));if(n==s.state.player||n==s.state.enemy)++realCities;}
  for(const auto& city:s.state.settlements)if(city.population>0)++campaignCities;
  check(campaignCities==realCities,"canonical nation assignment preserves coastal and border cities");
  for(size_t n=0;n<w.countries.size();++n)if(w.canonical(int(n))==int(n)&&w.countries[n].code!="ATA"){
    Simulation campaign(w);campaign.start(int(n));
    for(const auto& city:campaign.state.settlements)check(campaign.regionAt(city.position)==city.region,"all campaign objectives are on assigned region land");
  }
  Simulation grouped(w);grouped.start(w.find("LUX"));
  check(grouped.state.player==w.find("BEL"),"grouped campaign uses canonical country");
  for(const auto& city:w.cities)if(city.countryCode=="LUX")check(std::any_of(grouped.state.settlements.begin(),grouped.state.settlements.end(),[&](const Settlement& c){return c.name==city.name&&grouped.state.regions[c.region].nation==grouped.state.player;}),"grouped nation's imported cities retained");

  int base=producer(s,Facility::ArmyBase),home=s.state.capital;
  check(base>=0,"starting army producer");
  check(!s.queueUnit(UnitType::Armour,base),"advanced unit research gate");
  check(!s.buildAt(home,Facility::Factory,{-30,0}),"construction rejects ocean");
  check(!s.buildAt(24,Facility::Factory,s.state.regions[24].center),"construction rejects rival land");
  check(s.buildAt(home,Facility::Factory,s.state.regions[home].center),"controlled map construction queued");
  size_t construction=s.state.sites.size()-1;
  check(s.queueUnit(UnitType::Infantry,base),"unit production queued without currency");
  size_t recruit=s.state.units.size()-1;
  double output=s.state.outputRate,buildBefore=s.state.sites[construction].remaining,unitBefore=s.state.units[recruit].remaining;
  s.tick();
  near(s.state.productionRate,output*0.5,"production receives allocated share");
  near(buildBefore-s.state.sites[construction].remaining,output*0.25,"construction shares finite production");
  near(unitBefore-s.state.units[recruit].remaining,output*0.25,"unit shares finite production");
  near(s.state.research.front().progress,output*0.5*1.08*1.35,"research allocation applies laboratories and specialization");
  check(!s.orderUnit(recruit,home),"unfinished unit cannot move");
  s.state.researchBudget=0;s.refreshRates();double researchBefore=s.state.research.front().progress;
  s.state.sites[construction].remaining=1;s.state.units[recruit].remaining=100;
  s.tick();
  near(s.state.research.front().progress,researchBefore,"zero research share pauses active research");
  near(s.state.sites[construction].remaining,0,"small construction job completes");
  near(s.state.units[recruit].remaining,100-(s.state.productionRate-1),"unused share flows to unfinished jobs");
  double unitWork=s.state.units[recruit].remaining;s.state.researchBudget=1;s.refreshRates();s.tick();
  near(s.state.units[recruit].remaining,unitWork,"full research allocation pauses production");
  s.state.researchBudget=0.5;
  for(int n=0;n<200&&(!s.state.completed[0]||s.state.units[recruit].remaining>0);++n)s.tick();
  check(s.state.completed[0]&&s.state.levels[0]==1,"research completes and unlocks platform");
  check(s.state.units[recruit].remaining==0,"unit completes from shared output");
  check(s.available(1)&&s.available(2),"sibling research paths available independently");
  check(s.researchCost(2)>s.techs[2].cost,"future tech carries cost premium");
  check(s.queueResearch(2),"future tech available without year blocking");
  double premium=s.state.research.front().cost;s.state.day+=365*25;s.tick();
  near(s.state.research.front().cost,premium,"research price fixed at queue time");
  check(s.queueUnit(UnitType::Armour,base),"research unlocks armoured production");
  size_t canceledUnit=s.state.units.size()-1;
  check(s.cancelUnit(canceledUnit)&&!s.cancelUnit(canceledUnit),"production cancellation cannot repeat");
  check(s.build(home,Facility::Laboratory),"laboratory queued");size_t canceledSite=s.state.sites.size()-1;
  check(s.cancelSite(canceledSite)&&!s.cancelSite(canceledSite),"construction cancellation cannot repeat");
  near(s.state.money,0,"new work never creates currency charges or refunds");
  double beforeRefresh=s.state.research.front().progress;s.refreshRates();s.refreshRates();
  near(s.state.research.front().progress,beforeRefresh,"rate refresh does not advance work");

  // Production source loss and disabled laboratories have immediate effects.
  Simulation paused(w);paused.start(fra);int lab=producer(paused,Facility::Laboratory),army=producer(paused,Facility::ArmyBase);
  check(paused.queueResearch(0)&&paused.queueUnit(UnitType::Infantry,army),"paused systems initialized");
  paused.state.sites[lab].enabled=false;double work=paused.state.units.back().remaining;paused.tick();
  near(paused.state.research.front().progress,0,"research requires an operating laboratory");
  near(work-paused.state.units.back().remaining,paused.state.outputRate,"idle research share redirects to production");
  paused.state.regions[paused.state.sites[army].region].owner=paused.state.enemy;work=paused.state.units.back().remaining;paused.tick();
  near(paused.state.units.back().remaining,work,"captured producer cannot complete player unit");

  auto saveDirectory=std::filesystem::current_path()/"build"/"test-tmp";
  std::filesystem::create_directories(saveDirectory);
  auto save=(saveDirectory/"escalation-test-save.json").string();
  s.save(save);Simulation loaded(w);loaded.load(save);
  check(loaded.state.day==s.state.day&&loaded.state.completed==s.state.completed,"campaign and research save roundtrip");
  check(loaded.state.settlements.size()==s.state.settlements.size(),"settlement save roundtrip");
  check(loaded.state.sites[canceledSite].destroyed,"canceled site save roundtrip");
  near(loaded.state.research.front().cost,premium,"queued premium save roundtrip");
  s.refreshRates();near(loaded.state.researchRate,s.state.researchRate,"restored rates describe restored jobs");
  for(int n=0;n<50;++n){s.tick();loaded.tick();}
  near(loaded.state.research.front().progress,s.state.research.front().progress,"restored research continues deterministically");
  for(size_t i=0;i<s.state.units.size();++i)near(loaded.state.units[i].remaining,s.state.units[i].remaining,"restored production continues deterministically");
  Simulation flying(w);flying.start(fra);completeField(flying,1,0);
  check(flying.queueUnit(UnitType::Fighter,producer(flying,Facility::Airbase)),"aircraft production unlock");
  for(int n=0;n<20&&flying.state.units.back().remaining>0;++n)flying.tick();
  size_t aircraft=flying.state.units.size()-1;
  check(flying.orderUnitAt(aircraft,{-10,45}),"aircraft ocean destination order");flying.tick();flying.save(save);
  Simulation flightRestored(w);flightRestored.load(save);
  check(flightRestored.state.units[aircraft].moving&&flightRestored.state.units[aircraft].waypoints.size()==flying.state.units[aircraft].waypoints.size(),"geometric route and moving order saved");
  for(int n=0;n<12;++n){flying.tick();flightRestored.tick();near(distance(flying.state.units[aircraft].position,flightRestored.state.units[aircraft].position),0,"restored route follows identical movement");}
#ifndef ESCALATION_SKIP_EXCEPTION_TESTS
  int previous=loaded.state.day;
  auto rejects=[&](const Json& bad,const char* reason){writeSave(save,bad);bool rejected=false;try{loaded.load(save);}catch(const std::exception&){rejected=true;}check(rejected,reason);check(loaded.state.day==previous,"failed load remains transactional");};
  rejects(Json(Object{{"version",num(2)}}),"corrupt save rejected");
  s.save(save);auto valid=readJson(save),bad=valid;bad.get<Object>()["stock"]=str("broken");rejects(bad,"wrong type rejected");
  bad=valid;bad.get<Object>()["settlements"].get<Array>()[0].get<Object>()["region"]=num(999);rejects(bad,"invalid settlement index rejected");
  bad=valid;bad.get<Object>()["settlements"]=Json(Array{});rejects(bad,"missing regional objectives rejected");
  bad=valid;bad.get<Object>()["queue"].get<Array>()[0].get<Object>()["cost"]=num(1);rejects(bad,"invalid research premium rejected");
  bad=valid;bad.get<Object>()["units"].get<Array>()[0].get<Object>()["waypoints"]=Json(Array{Json(Array{num(0),num(100)})});rejects(bad,"invalid movement geometry rejected");
  bad=valid;bad.get<Object>()["warheadsQueued"]=num(1);bad.get<Object>()["warheadWork"]=num(0);rejects(bad,"invalid nuclear production state rejected");
#endif
  // Legacy v2 snapshots migrate time-based queues and preserve established control.
  Simulation legacy(w);legacy.start(fra);legacy.build(legacy.state.capital,Facility::Factory);legacy.state.sites.back().remaining=17.5;
  legacy.queueUnit(UnitType::Infantry,producer(legacy,Facility::ArmyBase));legacy.state.units.back().remaining=6;legacy.state.units.back().total=12;
  legacy.state.regions[24].owner=fra;legacy.save(save);auto old=readJson(save);old.get<Object>()["version"]=num(2);old.get<Object>().erase("settlements");old.get<Object>().erase("warheadWork");old.get<Object>().erase("warheadsQueued");writeSave(save,old);
  Simulation migrated(w);migrated.load(save);
  near(migrated.state.sites.back().remaining,migrated.buildCost(Facility::Factory)*0.5,"legacy building completion fraction migrated");
  near(migrated.state.units.back().remaining,migrated.unitCost(UnitType::Infantry)*0.5,"legacy unit completion fraction migrated");
  for(const auto& city:migrated.state.settlements)if(city.region==24)check(city.owner==fra,"legacy captured region objectives inherit control");
  check(migrated.state.money==0&&migrated.state.simpleEconomy,"legacy monetary economy retired");
  Simulation history(w);history.start(fra);history.state.sites.resize(1000);
  check(!history.build(history.state.capital,Facility::Factory),"construction history matches save-safe limit");

  // Strategic production is expensive shared work, followed by the existing scripted second strike.
  Simulation nuclear(w);nuclear.start(fra);check(!nuclear.produceWarhead(),"nuclear technology gate");
  completeField(nuclear,6,3);completeField(nuclear,4,4);
  check(nuclear.produceWarhead()&&nuclear.state.stock[6]==0,"warhead is queued rather than granted instantly");
  double nuclearWork=nuclear.state.warheadWork;nuclear.tick();
  near(nuclearWork-nuclear.state.warheadWork,nuclear.state.productionRate,"warhead consumes national production");
  nuclear.save(save);Simulation arsenal(w);arsenal.load(save);
  check(arsenal.state.warheadsQueued==1,"strategic production queue saved");
  near(arsenal.state.warheadWork,nuclear.state.warheadWork,"strategic work saved without advancement");
  for(int n=0;n<200&&arsenal.state.stock[6]<1;++n)arsenal.tick();
  check(arsenal.state.stock[6]==1&&arsenal.state.warheadsQueued==0,"warhead completes with finite work");
  check(!arsenal.launchNuclear(24),"nuclear release requires war");arsenal.declareWar();
  check(!arsenal.launchNuclear(24),"nuclear release requires operational missile site");
  check(arsenal.build(arsenal.state.capital,Facility::Silo),"missile site unlocked");
  for(int n=0;n<100&&arsenal.state.sites.back().remaining>0;++n)arsenal.tick();
  check(arsenal.launchNuclear(24),"nuclear delivery permitted once ready");
  check(arsenal.state.defeat&&arsenal.state.nuclearUsed&&!arsenal.state.war&&arsenal.state.stability<50,"scripted second strike ends campaign");
  check(!arsenal.launchNuclear(24)&&!arsenal.produceWarhead(),"ended campaign rejects nuclear orders");
  std::filesystem::remove(save);std::filesystem::remove(save+".bak");
  std::cout<<"Passed "<<checks<<" checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<"\n";return 1;}}
