#include "escalation/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace escalation {
namespace {
bool occupying(UnitType t) {
  return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces;
}
bool ready(const Unit& u) { return u.deployed&&u.health>0&&u.remaining<=0; }
double strength(const Unit& u) {
  return (u.type==UnitType::Armour?1.8:u.type==UnitType::Artillery?1.2:1.0)*u.health/100;
}


}

double Simulation::roadMovementBonus(Point p) const {
  const double scale=std::max(0.20,std::cos(p.y*0.01745329252));
  for(int id:world_->nearbyRoads(p,0.10)){
    const auto& road=world_->roads[id];
    for(size_t n=1;n<road.points.size();++n){
      Point a=road.points[n-1],b=road.points[n];
      double ax=std::remainder(a.x-p.x,360.0)*scale,ay=a.y-p.y;
      double bx=ax+std::remainder(b.x-a.x,360.0)*scale,by=b.y-p.y;
      if(std::min(ax,bx)>0.10||std::max(ax,bx)<-0.10||std::min(ay,by)>0.10||std::max(ay,by)<-0.10)continue;
      double dx=bx-ax,dy=by-ay,den=dx*dx+dy*dy;
      double t=den>0?std::clamp(-(ax*dx+ay*dy)/den,0.0,1.0):0;
      if(std::hypot(ax+t*dx,ay+t*dy)<=0.10)return 0.15;
    }
  }
  return 0;
}

double Simulation::frontlinePressure(Point p) const {
  double pressure=0;
  if(!state.war)return 0;
  for(const auto& u:state.units)if(ready(u)&&occupying(u.type)){
    double d=distanceKm(p,u.position);
    if(d<30)pressure+=(u.owner==state.player?1.0:-1.0)*strength(u)*std::pow(1-d/30,2);
  }
  return std::clamp(pressure,-1.0,1.0);
}

