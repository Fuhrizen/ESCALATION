#include "escalation/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace escalation {
int nearestCoastNation(const World& world,Point p){
  double closest=1e20;int nation=-1;double scale=std::max(.2,std::cos(p.y*.01745329252));
  for(size_t c=0;c<world.countries.size();++c)for(const auto& polygon:world.countries[c].polygons)for(const auto& ring:polygon.rings)for(size_t n=1;n<ring.size();++n){
    Point a=ring[n-1],b=ring[n];double ax=std::remainder(a.x-p.x,360.0)*scale,ay=a.y-p.y,bx=ax+std::remainder(b.x-a.x,360.0)*scale,by=b.y-p.y;
    double dx=bx-ax,dy=by-ay,den=dx*dx+dy*dy,t=den>0?std::clamp(-(ax*dx+ay*dy)/den,0.0,1.0):0;
    double d=std::hypot(ax+t*dx,ay+t*dy);if(d<closest){closest=d;nation=int(c);}
  }
  return nation;
}
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
    case UnitType::Infantry:return {.65,18,1.3};
    case UnitType::Armour:return {1,28,1.8};
    case UnitType::Artillery:return {.55,90,1.0};
    case UnitType::Destroyer:return {.85,160,1.1};
    case UnitType::Submarine:return {.65,220,.9};
    case UnitType::Cruiser:return {.75,480,1.8};
    case UnitType::MissileSubmarine:return {.55,650,1.4};
    case UnitType::Fighter:case UnitType::Bomber:return {0,0,0};
    case UnitType::SpecialForces:return {.8,12,1.2};
    case UnitType::MissileLauncher:return {.6,600,1.2};
    case UnitType::Count:break;
  }
  return {0,0,0};
}
bool groundUnit(UnitType t){return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces||t==UnitType::MissileLauncher;}
bool navalUnit(UnitType t){return t==UnitType::Destroyer||t==UnitType::Submarine||t==UnitType::Cruiser||t==UnitType::MissileSubmarine;}
bool activeUnit(UnitType t){return groundUnit(t)||navalUnit(t);}
TargetDomain targetDomain(UnitType t){return (t==UnitType::Submarine||t==UnitType::MissileSubmarine)?TargetDomain::Submerged:navalUnit(t)?TargetDomain::Naval:groundUnit(t)?TargetDomain::Ground:TargetDomain::Air;}
bool weaponTargets(const WeaponProfile& w,UnitType t){return activeUnit(t)&&(w.targetMask&unsigned(targetDomain(t)))&&(!w.armourOnly||t==UnitType::Armour);}
std::vector<WeaponProfile> baseWeapons(UnitType t){
  // These are strategic-map gameplay values, not real-world equipment specifications.
  auto weapon=[](const char* id,const char* name,unsigned mask,double lo,double hi,double damage,double cooldown,int rounds,int reserve,double reload,double speed,const char* behaviour){
    WeaponProfile w;w.id=id;w.name=name;w.targetMask=mask;w.minRangeKm=lo;w.maxRangeKm=hi;w.damage=damage;w.cooldownSeconds=cooldown;w.magazine=rounds;w.reserveAmmo=reserve;w.reloadSeconds=reload;w.projectileSpeedKmPerSecond=speed;w.behaviour=behaviour;return w;
  };
  constexpr unsigned ground=unsigned(TargetDomain::Ground),naval=unsigned(TargetDomain::Naval),sub=unsigned(TargetDomain::Submerged);
  switch(t){
    case UnitType::Infantry:return {weapon("rifle","Rifles",ground,0,18,.325,.25,120,2400,3,0,"Direct fire")};
    case UnitType::Armour:return {weapon("tank_gun","Tank gun",ground,0,28,7.2,4,24,120,8,8,"Direct fire"),weapon("machine_gun","Machine gun",ground,0,12,.3,1,120,1200,4,0,"Direct fire")};
    case UnitType::Artillery:return {weapon("howitzer","Indirect artillery",ground,4,90,8,8,12,120,12,4,"Indirect fire")};
    case UnitType::SpecialForces:return {weapon("carbine","Carbines",ground,0,12,.3,.25,120,2400,3,0,"Direct fire")};
    case UnitType::MissileLauncher:return {weapon("ground_missile","Tactical missiles",ground,12,600,22,24,4,24,20,12,"Guided missile")};
    case UnitType::Destroyer:return {weapon("naval_gun","Coastal gun",ground|naval,0,24,4,4,30,300,10,8,"Direct coastal fire"),weapon("anti_ship","Anti-ship missiles",naval,8,160,12,12,4,20,24,16,"Guided anti-ship missile"),weapon("asw_torpedo","ASW torpedoes",sub,0,32,10,12,4,20,20,1.5,"Anti-submarine torpedo")};
    case UnitType::Submarine:return {weapon("sub_torpedo","Torpedoes",naval|sub,0,40,12,14,4,20,24,1.5,"Submerged torpedo"),weapon("cruise","Cruise missiles",ground,18,220,10,16,4,16,28,14,"Land attack cruise missile")};
    case UnitType::Cruiser:return {weapon("naval_gun","Naval gun",ground|naval,0,32,6,4,40,320,12,8,"Direct coastal fire"),weapon("anti_ship","Heavy anti-ship missiles",naval,12,280,20,12,8,32,26,16,"Guided anti-ship missile"),weapon("cruise","Land attack cruise missiles",ground,25,480,22,16,12,36,30,14,"Land attack cruise missile"),weapon("asw_torpedo","Defensive ASW torpedoes",sub,0,16,8,16,2,12,26,1.5,"Anti-submarine torpedo")};
    case UnitType::MissileSubmarine:return {weapon("sub_torpedo","Defensive torpedoes",naval|sub,0,28,8,18,2,12,28,1.5,"Submerged torpedo"),weapon("cruise","Cruise missile salvo",ground,25,650,24,8,16,48,36,14,"Land attack cruise missile")};
    default:return {};
  }
}
int weaponSlot(UnitType type,const std::string& id){auto slots=baseWeapons(type);for(size_t n=0;n<slots.size();++n)if(slots[n].id==id||(id=="depth_charge"&&slots[n].id=="asw_torpedo"))return int(n);return type==UnitType::Infantry&&id=="atgm"?int(slots.size()):-1;}
bool canEngage(UnitType a,UnitType b){for(const auto& w:baseWeapons(a))if(weaponTargets(w,b))return true;return false;}
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
  if(unit.owner==state.player&&navalUnit(unit.type)&&hasTech(121))speed*=1.15;
  if(groundUnit(unit.type))speed*=1+roadMovementBonus(unit.position);
  return speed;
}
std::vector<WeaponProfile> Simulation::unitWeapons(const Unit& u) const {
  auto weapons=baseWeapons(u.type);
  if((u.type==UnitType::Destroyer||u.type==UnitType::Cruiser)&&(u.owner!=state.player||!hasTech(116)))for(auto& w:weapons)if(w.id=="asw_torpedo"){w.id="depth_charge";w.name="Depth charges";w.behaviour="Snapshot anti-submarine attack";w.maxRangeKm=8;w.damage=4;}
  if(u.owner!=state.player){weapons.erase(std::remove_if(weapons.begin(),weapons.end(),[](const WeaponProfile& w){return w.id=="anti_ship"||w.id=="cruise";}),weapons.end());return weapons;}
  weapons.erase(std::remove_if(weapons.begin(),weapons.end(),[&](const WeaponProfile& w){return (w.id=="asw_torpedo"&&!hasTech(116))||(w.id=="anti_ship"&&!hasTech(117))||(w.id=="cruise"&&!hasTech(119));}),weapons.end());
  if(u.type==UnitType::Infantry&&hasTech(18)){
    WeaponProfile at;at.id="atgm";at.name="Anti-tank guided missiles";at.behaviour="Guided anti-armour missile";at.targetMask=unsigned(TargetDomain::Ground);at.armourOnly=true;at.minRangeKm=.5;at.maxRangeKm=40;at.damage=7.8;at.cooldownSeconds=20;at.magazine=4;at.reserveAmmo=24;at.reloadSeconds=20;at.projectileSpeedKmPerSecond=5;weapons.push_back(at);
  }
  for(auto& w:weapons){
    if(groundUnit(u.type)){
      for(int i=28;i<42;++i)if(hasTech(i))w.maxRangeKm*=1.02;
      if(hasTech(13))w.damage*=1.10;
    }
    if(u.type==UnitType::Infantry&&w.id=="rifle") {if(hasTech(2))w.damage*=1.10;if(hasTech(8))w.damage*=1.10;}
    if(u.type==UnitType::Artillery){if(hasTech(10))w.damage*=1.20;if(hasTech(11))w.maxRangeKm*=1.20;}
    if(u.type==UnitType::SpecialForces&&hasTech(48))w.damage*=1.15;
    if(u.type==UnitType::MissileLauncher){if(hasTech(63))w.damage*=1.15;if(hasTech(65))w.maxRangeKm*=1.20;}
    if(w.id=="naval_gun"&&hasTech(115))w.damage*=1.20;
    if(w.id=="sub_torpedo"&&hasTech(118))w.damage*=1.20;
    if(navalUnit(u.type)&&hasTech(120))w.maxRangeKm*=1.10;
  }
  return weapons;
}
double Simulation::unitRangeKm(const Unit& u) const {double range=0;for(const auto& w:unitWeapons(u))range=std::max(range,w.maxRangeKm);return range;}
double Simulation::engagementRange(const Unit& a,const Unit& b) const {double range=0;for(const auto& w:unitWeapons(a))if(weaponTargets(w,b.type))range=std::max(range,w.maxRangeKm);return range;}
bool Simulation::canEngage(const Unit& a,const Unit& b) const {if(!detectsSubmarine(a,b))return false;double d=distanceKm(a.position,b.position);for(const auto& w:unitWeapons(a))if(weaponTargets(w,b.type)&&d>=w.minRangeKm&&d<=w.maxRangeKm)return true;return false;}
double Simulation::sonarRangeKm(const Unit& u) const {
  double range=u.type==UnitType::Destroyer?32:u.type==UnitType::Cruiser?16:u.type==UnitType::Submarine?45:u.type==UnitType::MissileSubmarine?28:0;
  if(u.owner==state.player&&hasTech(120))range*=1.35;
  return range;
}
bool Simulation::detectsSubmarine(const Unit& observer,const Unit& target) const {
  if(targetDomain(target.type)!=TargetDomain::Submerged||observer.owner==target.owner)return true;
  double range=sonarRangeKm(observer);
  if(target.owner==state.player&&hasTech(121))range*=.8;
  return range>0&&distanceKm(observer.position,target.position)<=range;
}
double Simulation::unitDamagePerSecond(const Unit& u,UnitType defender) const {double damage=0;for(const auto& w:unitWeapons(u))if(weaponTargets(w,defender))damage+=w.damage/w.cooldownSeconds;return damage;}
double Simulation::unitProtection(const Unit& u) const {
  if(u.owner!=state.player||!groundUnit(u.type))return 0;
  double protection=hasTech(3)?.05:0;
  for(int i=14;i<28;++i)if(i!=17&&i!=18&&hasTech(i))protection+=.02;
  if(u.type==UnitType::Infantry&&hasTech(6))protection+=.10;
  return std::min(.65,protection);
}
int Simulation::readyUnitCount(UnitType type) const {
  int count=0;for(const auto& u:state.units)if(u.owner==state.player&&u.type==type&&activeUnit(type)&&u.health>0&&u.remaining<=0&&!u.deployed)++count;return count;
}
std::string Simulation::deploymentReason(UnitType type,Point p) const {
  if(state.player<0||state.victory||state.defeat)return "No active campaign";
  if(!activeUnit(type))return "Retired air unit";
  if(readyUnitCount(type)<=0)return "Manufacture this formation first";
  if(navalUnit(type)){
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||p.y< -90||p.y>90)return "Place in coastal water";
    if(world_->pick(p)>=0)return "Place in coastal water";
    if(world_->canonical(nearestCoastNation(*world_,p))!=state.player)return "Deploy beside your own coast";
    bool yard=false;for(const auto& site:state.sites)if(site.type==Facility::Shipyard&&!site.destroyed&&site.enabled&&site.remaining<=0&&site.region>=0&&site.region<int(state.regions.size())&&state.regions[site.region].owner==state.player&&distanceKm(p,site.position)<=30)yard=true;
    return yard?"":"Deploy within 30 km of an operational friendly shipyard";
  }
  if(!std::isfinite(p.x)||!std::isfinite(p.y)||p.y< -90||p.y>90)return "Place on controlled land";
  int r=regionAt(p);if(r<0||state.regions[r].owner!=state.player)return "Place on controlled land";
  return "";
}
bool Simulation::deployUnit(UnitType type,Point p){
  if(!deploymentReason(type,p).empty())return false;
  for(auto& u:state.units)if(u.owner==state.player&&u.type==type&&u.health>0&&u.remaining<=0&&!u.deployed){u.deployed=true;u.region=regionAt(p);if(navalUnit(type)){double nearest=1e20;for(const auto& yard:state.sites)if(yard.type==Facility::Shipyard&&!yard.destroyed&&yard.enabled&&yard.remaining<=0&&yard.region>=0&&yard.region<int(state.regions.size())&&state.regions[yard.region].owner==state.player){double d=distanceKm(p,yard.position);if(d<nearest){nearest=d;u.region=yard.region;}}}u.position=u.destination=p;u.target=-1;log(std::string("Deployed: ")+unitNames[int(type)]);refreshRates();return true;}
  return false;
}
std::vector<Capability> Simulation::unitCapabilities(size_t index) const {
  if(index>=state.units.size())return {};
  const auto& u=state.units[index];if(!activeUnit(u.type)||u.health<=0)return {};
  std::vector<Capability> result;
  auto number=[](double v){std::ostringstream text;text<<std::fixed<<std::setprecision(2)<<v;return text.str();};
  result.push_back({"Movement",number(unitSpeedKmPerSecond(u))+" km/s","movement",u.deployed});
  for(const auto& w:unitWeapons(u)){
    std::string domains;if(w.targetMask&1)domains=w.armourOnly?"armour":"ground";if(w.targetMask&2)domains+=(domains.empty()?"":", ")+std::string("surface ships");if(w.targetMask&4)domains+=(domains.empty()?"":", ")+std::string("submarines");
    result.push_back({w.name,number(w.minRangeKm)+"–"+number(w.maxRangeKm)+" km; "+number(w.damage)+" damage / "+number(w.cooldownSeconds)+"s; "+domains,"weapon",u.deployed});
    result.push_back({w.name+" ammunition",std::to_string(w.magazine)+" rounds; "+number(w.reloadSeconds)+"s reload; "+std::to_string(w.reserveAmmo)+" reserve","weapon",u.deployed});
  }
  result.push_back({"Damage protection",number(unitProtection(u)*100)+"%","shield",u.deployed});
  if(groundUnit(u.type)&&u.type!=UnitType::MissileLauncher)result.push_back({"Settlement occupation","Requires presence within 5 km","capture",u.deployed});
  if(u.type==UnitType::Infantry&&u.owner==state.player&&hasTech(18))result.push_back({"Anti-tank weapons",number(unitDamagePerSecond(u,UnitType::Armour))+" /s against armour","weapon",u.deployed});
  if(navalUnit(u.type))result.push_back({"Sonar",number(sonarRangeKm(u))+" km submarine detection radius","shield",u.deployed});
  if(navalUnit(u.type))result.push_back({"Port service",number(u.portServiceSeconds)+" / 30s","factory",u.deployed&&!u.moving});
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
  if(site.destroyed||site.type==Facility::Airbase)return {};
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
  if(site.type==Facility::Shipyard){if(hasTech(117))result.push_back({"Manufacture","Cruiser","factory",active});if(hasTech(119))result.push_back({"Manufacture","Cruise missile submarine","factory",active});if(hasTech(113))result.push_back({"Manufacture","Destroyer","factory",active});if(hasTech(114))result.push_back({"Manufacture","Submarine","factory",active});}
  bool laboratory=false;for(const auto& lab:state.sites)if(lab.type==Facility::Laboratory&&!lab.destroyed&&lab.enabled&&lab.remaining<=0&&lab.region>=0&&lab.region<int(state.regions.size())&&state.regions[lab.region].owner==state.player)laboratory=true;
  if(site.type==Facility::Factory&&hasTech(59)&&hasTech(87))result.push_back({"Strategic manufacture","Warheads; requires an operational research centre","factory",active&&laboratory});
  if(site.type==Facility::Silo&&hasTech(58))result.push_back({"Ballistic delivery",std::to_string(int(siteMissileRange(index)))+" km; conventional ground strike; 45s reload","weapon",active});
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
