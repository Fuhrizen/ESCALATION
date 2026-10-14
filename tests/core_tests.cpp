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
  check(!s.buildAt(home,Facility::Factory,s.state.regions[home].center),"placement requires ready inventory");
  check(!s.placementReason(Facility::Factory,home,s.state.regions[home].center).empty(),"placement preview requires inventory");
  check(s.queueBuilding(Facility::Factory),"global manufacture queues without a location");
  size_t construction=0;
  check(s.queueUnit(UnitType::Infantry,base),"unit production queued without currency");
  size_t recruit=s.state.units.size()-1;
  double output=s.state.outputRate,buildBefore=s.state.buildingQueue[construction].progress,unitBefore=s.state.units[recruit].remaining;
  s.tick();
  near(s.state.productionRate,output*0.5,"production receives allocated share");
  near(s.state.buildingQueue[construction].progress-buildBefore,output*0.25*simulationTickSeconds*0.1,"construction shares finite production");
  near(unitBefore-s.state.units[recruit].remaining,output*0.25*simulationTickSeconds*0.1,"unit shares finite production");
  near(s.state.research.front().progress,output*0.5*1.08*1.35*simulationTickSeconds*0.4,"research allocation applies laboratories and specialization");
  check(!s.orderUnit(recruit,home),"unfinished unit cannot move");
  s.state.researchBudget=0;s.refreshRates();double researchBefore=s.state.research.front().progress;
  s.state.buildingQueue[construction].progress=s.state.buildingQueue[construction].total-0.01;s.state.units[recruit].remaining=100;
  s.tick();
  near(s.state.research.front().progress,researchBefore,"zero research share pauses active research");
  check(s.state.buildingQueue.empty()&&s.state.buildingInventory[int(Facility::Factory)]==1,"completed manufacture enters ready inventory");
  check(!s.placeBuilding(Facility::Factory,24,s.state.regions[24].center)&&s.state.buildingInventory[int(Facility::Factory)]==1,"failed placement preserves inventory");
  check(s.placeBuilding(Facility::Factory,home,s.state.regions[home].center)&&s.state.buildingInventory[int(Facility::Factory)]==0,"successful placement consumes ready inventory");
  check(s.state.sites.back().remaining==0,"ready building is operational when placed");
  completeField(s,2,0);s.state.buildingInventory[int(Facility::Shipyard)]=1;
  check(!s.placementReason(Facility::Shipyard,home,s.state.regions[home].center).empty(),"shipyard preview rejects inland placement");
  check(!s.placeBuilding(Facility::Shipyard,home,s.state.regions[home].center)&&s.state.buildingInventory[int(Facility::Shipyard)]==1,"inland shipyard placement preserves inventory");
  near(s.state.units[recruit].remaining,100-(output*simulationTickSeconds*0.1-0.01),"unused share flows to unfinished jobs");
  double unitWork=s.state.units[recruit].remaining;s.state.researchBudget=1;s.refreshRates();s.tick();
  near(s.state.units[recruit].remaining,unitWork,"full research allocation pauses production");
  s.state.researchBudget=0.5;
  for(int n=0;n<5000&&(!s.state.completed[0]||s.state.units[recruit].remaining>0);++n)s.tick();
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
  check(s.queueBuilding(Facility::Laboratory),"laboratory manufacture queued");
  check(s.cancelBuilding(0)&&!s.cancelBuilding(0),"manufacture cancellation cannot repeat");
  check(s.queueBuilding(Facility::Factory),"pending manufacture for save test");s.state.buildingQueue.front().progress=12.5;
  s.state.buildingInventory[int(Facility::Laboratory)]=2;
  near(s.state.money,0,"new work never creates currency charges or refunds");
  double beforeRefresh=s.state.research.front().progress;s.refreshRates();s.refreshRates();
  near(s.state.research.front().progress,beforeRefresh,"rate refresh does not advance work");

  // Production source loss and disabled laboratories have immediate effects.
  Simulation paused(w);paused.start(fra);int lab=producer(paused,Facility::Laboratory),army=producer(paused,Facility::ArmyBase);
  check(paused.queueResearch(0)&&paused.queueUnit(UnitType::Infantry,army),"paused systems initialized");
  paused.state.sites[lab].enabled=false;double work=paused.state.units.back().remaining;paused.tick();
  near(paused.state.research.front().progress,0,"research requires an operating laboratory");
  near(work-paused.state.units.back().remaining,paused.state.outputRate*simulationTickSeconds*0.1,"idle research share redirects to production");
  paused.state.regions[paused.state.sites[army].region].owner=paused.state.enemy;work=paused.state.units.back().remaining;paused.tick();
  near(paused.state.units.back().remaining,work,"captured producer cannot complete player unit");

  // ETA uses the same active streams and complete FIFO backlog as production.
  Simulation estimates(w);estimates.start(fra,Field::Army);
  int etaArmy=producer(estimates,Facility::ArmyBase),etaLab=producer(estimates,Facility::Laboratory);
  check(estimates.queueUnit(UnitType::Infantry,etaArmy),"ETA producer initialized");
  estimates.state.sites[etaArmy].enabled=false;estimates.refreshRates();
  near(estimates.buildingETA(Facility::Factory),100/(estimates.state.outputRate*0.1),"paused producer excluded from prospective building ETA");
  near(estimates.researchETA(0),estimates.researchCost(0)/(estimates.state.outputRate*1.08*1.35*0.4),"paused production redirects prospective research share");
  check(!std::isfinite(estimates.unitETA(estimates.state.units.size()-1)),"paused producer has paused unit ETA");
  completeField(estimates,6,3);completeField(estimates,4,4);
  check(estimates.produceWarhead()&&estimates.produceWarhead(),"ETA strategic backlog initialized");
  estimates.state.warheadWork=0.01;estimates.refreshRates();
  near(estimates.buildingETA(Facility::Factory),200/(estimates.state.outputRate*0.1),"all strategic FIFO work blocks prospective building");
  double strategicTick=estimates.state.productionRate*simulationTickSeconds*0.1;estimates.tick();
  check(estimates.state.stock[int(Resource::Warheads)]==1&&estimates.state.warheadsQueued==1,"strategic FIFO completes front once");
  near(estimates.state.warheadWork,1200-(strategicTick-0.01),"strategic FIFO receives same-tick residual capacity");
  estimates.state.sites[etaLab].enabled=false;estimates.refreshRates();
  near(estimates.buildingETA(Facility::Factory),100/(estimates.state.outputRate*0.1),"paused strategic backlog excluded from ETA");
  estimates.state.sites.push_back({estimates.state.capital,Facility::Factory,20,0,true,estimates.state.regions[estimates.state.capital].center,false});
  estimates.refreshRates();near(estimates.buildingETA(Facility::Factory),120/(estimates.state.outputRate*0.1),"unfinished active sites count toward shared ETA");
  check(estimates.cancelSite(estimates.state.sites.size()-1),"unfinished site canceled");
  near(estimates.buildingETA(Facility::Factory),100/(estimates.state.outputRate*0.1),"canceling foundation updates ETA immediately");

  Simulation fifo(w);fifo.start(fra);
  check(fifo.queueBuilding(Facility::Factory)&&fifo.queueBuilding(Facility::Laboratory),"FIFO carryover initialized");
  double tickWork=fifo.state.productionRate*simulationTickSeconds*0.1;
  fifo.state.buildingQueue.front().progress=fifo.state.buildingQueue.front().total-0.01;
  fifo.tick();
  check(fifo.state.buildingInventory[int(Facility::Factory)]==1&&fifo.state.buildingQueue.size()==1,"FIFO front completes exactly once");
  near(fifo.state.buildingQueue.front().progress,tickWork-0.01,"FIFO next item receives residual capacity in same tick");
  check(fifo.state.sites.size()==12,"ready inventory grants no operational sites");
  Simulation sharedFifo(w);sharedFifo.start(fra);
  sharedFifo.queueBuilding(Facility::Factory);sharedFifo.queueBuilding(Facility::Laboratory);sharedFifo.queueUnit(UnitType::Infantry,producer(sharedFifo,Facility::ArmyBase));
  sharedFifo.state.buildingQueue.front().progress=sharedFifo.state.buildingQueue.front().total-0.01;
  double sharedTick=sharedFifo.state.productionRate*simulationTickSeconds*0.1;sharedFifo.tick();
  near(sharedFifo.state.buildingQueue.front().progress,(sharedTick-0.02)/2,"next FIFO job joins active unit at completion boundary");
  near(sharedFifo.state.units.back().remaining,sharedFifo.unitCost(UnitType::Infantry)-sharedTick/2,"unit receives fair share across FIFO boundary");

  auto saveDirectory=std::filesystem::current_path()/"build"/"test-tmp";
  std::filesystem::create_directories(saveDirectory);
  auto save=(saveDirectory/"escalation-test-save.json").string();
  s.save(save);Simulation loaded(w);loaded.load(save);
  check(loaded.state.day==s.state.day&&loaded.state.completed==s.state.completed,"campaign and research save roundtrip");
  check(loaded.state.settlements.size()==s.state.settlements.size(),"settlement save roundtrip");
  near(loaded.state.buildingQueue.front().progress,12.5,"manufacture progress save roundtrip");
  check(loaded.state.buildingInventory[int(Facility::Laboratory)]==2,"ready inventory save roundtrip");
  near(loaded.state.research.front().cost,premium,"queued premium save roundtrip");
  s.refreshRates();near(loaded.state.researchRate,s.state.researchRate,"restored rates describe restored jobs");
  for(int n=0;n<50;++n){s.tick();loaded.tick();}
  near(loaded.state.research.front().progress,s.state.research.front().progress,"restored research continues deterministically");
  for(size_t i=0;i<s.state.units.size();++i)near(loaded.state.units[i].remaining,s.state.units[i].remaining,"restored production continues deterministically");
  Simulation flying(w);flying.start(fra);completeField(flying,1,0);
  check(flying.queueUnit(UnitType::Fighter,producer(flying,Facility::Airbase)),"aircraft production unlock");
  for(int n=0;n<5000&&flying.state.units.back().remaining>0;++n)flying.tick();
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
  bad=valid;bad.get<Object>()["buildingInventory"].get<Array>()[int(Facility::Factory)]=num(1000);rejects(bad,"ready plus queued building cap rejected");
  bad=valid;bad.get<Object>()["buildingInventory"].get<Array>()[int(Facility::Farm)]=num(1);rejects(bad,"retired ready buildings rejected");
  bad=valid;bad.get<Object>()["buildingQueue"].get<Array>()[0].get<Object>()["progress"]=num(101);rejects(bad,"building progress beyond total rejected");
  bad=valid;bad.get<Object>()["buildingQueue"].get<Array>()[0].get<Object>()["total"]=num(99);rejects(bad,"fabrication cost cannot change in save");
  bad=valid;auto repeated=bad.get<Object>()["buildingQueue"].get<Array>()[0];bad.get<Object>()["buildingQueue"]=Json(Array(65,repeated));rejects(bad,"oversized fabrication queue rejected");