double Simulation::siteMissileRange(size_t index) const {
  if(index>=state.sites.size()||state.sites[index].type!=Facility::Silo)return 0;
  double range=hasTech(60)?12000:1800;if(hasTech(65))range*=1.2;return range;
}
std::string Simulation::missileStrikeReason(size_t index,Point target,bool nuclear) const {
  if(state.player<0||state.victory||state.defeat)return "No active campaign";
  if(!state.war)return "Declare war before firing";
  if(index>=state.sites.size())return "Select a missile site";
  const auto& site=state.sites[index];
  if(site.type!=Facility::Silo||site.destroyed||!site.enabled||site.remaining>0||site.region<0||site.region>=int(state.regions.size())||state.regions[site.region].owner!=state.player)return "Requires an operational friendly missile site";
  if(!hasTech(58))return "Requires Guided missiles";
  if(nuclear&&(!hasTech(60)||state.stock[int(Resource::Warheads)]<1))return "Requires nuclear delivery research and a ready warhead";
  if(site.missileCooldown>0)return "Missile site reloading";
  if(!std::isfinite(target.x)||!std::isfinite(target.y)||std::abs(target.y)>90)return "Invalid target";
  int r=regionAt(target);if(r<0||state.regions[r].owner!=state.enemy)return "Target hostile controlled land";
  double distance=distanceKm(site.position,target);if(distance<25||distance>siteMissileRange(index))return "Target outside ballistic missile envelope";
  if(state.engagements.size()>=4096)return "Projectile capacity reached";
  return "";
}
bool Simulation::launchMissile(size_t index,Point target,bool nuclear){
  if(!missileStrikeReason(index,target,nuclear).empty())return false;
  auto& site=state.sites[index];Engagement shot;shot.origin=site.position;shot.target=target;shot.owner=state.player;shot.sourceSite=int(index);shot.weaponId=nuclear?"nuclear_ballistic":"ballistic";shot.targetMask=nuclear?7:1;shot.ballistic=true;shot.nuclear=nuclear;shot.areaStrike=true;shot.damage=nuclear?250:45;shot.impactRadiusKm=nuclear?65:8;shot.total=shot.remaining=std::max(8.0,distanceKm(site.position,target)/25.0);state.engagements.push_back(shot);site.missileCooldown=45;
  if(nuclear){--state.stock[int(Resource::Warheads)];state.nuclearUsed=true;}
  log(nuclear?"Nuclear ballistic missile launched":"Conventional ballistic missile launched");return true;
}
std::string Simulation::unitAttackReason(size_t index,Point target) const {
  if(state.player<0||state.victory||state.defeat||!state.war)return "Requires an active war";
  if(index>=state.units.size())return "Select a formation";
  const auto& u=state.units[index];if(u.owner!=state.player||!ready(u))return "Requires a deployed friendly formation";
  if(!std::isfinite(target.x)||!std::isfinite(target.y)||std::abs(target.y)>90)return "Invalid target";
  int r=regionAt(target);if(r<0||state.regions[r].owner!=state.enemy)return "Target hostile controlled land";
  double d=distanceKm(u.position,target);
  for(const auto& w:unitWeapons(u))if((w.id=="ground_missile"||w.id=="cruise"||w.id=="howitzer")&&d>=w.minRangeKm&&d<=w.maxRangeKm){int n=weaponSlot(u.type,w.id);if(n>=0&&(n>=int(u.weaponAmmo.size())||(u.weaponAmmo[n]>0&&u.weaponCooldowns[n]<=0&&u.weaponReloads[n]<=0)))return state.engagements.size()<4096?"":"Projectile capacity reached";return "Weapon reloading or ammunition exhausted";}
  return "No point-strike weapon in range";
}
bool Simulation::orderUnitAttackAt(size_t index,Point target){
  if(!unitAttackReason(index,target).empty())return false;
  auto& u=state.units[index];double d=distanceKm(u.position,target);
  for(const auto& w:unitWeapons(u))if((w.id=="ground_missile"||w.id=="cruise"||w.id=="howitzer")&&d>=w.minRangeKm&&d<=w.maxRangeKm){
    auto slots=baseWeapons(u.type);size_t old=u.weaponAmmo.size();u.weaponAmmo.resize(slots.size());u.weaponReserve.resize(slots.size());u.weaponCooldowns.resize(slots.size());u.weaponReloads.resize(slots.size());for(size_t n=old;n<slots.size();++n){u.weaponAmmo[n]=slots[n].magazine;u.weaponReserve[n]=slots[n].reserveAmmo;}
    int n=weaponSlot(u.type,w.id);--u.weaponAmmo[n];u.weaponCooldowns[n]=w.cooldownSeconds;
    Engagement shot;shot.origin=u.position;shot.target=target;shot.attacker=int(index);shot.owner=u.owner;shot.weaponId=w.id;shot.targetMask=1;shot.areaStrike=true;shot.ballistic=w.id=="ground_missile";shot.damage=w.damage*(.35+.65*u.health/100);shot.impactRadiusKm=w.id=="howitzer"?4:6;shot.total=shot.remaining=std::max(2.0,d/w.projectileSpeedKmPerSecond);state.engagements.push_back(shot);log(w.name+" point strike launched");return true;
  }
  return false;
}

