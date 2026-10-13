#include "escalation/simulation.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace escalation;
namespace {
int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
Country country(const char* code,double left,double right){
  Country c;c.code=code;c.name=code;c.center={(left+right)/2,0};
  Polygon p;p.rings.push_back({{left,-5},{right,-5},{right,5},{left,5},{left,-5}});c.polygons.push_back(p);return c;
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
  check(std::abs(distance(before,s.state.units[0].position)-0.23)<1e-8,"real road gives exactly fifteen percent movement bonus");
  check(s.state.units[0].moving,"short movement advances without teleporting to destination");
  s.holdUnit(0);check(!s.state.units[0].moving&&s.state.units[0].waypoints.empty(),"hold clears the full geometric route");
  check(s.roadMovementBonus({-1,1})==0,"road bonus never applies away from imported road geometry");
  s.state.units[0].position={-1,1};check(s.orderUnitAt(0,{-2,1}),"off-road point order succeeds");
  before=s.state.units[0].position;s.tick();check(std::abs(distance(before,s.state.units[0].position)-0.20)<1e-5,"off-road movement retains base speed");
  s.state.levels[7]=2;s.holdUnit(0);s.state.units[0].position={-1,1};
  check(s.orderUnitAt(0,{-2,1}),"support-upgraded movement order succeeds");
  before=s.state.units[0].position;s.tick();check(std::abs(distance(before,s.state.units[0].position)-0.212)<1e-5,"support research improves formation speed by three percent per level");
  s.state.units[0].remaining=5;check(!s.orderUnitAt(0,{-3,1}),"unfinished unit cannot move");
  s.state.units[0].remaining=0;s.holdUnit(0);s.state.units[0].position={0.4,0};s.state.units[0].region=2;
  check(s.declareWar(),"war starts once");check(!s.attack(2),"legacy remote offensive action is retired");
  for(int n=0;n<24;++n)s.tick();
  check(s.state.settlements[2].owner==0,"ground presence captures a local settlement automatically");
  check(s.state.settlements[3].owner==1&&s.state.regions[2].owner==1,"one town does not capture a multi-town region");
  Site factory;factory.region=2;factory.type=Facility::ArmyBase;factory.position={2.2,0};s.state.sites.push_back(factory);
  Unit queued=formation(1,2,{2.2,0});queued.sourceSite=0;queued.remaining=queued.total=30;s.state.units.push_back(queued);
  s.state.units[0].position={2.2,0};for(int n=0;n<24;++n)s.tick();
  check(s.state.regions[2].owner==0&&s.state.settlements[3].owner==0,"region changes owner only when every town is held");
  check(s.state.units[1].health==0,"seized production base cancels former-owner unfinished units");
  check(!s.state.victory,"a single captured region does not end the campaign");
  Simulation local(w);campaign(local);local.state.war=true;
  local.state.units={formation(0,2,{0.4,0}),formation(1,2,{0.6,0})};
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
  Simulation air(w);campaign(air);air.state.war=true;
  air.state.units={formation(0,2,{1,0},UnitType::Bomber),formation(1,2,{1.1,0})};
  air.state.levels[1]=3;air.tick();check(air.state.units[1].health<98.85,"air research improves bomber damage through its own branch");
  World islands=world(true);Simulation crossing(islands);campaign(crossing,true);crossing.state.war=true;
  crossing.state.units={formation(0,1,{-4,0})};
  check(!crossing.orderUnitAt(0,{4,0}),"synthetic region adjacency cannot carry ground forces over water");
  crossing.state.levels[2]=1;check(!crossing.orderUnitAt(0,{4,0}),"naval research alone does not create transport capacity");
  Site shipyard;shipyard.region=1;shipyard.type=Facility::Shipyard;shipyard.position={-4,0};crossing.state.sites.push_back(shipyard);
  check(crossing.orderUnitAt(0,{4,0}),"naval research and operational controlled shipyard permit embarkation");
  before=crossing.state.units[0].position;crossing.tick();check(distance(before,crossing.state.units[0].position)<0.3,"embarked forces still move continuously");
  crossing.state.units.push_back(formation(0,1,{-2,0},UnitType::Destroyer));
  check(!crossing.orderUnitAt(1,{4,0}),"naval units reject land destinations");
  check(crossing.orderUnitAt(1,{2,0}),"naval units can navigate open water");
  World blocked=world(true);blocked.countries.push_back(country("CCC",-0.5,0.5));
  Simulation neutral(blocked);campaign(neutral,true);neutral.state.war=true;neutral.state.levels[2]=1;neutral.state.sites.push_back(shipyard);
  neutral.state.units={formation(0,1,{-4,0})};
  check(neutral.orderUnitAt(0,{4,0}),"transport route can detour around neutral land");
  for(int n=0;n<100&&neutral.state.units[0].moving;++n){neutral.tick();check(blocked.pick(neutral.state.units[0].position)!=2,"transport never traverses neutral territory");}
  check(!neutral.state.units[0].moving&&distance(neutral.state.units[0].position,{4,0})<1e-8,"detoured transport reaches its ordered destination");
  Simulation capacity(w);campaign(capacity);Site base;base.type=Facility::ArmyBase;base.region=0;capacity.state.sites.push_back(base);
  capacity.state.units.resize(1000);for(auto& unit:capacity.state.units)unit.health=0;
  check(!capacity.queueUnit(UnitType::Infantry,0),"canceled unit history cannot exceed load-safe campaign capacity");
  capacity.state.units.clear();capacity.state.victory=true;
  check(!capacity.queueUnit(UnitType::Infantry,0),"finished campaign cannot queue new units");
  std::cout<<"Passed "<<checks<<" tactical checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" tactical checks: "<<e.what()<<"\n";return 1;}}
