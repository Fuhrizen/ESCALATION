#include "escalation/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace escalation {
namespace {
bool occupying(UnitType t) {
  return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces;
}
bool groundTarget(UnitType t) { return occupying(t)||t==UnitType::MissileLauncher; }
bool naval(UnitType t) { return t==UnitType::Destroyer||t==UnitType::Submarine; }
bool ready(const Unit& u) { return u.health>0&&u.remaining<=0; }
double localDistance(Point a,Point b) {
  double x=std::remainder(a.x-b.x,360.0)*std::max(0.20,std::cos((a.y+b.y)*0.00872664626));
  return std::hypot(x,a.y-b.y);
}
double strength(const Unit& u) {
  return (u.type==UnitType::Armour?1.8:u.type==UnitType::Artillery?1.2:1.0)*u.health/100;
}
int technologyField(UnitType t) {
  return naval(t)?2:t==UnitType::Fighter||t==UnitType::Bomber?1:t==UnitType::SpecialForces?3:t==UnitType::MissileLauncher?4:0;
}
double range(UnitType t) {
  return t==UnitType::Artillery?1.25:t==UnitType::Bomber?1.6:t==UnitType::MissileLauncher?2.2:t==UnitType::Fighter?1.2:0.65;
}
bool canFight(UnitType a,UnitType b) {
  if(naval(a))return naval(b)||b==UnitType::Bomber;
  if(a==UnitType::Fighter)return b==UnitType::Fighter||b==UnitType::Bomber;
  if(a==UnitType::Bomber)return groundTarget(b)||naval(b);
  return groundTarget(b);
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
    double d=localDistance(p,u.position);
    if(d<2.5)pressure+=(u.owner==state.player?1.0:-1.0)*strength(u)*std::pow(1-d/2.5,2);
  }
  return std::clamp(pressure,-1.0,1.0);
}

void Simulation::combat(){
  if(!state.war)return;
  // Defenders react to actual incursions; orders never create damage remotely.
  for(auto& defender:state.units)if(defender.owner==state.enemy&&ready(defender)&&occupying(defender.type)){
    Point destination=defender.position;
    double best=std::numeric_limits<double>::max();
    bool response=false;
    for(const auto& attacker:state.units)if(attacker.owner==state.player&&ready(attacker)&&occupying(attacker.type)){
      int r=attacker.region;
      bool threat=r>=0&&r<int(state.regions.size())&&state.regions[r].nation==state.enemy;
      if(!threat)for(const auto& city:state.settlements)if(city.owner==state.enemy&&localDistance(city.position,attacker.position)<1.5){threat=true;break;}
      double d=localDistance(defender.position,attacker.position);
      if(threat&&d<best){best=d;destination=attacker.position;response=true;}
    }
    if(!response)for(const auto& city:state.settlements)if(city.region>=0&&city.region<int(state.regions.size())&&state.regions[city.region].nation==state.enemy&&city.owner!=state.enemy){
      double d=localDistance(defender.position,city.position);
      if(d<best){best=d;destination=city.position;response=true;}
    }
    if(response&&best>0.35){
      int target=regionAt(destination);
      bool changed=defender.target!=target||localDistance(defender.destination,destination)>0.75;
      if(changed||(!defender.moving&&state.day%15==0)){
        if(!planUnitMovement(defender,destination)){
          defender.moving=false;defender.path.clear();defender.waypoints.clear();
          // Retry failed routes periodically rather than running an expensive
          // coastal search every tick for an unreachable island intrusion.
          defender.target=target;defender.destination=destination;
        }
      }
    }else if(response){defender.moving=false;defender.path.clear();defender.waypoints.clear();}
  }
  std::vector<double> damage(state.units.size(),0);
  for(size_t a=0;a<state.units.size();++a){
    const auto& ua=state.units[a];if(!ready(ua))continue;
    for(size_t b=a+1;b<state.units.size();++b){
      const auto& ub=state.units[b];if(!ready(ub)||ua.owner==ub.owner)continue;
      double d=localDistance(ua.position,ub.position);
      if(canFight(ua.type,ub.type)&&d<=range(ua.type))damage[b]+=0.85*strength(ua)*(ua.owner==state.player?1+0.12*state.levels[technologyField(ua.type)]:1.15);
      if(canFight(ub.type,ua.type)&&d<=range(ub.type))damage[a]+=0.85*strength(ub)*(ub.owner==state.player?1+0.12*state.levels[technologyField(ub.type)]:1.15);
    }
  }
  for(size_t i=0;i<state.units.size();++i){
    auto& u=state.units[i];u.health=std::max(0.0,u.health-damage[i]);
    if(u.health<=0){u.moving=false;u.path.clear();u.waypoints.clear();}
  }
  for(auto& city:state.settlements){
    double player=0,enemy=0;
    for(const auto& u:state.units)if(ready(u)&&occupying(u.type)&&localDistance(u.position,city.position)<=0.55){
      if(u.owner==state.player)player+=strength(u);else if(u.owner==state.enemy)enemy+=strength(u);
    }
    double attackers=city.owner==state.enemy?player:enemy,defenders=city.owner==state.enemy?enemy:player;
    // Opposing troops must be removed before occupation can finish. Defenders
    // recover an abandoned objective; a contested city preserves its progress.
    if(attackers<=0||defenders<=0)city.capture=std::clamp(city.capture+(attackers-defenders)*0.045,0.0,1.0);
    if(city.capture>=1){city.owner=city.owner==state.enemy?state.player:state.enemy;city.capture=0;log("Settlement secured: "+city.name);}
  }
  for(size_t r=0;r<state.regions.size();++r){
    auto& region=state.regions[r];int cities=0,player=0,enemy=0;double progress=0;
    for(const auto& city:state.settlements)if(city.region==int(r)){++cities;player+=city.owner==state.player;enemy+=city.owner==state.enemy;progress+=city.owner==state.player?1-city.capture:city.capture;}
    int newOwner=region.owner;
    if(cities){
      region.capture=progress/cities;
      if(player==cities)newOwner=state.player;else if(enemy==cities)newOwner=state.enemy;
    }
    if(newOwner!=region.owner){
      region.owner=newOwner;
      // Queued formations cannot emerge under their former owner at a seized base.
      for(auto& unit:state.units)if(unit.remaining>0&&unit.sourceSite>=0&&unit.sourceSite<int(state.sites.size())&&state.sites[unit.sourceSite].region==int(r)&&unit.owner!=newOwner){
        unit.health=0;unit.remaining=0;unit.moving=false;unit.path.clear();unit.waypoints.clear();
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