void Simulation::combat(){
  for(auto& site:state.sites)site.missileCooldown=std::max(0.0,site.missileCooldown-simulationTickSeconds);
  if(!state.war&&state.engagements.empty())return;
  // Defenders react to actual incursions; orders never create damage remotely.
  for(auto& defender:state.units)if(state.war&&defender.owner==state.enemy&&ready(defender)&&occupying(defender.type)){
    Point destination=defender.position;
    double best=std::numeric_limits<double>::max();
    bool response=false,settlementObjective=false;
    const Unit* responseTarget=nullptr;
    int objectiveRegion=-1;
    for(const auto& attacker:state.units)if(attacker.owner==state.player&&ready(attacker)&&occupying(attacker.type)){
      int r=attacker.region;
      bool threat=r>=0&&r<int(state.regions.size())&&state.regions[r].nation==state.enemy;
      if(!threat)for(const auto& city:state.settlements)if(city.owner==state.enemy&&distanceKm(city.position,attacker.position)<30){threat=true;break;}
      double d=distanceKm(defender.position,attacker.position);
      if(threat&&d<best){best=d;destination=attacker.position;response=true;responseTarget=&attacker;}
    }
    if(!response)for(const auto& city:state.settlements)if(city.region>=0&&city.region<int(state.regions.size())&&state.regions[city.region].nation==state.enemy&&city.owner!=state.enemy){
      double d=distanceKm(defender.position,city.position);
      if(d<best){best=d;destination=city.position;response=true;settlementObjective=true;objectiveRegion=city.region;}
    }
    // Weapon standoff is useful against troops, but occupation requires a
    // formation inside the city's five-kilometre perimeter.
    double stoppingDistance=settlementObjective?4.5:(responseTarget?engagementRange(defender,*responseTarget):unitRangeKm(defender))*.85;
    if(response&&(best>stoppingDistance||(settlementObjective&&regionAt(defender.position)!=objectiveRegion))){
      int target=regionAt(destination);
      bool changed=defender.target!=target||distanceKm(defender.destination,destination)>5;
      if(changed||(!defender.moving&&state.day%15==0)){
        if(!planUnitMovement(defender,destination)){
          defender.moving=false;defender.path.clear();defender.waypoints.clear();
          // Retry failed routes periodically rather than running an expensive
          // coastal search every tick for an unreachable island intrusion.
          defender.target=target;defender.destination=destination;
        }
      }
    }else{defender.moving=false;defender.path.clear();defender.waypoints.clear();}
  }
  // Naval defenders protect home coast water only; they never pursue cities.
  for(auto& defender:state.units)if(state.war&&defender.owner==state.enemy&&ready(defender)&&navalUnit(defender.type)){
    const Unit* threat=nullptr;double best=std::numeric_limits<double>::max();
    for(const auto& attacker:state.units)if(attacker.owner==state.player&&ready(attacker)&&navalUnit(attacker.type)&&engagementRange(defender,attacker)>0){
      bool homeWater=false;for(const auto& region:state.regions)if(region.nation==state.enemy&&distanceKm(region.center,attacker.position)<180){homeWater=true;break;}
      double d=distanceKm(defender.position,attacker.position);if(homeWater&&d<best){threat=&attacker;best=d;}
    }
    if(threat&&best>engagementRange(defender,*threat)*.85){if(distanceKm(defender.destination,threat->position)>5||!defender.moving)planUnitMovement(defender,threat->position);}
    else {defender.moving=false;defender.path.clear();defender.waypoints.clear();}
  }
  std::vector<double> damage(state.units.size(),0);
  for(auto& shot:state.engagements){
    if(shot.impactSeconds>0){shot.impactSeconds=std::max(0.0,shot.impactSeconds-simulationTickSeconds);continue;}
    if(shot.remaining<=0)continue;
    if(shot.guided&&shot.defender>=0&&shot.defender<int(state.units.size())&&ready(state.units[shot.defender]))shot.target=state.units[shot.defender].position;
    shot.remaining-=simulationTickSeconds;
    if(shot.remaining<=0&&shot.areaStrike){
      for(size_t n=0;n<state.units.size();++n){const auto& u=state.units[n];if(ready(u)&&(shot.nuclear||u.owner!=shot.owner)&&(shot.targetMask&unsigned(targetDomain(u.type)))&&distanceKm(u.position,shot.target)<=shot.impactRadiusKm)damage[n]+=shot.damage*(1-unitProtection(u));}
      for(auto& site:state.sites)if(!site.destroyed&&site.region>=0&&site.region<int(state.regions.size())&&(shot.nuclear||state.regions[site.region].owner!=shot.owner)&&distanceKm(site.position,shot.target)<=shot.impactRadiusKm){site.destroyed=true;site.enabled=false;}
      if(shot.nuclear){state.stability=std::max(0.0,state.stability-25);log("Nuclear detonation: formations and infrastructure in the blast area destroyed");}else log("Conventional strike impacted");
    }
    if(shot.remaining<=0&&shot.defender>=0&&shot.defender<int(state.units.size())){
      const auto& target=state.units[shot.defender];
      if(ready(target)&&(shot.targetMask&unsigned(targetDomain(target.type)))&&distanceKm(target.position,shot.target)<=shot.impactRadiusKm+unitSpeedKmPerSecond(target)*simulationTickSeconds)damage[shot.defender]+=shot.damage*(1-unitProtection(target));
    }
    if(shot.remaining<=0&&shot.damage>0)shot.impactSeconds=shot.nuclear?3.0:1.5;
  }
  state.engagements.erase(std::remove_if(state.engagements.begin(),state.engagements.end(),[](const Engagement& shot){return shot.remaining<=0&&shot.impactSeconds<=0;}),state.engagements.end());
  // Each mount chooses one eligible target and consumes its own ammunition.
  // Every launch is selected before damage is applied, preserving mutual fire.
  for(size_t a=0;a<state.units.size();++a){
    auto& ua=state.units[a];if(!state.war||!ready(ua))continue;
    const auto weapons=unitWeapons(ua);auto slots=baseWeapons(ua.type);
    for(const auto& weapon:weapons)if(weaponSlot(ua.type,weapon.id)>=int(slots.size()))slots.push_back(weapon);
    size_t old=ua.weaponAmmo.size();
    ua.weaponAmmo.resize(slots.size());ua.weaponReserve.resize(slots.size());ua.weaponCooldowns.resize(slots.size());ua.weaponReloads.resize(slots.size());
    for(size_t n=old;n<slots.size();++n){ua.weaponAmmo[n]=slots[n].magazine;ua.weaponReserve[n]=slots[n].reserveAmmo;}
    for(const auto& weapon:weapons){
      const size_t n=size_t(weaponSlot(ua.type,weapon.id));
      ua.weaponCooldowns[n]=std::max(0.0,ua.weaponCooldowns[n]-simulationTickSeconds);
      if(ua.weaponReloads[n]>0){
        ua.weaponReloads[n]=std::max(0.0,ua.weaponReloads[n]-simulationTickSeconds);
        if(ua.weaponReloads[n]==0){int rounds=std::min(weapon.magazine,ua.weaponReserve[n]);ua.weaponAmmo[n]=rounds;ua.weaponReserve[n]-=rounds;}
      }
      if(ua.weaponAmmo[n]<=0){if(ua.weaponReloads[n]==0&&ua.weaponReserve[n]>0)ua.weaponReloads[n]=weapon.reloadSeconds;continue;}
      if(ua.weaponCooldowns[n]>0||ua.weaponReloads[n]>0)continue;
      int target=-1;double nearest=std::numeric_limits<double>::max();
      for(size_t b=0;b<state.units.size();++b){
        const auto& ub=state.units[b];if(a==b||!ready(ub)||ua.owner==ub.owner||!weaponTargets(weapon,ub.type))continue;
        double d=distanceKm(ua.position,ub.position);
        if(d>=weapon.minRangeKm&&d<=weapon.maxRangeKm&&d<nearest){nearest=d;target=int(b);}
      }
      if(target<0)continue;
      --ua.weaponAmmo[n];ua.weaponCooldowns[n]=weapon.cooldownSeconds;
      double hit=weapon.damage*(.35+.65*ua.health/100);
      if(weapon.projectileSpeedKmPerSecond<=0){
        damage[target]+=hit*(1-unitProtection(state.units[target]));
        Engagement trace;trace.origin=ua.position;trace.target=state.units[target].position;trace.attacker=int(a);trace.defender=target;trace.weaponId=weapon.id;trace.targetMask=weapon.targetMask;trace.total=trace.remaining=simulationTickSeconds;trace.damage=0;
        if(state.engagements.size()<4096)state.engagements.push_back(trace);
      }
      else {
        Engagement shot;shot.origin=ua.position;shot.target=state.units[target].position;shot.attacker=int(a);shot.defender=target;shot.weaponId=weapon.id;shot.targetMask=weapon.targetMask;shot.owner=ua.owner;shot.ballistic=weapon.id=="ground_missile";shot.damage=hit;shot.total=shot.remaining=std::max(simulationTickSeconds,nearest/weapon.projectileSpeedKmPerSecond);shot.impactRadiusKm=weapon.impactRadiusKm;shot.guided=weapon.id=="atgm"||weapon.id=="anti_ship"||weapon.id=="asw_torpedo"||weapon.id=="sub_torpedo"||weapon.id=="cruise"||weapon.id=="ground_missile";
        if(state.engagements.size()<4096)state.engagements.push_back(shot);
      }
    }
  }
  for(size_t i=0;i<state.units.size();++i){
    auto& u=state.units[i];double previous=u.health;u.health=std::max(0.0,u.health-damage[i]);
    if(previous>0&&u.health<=0)log(std::string(u.owner==state.player?"Formation lost: ":"Enemy formation destroyed: ")+unitNames[int(u.type)]);
    if(u.health<=0){u.moving=false;u.path.clear();u.waypoints.clear();u.pendingDestinations.clear();}
  }
  if(!state.war)return;
  for(size_t cityIndex=0;cityIndex<state.settlements.size();++cityIndex){
    auto& city=state.settlements[cityIndex];
    double player=0,enemy=0;
    for(const auto& u:state.units)if(ready(u)&&occupying(u.type)&&u.region==city.region&&regionAt(u.position)==city.region&&distanceKm(u.position,city.position)<=5){
      if(u.owner==state.player)player+=strength(u);else if(u.owner==state.enemy)enemy+=strength(u);
    }
    double attackers=city.owner==state.enemy?player:enemy,defenders=city.owner==state.enemy?enemy:player;
    // Opposing troops must be removed before occupation can finish. Defenders
    // recover an abandoned objective; a contested city preserves its progress.
    const double occupationSeconds=mainSettlement(city.region)==int(cityIndex)?150.0:90.0;
    if(attackers<=0||defenders<=0)city.capture=std::clamp(city.capture+(std::min(1.0,attackers)-std::min(1.0,defenders))*simulationTickSeconds/occupationSeconds,0.0,1.0);
    if(city.capture>=1){city.owner=city.owner==state.enemy?state.player:state.enemy;city.capture=0;log("Settlement secured: "+city.name);}
  }
  for(size_t r=0;r<state.regions.size();++r){
    auto& region=state.regions[r];int cities=0,player=0,enemy=0;double progress=0;
    for(const auto& city:state.settlements)if(city.region==int(r)){++cities;player+=city.owner==state.player;enemy+=city.owner==state.enemy;progress+=city.owner==state.player?1-city.capture:city.capture;}
    int newOwner=region.owner;
    if(cities){
      region.capture=progress/cities;
      int main=mainSettlement(int(r));
      if(player==cities&&main>=0&&state.settlements[main].owner==state.player)newOwner=state.player;
      else if(enemy==cities&&main>=0&&state.settlements[main].owner==state.enemy)newOwner=state.enemy;
    }
    if(newOwner!=region.owner){
      region.owner=newOwner;
      // Queued formations cannot emerge under their former owner at a seized base.
      for(auto& unit:state.units)if(unit.remaining>0&&unit.sourceSite>=0&&unit.sourceSite<int(state.sites.size())&&state.sites[unit.sourceSite].region==int(r)&&unit.owner!=newOwner){
        unit.health=0;unit.remaining=0;unit.moving=false;unit.path.clear();unit.waypoints.clear();unit.pendingDestinations.clear();
      }
      log(newOwner==state.player?"Region secured: every settlement is held":"Enemy recaptured a region");
    }
  }
  int enemyRegions=0,playerRegions=0,totalEnemy=0,totalPlayer=0;
  for(const auto& r:state.regions){if(r.nation==state.enemy){++totalEnemy;enemyRegions+=r.owner==state.player;}if(r.nation==state.player){++totalPlayer;playerRegions+=r.owner==state.enemy;}}
  if(totalEnemy&&enemyRegions==totalEnemy){state.victory=true;state.war=false;log("All rival settlements and regions secured: victory");}
  if(totalPlayer&&playerRegions==totalPlayer){state.defeat=true;state.war=false;log("All home regions lost");}
}
}
