#include "escalation/simulation.hpp"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
using namespace escalation;
namespace {
int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
Country country(const char* code,double left,double right){
  Country c;c.code=code;c.name=code;c.center={(left+right)/2,0};
  Polygon p;p.rings.push_back({{left,-5},{right,-5},{right,5},{left,5},{left,-5}});c.polygons.push_back(p);c.triangles={std::array<Point,3>{{{left,-5},{right,-5},{right,5}}},std::array<Point,3>{{{left,-5},{right,5},{left,5}}}};return c;
}
World world(bool sea=false){
  World w;w.countries={country("AAA",-8,sea?-3:0),country("BBB",sea?3:0,8)};
  w.roads.push_back({{{-7,0},{7,0}}});w.rebuildRoadIndex();return w;
}
void campaign(Simulation& s,bool sea=false){
  s.state=Campaign{};s.state.player=0;s.state.enemy=1;s.state.capital=0;s.state.enemyCapital=3;
  s.state.completed.resize(s.techs.size());
  for(int n=0;n<4;++n){Region r;r.nation=r.owner=n<2?0:1;r.center={n==0?-6.0:n==1?(sea?-4.0:-1.0):n==2?(sea?4.0:1.0):6.0,0};
    for(int k=0;k<4;++k)if(k!=n)r.neighbours.push_back(k);
    s.state.regions.push_back(r);
  }
  s.state.settlements={{"Home west",{-6,0},1000,0,0,0},{"Home east",{sea?-4.0:-1.0,0},1000,1,0,0},
    {"Enemy west",{sea?4.0:0.4,0},1000,2,1,0},{"Enemy east",{sea?4.8:2.2,0},1000,2,1,0},{"Enemy reserve",{6,0},1000,3,1,0}};
}
Unit formation(int owner,int region,Point p,UnitType type=UnitType::Infantry){Unit u;u.owner=owner;u.region=region;u.position=u.destination=p;u.type=type;return u;}
}
int main(){try{
  World w=world();Simulation s(w);campaign(s);
  s.state.units.push_back(formation(0,1,{-1,0}));
  check(!s.orderUnitAt(0,{1,0}),"peace forbids hostile ground orders");
  check(!s.orderUnitAt(0,{-1,8}),"ground orders reject ocean destinations");
  check(!s.orderUnitAt(0,{NAN,0}),"movement rejects non-finite coordinates");
  check(s.orderUnitAt(0,{-2,0}),"direct friendly point order succeeds");
  Point before=s.state.units[0].position;s.tick();
  check(std::abs(distanceKm(before,s.state.units[0].position)-.65*1.15*simulationTickSeconds)<1e-5,"real road gives exactly fifteen percent movement bonus");
  check(s.state.units[0].moving,"short movement advances without teleporting to destination");
  Point existingDestination=s.state.units[0].destination;
  check(!s.orderUnitAt(0,{-1,8})&&s.state.units[0].moving&&distance(existingDestination,s.state.units[0].destination)==0,"rejected reorder preserves the previous valid route");
  s.holdUnit(0);check(!s.state.units[0].moving&&s.state.units[0].waypoints.empty(),"hold clears the full geometric route");
  Simulation orders(w);campaign(orders);
  orders.state.units={formation(0,0,{-6,1})};
  check(orders.unitArrivalETA(0)==0,"idle movement arrival ETA is zero");
  check(orders.orderUnitAt(0,{-5.99,1}),"initial queued-route leg succeeds");
  double firstETA=orders.unitArrivalETA(0);
  check(firstETA>0&&std::isfinite(firstETA),"moving formation has a finite arrival ETA");
  check(orders.orderUnitAt(0,{-5.98,1},true)&&orders.orderUnitAt(0,{-5.97,1},true),"shift orders append multiple destinations");
  check(orders.state.units[0].pendingDestinations.size()==2&&distance(orders.state.units[0].destination,{-5.99,1})<1e-8,"append preserves current leg and queue order");
  check(orders.unitPlannedPath(0).size()==3&&distance(orders.unitPlannedPath(0).back(),{-5.97,1})<1e-8,"planned path includes queued route geometry in order");
  check(std::abs(orders.unitArrivalETA(0)-firstETA*3)<1e-5,"arrival ETA includes every queued leg");
  check(!orders.orderUnitAt(0,{-5,8},true)&&orders.state.units[0].pendingDestinations.size()==2,"invalid appended destination leaves active route and queue intact");
  check(!orders.orderUnitAt(0,{-5,8})&&orders.state.units[0].pendingDestinations.size()==2,"invalid replacement preserves active queue");
  for(int n=0;n<100&&orders.state.units[0].moving;++n)orders.tick();
  check(!orders.state.units[0].moving&&orders.state.units[0].pendingDestinations.empty()&&distance(orders.state.units[0].position,{-5.97,1})<1e-8,"all queued destinations advance and finish automatically");
  check(orders.orderUnitAt(0,{-5.9,1},true)&&orders.state.units[0].pendingDestinations.empty(),"append on idle unit starts immediately");
  check(orders.orderUnitAt(0,{-5.8,1},true)&&orders.orderUnitAt(0,{-5.7,1}),"ordinary order replaces all queued legs");
  check(orders.state.units[0].pendingDestinations.empty()&&distance(orders.state.units[0].destination,{-5.7,1})<1e-8,"replacement uses only new destination");
  orders.orderUnitAt(0,{-5.6,1},true);orders.holdUnit(0);
  check(!orders.state.units[0].moving&&orders.state.units[0].pendingDestinations.empty()&&orders.unitArrivalETA(0)==0,"stop cancels active and pending movement orders");
  check(s.roadMovementBonus({-1,1})==0,"road bonus never applies away from imported road geometry");
  s.state.units[0].position={-1,1};check(s.orderUnitAt(0,{-2,1}),"off-road point order succeeds");
  before=s.state.units[0].position;s.tick();check(std::abs(distanceKm(before,s.state.units[0].position)-.65*simulationTickSeconds)<1e-5,"off-road movement retains base speed");
  s.state.completed[98]=s.state.completed[99]=true;s.holdUnit(0);s.state.units[0].position={-1,1};
  check(s.orderUnitAt(0,{-2,1}),"support-upgraded movement order succeeds");
  before=s.state.units[0].position;s.tick();check(std::abs(distanceKm(before,s.state.units[0].position)-.65*1.03*1.03*simulationTickSeconds)<1e-5,"support research improves formation speed by three percent per completed technology");
  s.state.units[0].remaining=5;check(!s.orderUnitAt(0,{-3,1}),"unfinished unit cannot move");
  s.state.units[0].remaining=0;s.holdUnit(0);s.state.units[0].position={0.4,0};s.state.units[0].region=2;
  check(s.declareWar(),"war starts once");check(!s.attack(2),"legacy remote offensive action is retired");
  for(int n=0;n<601;++n)s.tick();
  check(s.state.settlements[2].owner==0,"ground presence captures a local settlement automatically");
  check(s.state.settlements[3].owner==1&&s.state.regions[2].owner==1,"one town does not capture a multi-town region");
  Site factory;factory.region=2;factory.type=Facility::ArmyBase;factory.position={2.2,0};s.state.sites.push_back(factory);
  Unit queued=formation(1,2,{2.2,0});queued.sourceSite=0;queued.remaining=queued.total=100000;s.state.units.push_back(queued);
  s.state.units[0].position={2.2,0};for(int n=0;n<361;++n)s.tick();
  check(s.state.regions[2].owner==0&&s.state.settlements[3].owner==0,"region changes owner only when every town is held");
  check(s.state.units[1].health==0,"seized production base cancels former-owner unfinished units");
  check(!s.state.victory,"a single captured region does not end the campaign");
  Simulation local(w);campaign(local);local.state.war=true;
  local.state.units={formation(0,2,{0.4,0}),formation(1,2,{0.41,0})};
  local.state.settlements[2].capture=0.2;local.tick();
  check(local.state.units[0].health<100&&local.state.units[1].health<100,"nearby opposing units trade simultaneous automatic damage");
  check(std::abs(local.state.settlements[2].capture-0.2)<1e-8,"contested towns cannot finish occupation while defenders remain");
  local.state.units[1].position={6,0};double health=local.state.units[0].health;local.tick();
  check(local.state.units[0].health==health,"remote enemy units cannot damage a distant order target");
  check(local.state.units[1].moving&&distance(local.state.units[1].destination,local.state.units[0].position)<1e-8,"basic AI plans a defence of actual incursions");
  Point defender=local.state.units[1].position;local.tick();
  check(distance(defender,local.state.units[1].position)>0,"defensive AI advances along its route");
  check(local.frontlinePressure({0.4,0})>0&&local.frontlinePressure({6,0})<0,"frontline pressure follows both local forces");
  local.state.war=false;check(local.frontlinePressure({0.4,0})==0,"peacetime has no tactical frontline");
  Simulation recapture(w);campaign(recapture);recapture.state.war=true;
  recapture.state.settlements[2].owner=0;
  recapture.state.units={formation(1,2,{.8,0},UnitType::Artillery)};
  for(int n=0;n<int(35/simulationTickSeconds);++n)recapture.tick();
  check(recapture.state.units[0].moving&&distanceKm(recapture.state.units[0].position,{.4,0})<30,"artillery recapture advances through weapon standoff toward the city");
  for(int n=0;n<int(205/simulationTickSeconds);++n)recapture.tick();
  check(distanceKm(recapture.state.units[0].position,{.4,0})<=5&&recapture.state.settlements[2].owner==1,"defensive artillery enters occupation range and retakes a lost main city within four minutes");
  Simulation borderRecapture(w);campaign(borderRecapture);borderRecapture.state.war=true;
  borderRecapture.state.settlements[2].position={.01,0};borderRecapture.state.settlements[2].owner=0;borderRecapture.state.settlements[2].capture=.999;
  borderRecapture.state.units={formation(1,1,{-.01,0},UnitType::Artillery)};
  for(int n=0;n<int(10/simulationTickSeconds);++n)borderRecapture.tick();
  check(borderRecapture.state.units[0].region==2&&borderRecapture.state.settlements[2].owner==1,"recapture crosses the region boundary even when already within five kilometres of a town");
  Simulation withdrawn(w);campaign(withdrawn);withdrawn.state.war=true;
  withdrawn.state.units={formation(0,2,{.4,0}),formation(1,3,{6,0})};withdrawn.tick();
  check(withdrawn.state.units[1].moving,"defender pursues a live intrusion");
  withdrawn.state.units[0].health=0;withdrawn.tick();
  check(!withdrawn.state.units[1].moving&&withdrawn.state.units[1].waypoints.empty(),"defender clears stale pursuit when the intrusion disappears");
  Simulation upgrades(w);campaign(upgrades);
  upgrades.state.units={formation(0,1,{-1,1}),formation(0,1,{-1,1},UnitType::Armour)};
  check(upgrades.visibleResearch(0)&&!upgrades.visibleResearch(5)&&!upgrades.visibleResearch(17),"locked technologies have no visible node");
  upgrades.state.completed[0]=upgrades.state.completed[2]=upgrades.state.completed[5]=true;
  check(upgrades.visibleResearch(17)&&upgrades.visibleResearch(18),"support weapons exposes separate MANPADS and anti-tank forks");
  double speed=upgrades.unitSpeedKmPerSecond(upgrades.state.units[0]);
  upgrades.state.levels[0]=14;check(upgrades.unitSpeedKmPerSecond(upgrades.state.units[0])==speed,"highest tier grants no unrelated mobility");
  double damage=upgrades.unitDamagePerSecond(upgrades.state.units[0],UnitType::Armour);
  upgrades.state.completed[18]=true;
  check(std::abs(upgrades.unitDamagePerSecond(upgrades.state.units[0],UnitType::Armour)-damage*1.30)<1e-8,"ATGM technology increases infantry damage against armour");
  check(upgrades.unitDamagePerSecond(upgrades.state.units[0],UnitType::Infantry)==damage,"ATGM never increases anti-infantry damage");
  upgrades.state.completed[17]=true;
  auto capabilities=upgrades.unitCapabilities(0);bool reserved=false;for(const auto& c:capabilities)if(c.name=="MANPADS")reserved=!c.active;
  check(reserved,"MANPADS inspector honestly reserves unimplemented air effect");
  upgrades.state.completed[3]=true;check(upgrades.unitProtection(upgrades.state.units[0])==.05,"protection research reduces incoming damage");
  upgrades.state.completed[6]=true;
  check(std::abs(upgrades.unitProtection(upgrades.state.units[0])-.15)<1e-8&&std::abs(upgrades.unitProtection(upgrades.state.units[1])-.05)<1e-8,"body armour protects infantry without applying its bonus to vehicles");
  upgrades.state.completed[8]=true;
  check(std::abs(upgrades.unitDamagePerSecond(upgrades.state.units[0],UnitType::Infantry)-damage*1.10)<1e-8,"small arms improves normal infantry damage");
  check(std::abs(upgrades.unitDamagePerSecond(upgrades.state.units[0],UnitType::Armour)-damage*1.10*1.30)<1e-8,"small arms and anti-tank weapons apply their separate real effects");
  Simulation medicine(w);campaign(medicine);medicine.state.completed[12]=true;
  medicine.state.units={formation(0,1,{-1,1})};medicine.state.units[0].health=50;medicine.tick();
  check(std::abs(medicine.state.units[0].health-50.025)<1e-8,"field medicine restores living deployed formations while disengaged");
  medicine.state.units[0].deployed=false;medicine.tick();
  check(std::abs(medicine.state.units[0].health-50.025)<1e-8,"ready inventory does not receive deployed field treatment");
  medicine.state.units[0].deployed=true;medicine.state.war=true;medicine.state.units.push_back(formation(1,1,{-1.001,1}));medicine.tick();
  check(std::abs(medicine.state.units[0].health-(50.025-1.3*simulationTickSeconds))<1e-8,"field medicine pauses within hostile weapon range");
  medicine.state.units[0].health=0;medicine.state.war=false;medicine.tick();check(medicine.state.units[0].health==0,"field medicine never revives destroyed units");
  World islands=world(true);Simulation crossing(islands);campaign(crossing,true);crossing.state.war=true;
  crossing.state.units={formation(0,1,{-4,0})};
  check(!crossing.orderUnitAt(0,{4,0}),"synthetic region adjacency cannot carry ground forces over water");
  crossing.state.levels[2]=14;
  Site shipyard;shipyard.region=1;shipyard.type=Facility::Shipyard;shipyard.position={-4,0};crossing.state.sites.push_back(shipyard);
  check(!crossing.orderUnitAt(0,{4,0}),"legacy naval level and shipyard cannot enable sea transport");
  crossing.state.units.push_back(formation(0,1,{-2,0},UnitType::Destroyer));
  check(!crossing.orderUnitAt(1,{2,0})&&!canEngage(UnitType::Destroyer,UnitType::Infantry),"legacy naval units have no active movement or combat");
  Simulation pacing(w);campaign(pacing);
  pacing.state.units={formation(0,0,{-6,1})};
  check(pacing.orderUnitAt(0,{-6+100.0/111.18,1}),"hundred kilometre travel order succeeds");
  Point start=pacing.state.units[0].position;
  for(int n=0;n<int(120/simulationTickSeconds);++n)pacing.tick();
  double travelled=distanceKm(start,pacing.state.units[0].position);
  check(travelled>77.9&&travelled<78.1&&pacing.state.units[0].moving,"infantry travels 78 km in two minutes and cannot teleport 100 km");
  check(std::abs(distanceKm({179.9,0},{-179.9,0})-22.239)<.01,"kilometre distance wraps the antimeridian");
  Simulation ranged(w);campaign(ranged);ranged.state.war=true;
  ranged.state.units={formation(0,2,{.4,0}),formation(1,2,{.5,0})};
  ranged.tick();check(ranged.state.units[0].health==100&&ranged.state.units[1].health==100,"infantry cannot fire eleven kilometres across the map");
  Simulation battle(w);campaign(battle);battle.state.war=true;
  battle.state.units={formation(0,2,{.4,0}),formation(1,2,{.41,0})};
  for(int n=0;n<int(180/simulationTickSeconds);++n)battle.tick();
  check(battle.state.units[0].health==0||battle.state.units[1].health==0,"close infantry battle resolves within three minutes");
  Simulation occupation(w);campaign(occupation);occupation.state.war=true;
  occupation.state.settlements[2].population=50000;
  occupation.state.units.assign(8,formation(0,2,{.4,0},UnitType::Armour));
  check(occupation.mainSettlement(2)==2,"largest regional population identifies the main city");
  for(int n=0;n<int(149/simulationTickSeconds);++n)occupation.tick();
  check(occupation.state.settlements[2].owner==1,"stacked armour cannot beat the 150 second main-city minimum");
  double capture=occupation.state.settlements[2].capture;
  Simulation checkpoint(w);checkpoint.start(0);checkpoint.state.settlements.front().capture=.42;
  check(checkpoint.orderUnitAt(0,{-6,1})&&checkpoint.orderUnitAt(0,{-5,1},true),"checkpoint stores active and queued orders");
  double savedETA=checkpoint.unitArrivalETA(0);
  checkpoint.save("tactical_restore_test.save");
  Simulation loaded(w);loaded.load("tactical_restore_test.save");std::remove("tactical_restore_test.save");
  check(loaded.state.units[0].pendingDestinations.size()==1&&distance(loaded.state.units[0].pendingDestinations[0],{-5,1})<1e-8,"pending movement destinations survive save and restore");
  check(std::abs(loaded.unitArrivalETA(0)-savedETA)<.01,"restored arrival ETA retains active and pending legs");
  check(std::abs(loaded.state.settlements.front().capture-.42)<1e-5,"partial occupation survives campaign restore");
  Simulation restored(w);restored.state=occupation.state;
  restored.state.units.push_back(formation(1,2,{.41,0}));restored.tick();
  check(std::abs(restored.state.settlements[2].capture-capture)<1e-5,"defender freezes even an almost complete occupation");
  restored.state.units.back().health=0;
  for(int n=0;n<8;++n)restored.tick();
  check(restored.state.settlements[2].owner==0&&restored.state.regions[2].owner==1,"main city alone cannot transfer a region containing another town");
  Simulation regionGuard(w);campaign(regionGuard);regionGuard.state.war=true;
  regionGuard.state.settlements[2].position={-.001,0};
  regionGuard.state.units={formation(0,1,{-.002,0})};regionGuard.tick();
  check(regionGuard.state.settlements[2].capture==0,"nearby troops in another region cannot occupy a settlement");
  Simulation capacity(w);campaign(capacity);Site base;base.type=Facility::ArmyBase;base.region=0;capacity.state.sites.push_back(base);
  capacity.state.units.resize(1000);for(auto& unit:capacity.state.units)unit.health=0;
  check(!capacity.queueUnit(UnitType::Infantry,0),"canceled unit history cannot exceed load-safe campaign capacity");
  capacity.state.units.clear();capacity.state.victory=true;
  check(!capacity.queueUnit(UnitType::Infantry,0),"finished campaign cannot queue new units");
  std::cout<<"Passed "<<checks<<" tactical checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" tactical checks: "<<e.what()<<"\n";return 1;}}
