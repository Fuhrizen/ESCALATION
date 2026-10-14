#include "escalation/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace escalation {
double distanceKm(Point a,Point b) {
  constexpr double radians=0.017453292519943295;
  const double latitude=(b.y-a.y)*radians;
  const double longitude=std::remainder(b.x-a.x,360.0)*radians;
  const double h=std::pow(std::sin(latitude/2),2)+std::cos(a.y*radians)*std::cos(b.y*radians)*std::pow(std::sin(longitude/2),2);
  return 6371.0088*2*std::asin(std::sqrt(std::clamp(h,0.0,1.0)));
}
UnitStats unitStats(UnitType type) {
  // Strategic map speeds deliberately compress travel; weapon ranges stay local.
  switch(type){
    case UnitType::Infantry:return {.65,3,1.3};
    case UnitType::Armour:return {1,5,1.8};
    case UnitType::Artillery:return {.55,30,1.0};
    case UnitType::Fighter:case UnitType::Bomber:case UnitType::Destroyer:case UnitType::Submarine:return {0,0,0};
    case UnitType::SpecialForces:return {.8,2,1.2};
    case UnitType::MissileLauncher:return {.6,120,1.2};
    case UnitType::Count:break;
  }
  return {0,0,0};
}
bool groundUnit(UnitType t){return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces||t==UnitType::MissileLauncher;}
bool canEngage(UnitType a,UnitType b){return groundUnit(a)&&groundUnit(b);}
bool Simulation::hasTech(int t) const{return t>=0&&t<int(state.completed.size())&&state.completed[t];}
bool Simulation::visibleResearch(int t) const {
  if(t<0||t>=int(techs.size())||state.player<0)return false;
  if(hasTech(t))return true;
  for(const auto& job:state.research)if(job.tech==t)return true;
  for(int p:techs[t].prerequisites)if(!hasTech(p))return false;
  return true;
}
double Simulation::unitSpeedKmPerSecond(const Unit& unit) const {
  double speed=unitStats(unit.type).speedKmPerSecond;
  if(unit.owner==state.player){
    if(hasTech(1))speed*=1.10;
    if(unit.type==UnitType::Infantry&&hasTech(4))speed*=1.15;
    if(unit.type==UnitType::Armour&&hasTech(7))speed*=1.15;
    if(unit.type==UnitType::SpecialForces&&hasTech(49))speed*=1.15;
    for(int i=98;i<112;++i)if(hasTech(i))speed*=1.03;
  }
  if(groundUnit(unit.type))speed*=1+roadMovementBonus(unit.position);
  return speed;
}
double Simulation::unitRangeKm(const Unit& unit) const {
  double range=unitStats(unit.type).rangeKm;
  if(unit.owner==state.player){
    for(int i=28;i<42;++i)if(hasTech(i))range*=1.02;
    if(unit.type==UnitType::Artillery&&hasTech(11))range*=1.20;
    if(unit.type==UnitType::MissileLauncher&&hasTech(65))range*=1.20;
  }
  return range;
}
double Simulation::unitDamagePerSecond(const Unit& u,UnitType defender) const {
  double damage=unitStats(u.type).damagePerSecond;
  if(u.owner==state.player){
    if(u.type==UnitType::Infantry&&hasTech(2))damage*=1.10;
    if(hasTech(13))damage*=1.10;
    if(u.type==UnitType::Infantry&&hasTech(8))damage*=1.10;
    if(u.type==UnitType::Infantry&&defender==UnitType::Armour&&hasTech(18))damage*=1.30;
    if(u.type==UnitType::Artillery&&hasTech(10))damage*=1.20;
    if(u.type==UnitType::SpecialForces&&hasTech(48))damage*=1.15;
    if(u.type==UnitType::MissileLauncher&&hasTech(63))damage*=1.15;
  }
  return damage;
}
double Simulation::unitProtection(const Unit& u) const {
  if(u.owner!=state.player||!groundUnit(u.type))return 0;
  double protection=hasTech(3)?.05:0;
  for(int i=14;i<28;++i)if(i!=17&&i!=18&&hasTech(i))protection+=.02;
  if(u.type==UnitType::Infantry&&hasTech(6))protection+=.10;
  return std::min(.65,protection);
}
int Simulation::readyUnitCount(UnitType type) const {
  int count=0;for(const auto& u:state.units)if(u.owner==state.player&&u.type==type&&groundUnit(type)&&u.health>0&&u.remaining<=0&&!u.deployed)++count;return count;
}
std::string Simulation::deploymentReason(UnitType type,Point p) const {
  if(state.player<0||state.victory||state.defeat)return "No active campaign";
  if(!groundUnit(type))return "Retired air or naval unit";
  if(readyUnitCount(type)<=0)return "Manufacture this formation first";
  if(!std::isfinite(p.x)||!std::isfinite(p.y)||p.y< -90||p.y>90)return "Place on controlled land";
  int r=regionAt(p);if(r<0||state.regions[r].owner!=state.player)return "Place on controlled land";
  return "";
}
bool Simulation::deployUnit(UnitType type,Point p){
  if(!deploymentReason(type,p).empty())return false;
  for(auto& u:state.units)if(u.owner==state.player&&u.type==type&&u.health>0&&u.remaining<=0&&!u.deployed){u.deployed=true;u.region=regionAt(p);u.position=u.destination=p;u.target=-1;log(std::string("Deployed: ")+unitNames[int(type)]);refreshRates();return true;}
  return false;
}
std::vector<Capability> Simulation::unitCapabilities(size_t index) const {
  if(index>=state.units.size())return {};
  const auto& u=state.units[index];if(!groundUnit(u.type)||u.health<=0)return {};
  std::vector<Capability> result;
  auto number=[](double v){std::ostringstream text;text<<std::fixed<<std::setprecision(2)<<v;return text.str();};
  result.push_back({"Movement",number(unitSpeedKmPerSecond(u))+" km/s","movement",u.deployed});
  result.push_back({"Weapon range",number(unitRangeKm(u))+" km","range",u.deployed});
  result.push_back({"Ground damage",number(unitDamagePerSecond(u))+" /s","weapon",u.deployed});
  result.push_back({"Damage protection",number(unitProtection(u)*100)+"%","shield",u.deployed});
  if(u.type!=UnitType::MissileLauncher)result.push_back({"Settlement occupation","Requires presence within 5 km","capture",u.deployed});
  if(u.type==UnitType::Infantry&&u.owner==state.player&&hasTech(18))result.push_back({"Anti-tank weapons",number(unitDamagePerSecond(u,UnitType::Armour))+" /s against armour","weapon",u.deployed});
  if(u.owner==state.player&&hasTech(12))result.push_back({"Field medicine","Recover 0.10 health/s while disengaged","medical",u.deployed});
  if(u.type==UnitType::Infantry&&u.owner==state.player&&hasTech(6))result.push_back({"Body armour","Infantry incoming damage -10%","shield",u.deployed});
  if(u.type==UnitType::Infantry&&u.owner==state.player&&hasTech(8))result.push_back({"Small arms","Infantry weapon damage +10%","weapon",u.deployed});
  if(u.type==UnitType::Infantry&&u.owner==state.player&&hasTech(17))result.push_back({"MANPADS","Equipment reserved; air targets pending","shield",false});
  if(!u.deployed)result.push_back({"Manufacture",u.remaining>0?"In production":"Ready to deploy","factory",false});
  return result;
}
std::vector<Capability> Simulation::siteCapabilities(size_t index) const {
  if(index>=state.sites.size())return {};
  const auto& site=state.sites[index];
  if(site.destroyed||site.type==Facility::Airbase||site.type==Facility::Shipyard)return {};
  bool active=site.enabled&&site.remaining<=0&&site.region>=0&&site.region<int(state.regions.size())&&state.regions[site.region].owner==state.player;
  std::vector<Capability> result;
  double income=buildingIncome(site.type);if(income>0)result.push_back({"Regional production",std::to_string(int(income))+" base output","factory",active});
  if(site.type==Facility::Laboratory)result.push_back({"Research allocation","Contributes laboratory capacity (+8%)","research",active});
  if(site.type==Facility::Factory||site.type==Facility::ArmyBase||site.type==Facility::Silo){
    for(UnitType type:{UnitType::Infantry,UnitType::Armour,UnitType::Artillery,UnitType::SpecialForces,UnitType::MissileLauncher}){
      int prerequisite=type==UnitType::Armour?7:type==UnitType::Artillery?9:type==UnitType::SpecialForces?46:type==UnitType::MissileLauncher?62:-1;
      bool source=site.type==Facility::Factory||(site.type==Facility::ArmyBase&&type!=UnitType::MissileLauncher)||(site.type==Facility::Silo&&type==UnitType::MissileLauncher);
      if(source&&(prerequisite<0||hasTech(prerequisite)))result.push_back({"Manufacture",unitNames[int(type)],"factory",active});
    }
  }
  bool laboratory=false;for(const auto& lab:state.sites)if(lab.type==Facility::Laboratory&&!lab.destroyed&&lab.enabled&&lab.remaining<=0&&lab.region>=0&&lab.region<int(state.regions.size())&&state.regions[lab.region].owner==state.player)laboratory=true;
  if(site.type==Facility::Factory&&hasTech(59)&&hasTech(87))result.push_back({"Strategic manufacture","Warheads; requires an operational research centre","factory",active&&laboratory});
  if(site.type==Facility::Silo&&hasTech(60))result.push_back({"Nuclear delivery","Strategic warhead release during war","weapon",active});
  result.push_back({"Operation",active?"Operational":site.remaining>0?"Under construction":"Inactive","building",active});
  return result;
}
int Simulation::mainSettlement(int region) const {
  int result=-1;
  for(size_t i=0;i<state.settlements.size();++i)if(state.settlements[i].region==region&&(result<0||state.settlements[i].population>state.settlements[result].population))result=int(i);
  return result;
}
}