#endif
  // Version 3 unfinished foundations preserve their fraction exactly once.
  Simulation foundation(w);foundation.start(fra);
  foundation.state.sites.push_back({foundation.state.capital,Facility::Factory,60,0,true,foundation.state.regions[foundation.state.capital].center,false});
  foundation.queueUnit(UnitType::Infantry,producer(foundation,Facility::ArmyBase));int originalSource=foundation.state.units.back().sourceSite;
  foundation.save(save);auto v3=readJson(save);v3.get<Object>()["version"]=num(3);v3.get<Object>().erase("buildingQueue");v3.get<Object>().erase("buildingInventory");writeSave(save,v3);
  Simulation v3restored(w);v3restored.load(save);
  check(v3restored.state.buildingQueue.size()==1&&v3restored.state.sites.back().destroyed,"v3 foundation becomes one fabrication item");
  near(v3restored.state.buildingQueue.front().progress,40,"v3 fabrication preserves fraction");
  check(v3restored.state.units.back().sourceSite==originalSource&&v3restored.state.sites[originalSource].type==Facility::ArmyBase,"v3 migration preserves producer site indices");
  check(v3restored.state.buildingInventory[int(Facility::Factory)]==0,"v3 migration grants no free ready building");
  check(v3restored.queueBuilding(Facility::Laboratory),"second fabrication queued");double nextProgress=v3restored.state.buildingQueue.back().progress;
  v3restored.tick();near(v3restored.state.buildingQueue.back().progress,nextProgress,"fabrication follows FIFO order");
  // Legacy v2 snapshots migrate time-based queues and preserve established control.
  Simulation legacy(w);legacy.start(fra);legacy.state.sites.push_back({legacy.state.capital,Facility::Factory,17.5,0,true,legacy.state.regions[legacy.state.capital].center,false});
  legacy.queueUnit(UnitType::Infantry,producer(legacy,Facility::ArmyBase));legacy.state.units.back().remaining=6;legacy.state.units.back().total=12;
  legacy.state.regions[24].owner=fra;legacy.save(save);auto old=readJson(save);old.get<Object>()["version"]=num(2);old.get<Object>().erase("settlements");old.get<Object>().erase("warheadWork");old.get<Object>().erase("warheadsQueued");writeSave(save,old);
  Simulation migrated(w);migrated.load(save);
  near(migrated.state.buildingQueue.front().progress,migrated.buildCost(Facility::Factory)*0.5,"legacy building completion fraction migrated");
  check(migrated.state.sites.back().destroyed&&migrated.state.buildingInventory[int(Facility::Factory)]==0,"legacy foundation never duplicates ready inventory");
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
  near(nuclearWork-nuclear.state.warheadWork,nuclear.state.productionRate*simulationTickSeconds*0.1,"warhead consumes national production");
  nuclear.save(save);Simulation arsenal(w);arsenal.load(save);
  check(arsenal.state.warheadsQueued==1,"strategic production queue saved");
  near(arsenal.state.warheadWork,nuclear.state.warheadWork,"strategic work saved without advancement");
  for(int n=0;n<10000&&arsenal.state.stock[6]<1;++n)arsenal.tick();
  check(arsenal.state.stock[6]==1&&arsenal.state.warheadsQueued==0,"warhead completes with finite work");
  check(!arsenal.launchNuclear(24),"nuclear release requires war");arsenal.declareWar();
  check(!arsenal.launchNuclear(24),"nuclear release requires operational missile site");
  check(arsenal.queueBuilding(Facility::Silo),"missile site manufacture unlocked");
  for(int n=0;n<10000&&arsenal.state.buildingInventory[int(Facility::Silo)]==0;++n)arsenal.tick();
  check(arsenal.build(arsenal.state.capital,Facility::Silo),"ready missile site placed");
  check(arsenal.launchNuclear(24),"nuclear delivery permitted once ready");
  check(arsenal.state.defeat&&arsenal.state.nuclearUsed&&!arsenal.state.war&&arsenal.state.stability<50,"scripted second strike ends campaign");
  check(!arsenal.launchNuclear(24)&&!arsenal.produceWarhead(),"ended campaign rejects nuclear orders");
  std::filesystem::remove(save);std::filesystem::remove(save+".bak");
  std::cout<<"Passed "<<checks<<" checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<"\n";return 1;}}
