#include "escalation/simulation.hpp"
#include <cmath>
#include <algorithm>
namespace escalation {
int Simulation::territoryKey(Point p){
  if(!std::isfinite(p.x)||!std::isfinite(p.y)||p.y<-90||p.y>90)return -1;
  double longitude=std::fmod(p.x+180,360);if(longitude<0)longitude+=360;
  int x=std::min(3599,int(longitude/territoryCellDegrees));
  int y=std::clamp(int((p.y+90)/territoryCellDegrees),0,1799);
  return y*3600+x;
}
Point Simulation::territoryCenter(int key){return {-180+(key%3600+.5)*territoryCellDegrees,-90+(key/3600+.5)*territoryCellDegrees};}
int Simulation::territoryOwner(Point p) const {
  int key=territoryKey(p);if(key<0)return -1;
  auto cell=state.territory.find(key);if(cell!=state.territory.end())return cell->second.region<0?-1:cell->second.occupied?cell->second.owner:state.regions[cell->second.region].owner;
  int region=regionAt(p);return region<0?-1:state.regions[region].owner;
}
void Simulation::updateTerritory(){
  if(!state.war)return;
  std::size_t fingerprint=0;
  auto hash=[&](double value){fingerprint^=std::hash<double>{}(value)+0x9e3779b9+(fingerprint<<6)+(fingerprint>>2);};
  hash(state.territory.size());hash(state.territoryRevision);hash(state.player);hash(state.enemy);
  for(const auto& region:state.regions)hash(region.owner);
  for(const auto& u:state.units){hash(u.owner);hash(int(u.type));hash(u.position.x);hash(u.position.y);hash(u.health);hash(u.remaining);hash(u.deployed);}
  if(fingerprint==territoryFingerprint_)return;
  territoryFingerprint_=fingerprint;
  // Sparse local candidates: cost depends on deployed troops, never viewport area.
  // Cached geographic membership survives troop departures, pause and save/load.
  std::unordered_map<int,double> influence;
  for(const auto& u:state.units){
    if(!u.deployed||u.health<=0||u.remaining>0||!(u.type==UnitType::Infantry||u.type==UnitType::Armour||u.type==UnitType::Artillery||u.type==UnitType::SpecialForces))continue;
    double latitudeRadius=30/111.2,longitudeRadius=latitudeRadius/std::max(.2,std::cos(u.position.y*.01745329252));
    int origin=territoryKey(u.position),ox=origin%3600,oy=origin/3600;
    int rx=int(std::ceil(longitudeRadius/territoryCellDegrees))+1,ry=4;
    double power=(u.type==UnitType::Armour?1.8:u.type==UnitType::Artillery?1.2:1.0)*u.health/100;
    for(int dy=-ry;dy<=ry;++dy)for(int dx=-rx;dx<=rx;++dx){
      int y=oy+dy;if(y<0||y>=1800)continue;int x=(ox+dx+3600)%3600,key=y*3600+x;
      Point p=territoryCenter(key);double d=distanceKm(p,u.position);if(d>=30)continue;
      auto found=state.territory.find(key);
      if(found==state.territory.end()){
        int region=regionAt(p);state.territory.emplace(key,TerritoryCell{region,region<0?-1:state.regions[region].owner});found=state.territory.find(key);
      }
      if(found->second.region<0)continue;
      influence[key]+=(u.owner==state.player?1:-1)*power*(1-d/30)*(1-d/30);
    }
  }
  for(const auto& v:influence){
    // Near ties are contested. An abandoned cell has no influence entry and stays held.
    if(std::abs(v.second)<.05)continue;
    auto& cell=state.territory.at(v.first);int owner=v.second>0?state.player:state.enemy;
    int prior=cell.occupied?cell.owner:state.regions[cell.region].owner;
    if(prior!=owner){cell.owner=owner;cell.occupied=true;++state.territoryRevision;}
  }
}
}
