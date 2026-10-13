#include "escalation/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
namespace escalation {
  const std::array<const char*,9> unitNames={
    "Infantry brigade","Armoured brigade","Artillery battery","Fighter squadron","Bomber squadron","Destroyer","Submarine","Special forces","Missile battery"
  };
  static bool ground(UnitType t){
    return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces;
  }
  static bool naval(UnitType t){
    return t==UnitType::Destroyer||t==UnitType::Submarine;
  }
  double Simulation::buildingIncome(Facility t) const {
    static const double yields[]={
      6,10,14,12,0,12,0,0,0,0,0,0,0,8,25,18
    };
    return int(t)>=0&&int(t)<16?yields[int(t)]:0;
  }
  double Simulation::regionIncome(int r) const {
    if(r<0||r>=int(state.regions.size()))return 0;
    double total=0;
    for(auto& s:state.sites)if(s.region==r&&!s.destroyed&&s.enabled&&s.remaining<=0)total+=buildingIncome(s.type)*(1+0.15*state.regions[r].roads)*(1+0.1*state.levels[5]);
    return total;
  }
  int Simulation::regionAt(Point p) const {
    int country=world_->pick(p);
    if(country<0)return -1;
    int best=-1;
    double score=1e30;
    for(size_t i=0;i<state.regions.size();++i)if(state.regions[i].nation==country){
      auto q=state.regions[i].center;
      double dx=std::remainder(p.x-q.x,360.0),dy=p.y-q.y,d=dx*dx+dy*dy;
      if(d<score){
        score=d;
        best=int(i);
      }
    }
    return best;
  }
  bool Simulation::buildAt(int r,Facility type,Point p){
    if(regionAt(p)!=r||!buildReason(r,type).empty())return false;
    if(type==Facility::Shipyard){
      bool coast=false;
      for(int n=0;n<16;++n){
        double angle=n*0.3926990817;
        if(world_->pick({
          p.x+std::cos(angle)*0.6,p.y+std::sin(angle)*0.6
        })<0)coast=true;
      }
      if(!coast)return false;
    }
    if(!build(r,type))return false;
    state.sites.back().position=p;
    return true;
  }
  bool Simulation::cancelSite(size_t i){
    if(i>=state.sites.size())return false;
    auto& site=state.sites[i];
    if(site.destroyed||site.remaining<=0||state.regions[site.region].owner!=state.player)return false;
    state.money+=buildCost(site.type)*0.5;
    site.destroyed=true;
    site.enabled=false;
    site.remaining=0;
    log("Construction cancelled; half the treasury cost returned");
    return true;
  }
  double Simulation::roadCost(int r) const {
    return r>=0&&r<int(state.regions.size())?500.0*(state.regions[r].roads+1)*(state.regions[r].roads+1):1e30;
  }
  bool Simulation::upgradeRoad(int r){
    if(r<0||r>=int(state.regions.size())||state.regions[r].owner!=state.player||state.regions[r].roads>=5||state.money<roadCost(r))return false;
    state.money-=roadCost(r);
    ++state.regions[r].roads;
    log("Regional roads upgraded: income and movement improved");
    return true;
  }
  double Simulation::unitCost(UnitType t) const {
    static double values[]={
      750,1400,1100,1800,3200,4200,5000,1600,6500
    };
    return int(t)>=0&&int(t)<9?values[int(t)]:1e30;
  }
  double Simulation::unitDays(UnitType t) const {
    static double values[]={
      12,20,18,24,35,45,50,25,45
    };
    return int(t)>=0&&int(t)<9?values[int(t)]:1e30;
  }
  std::string Simulation::unitReason(UnitType t,int site) const {
    int n=int(t);
    if(n<0||n>=9)return "Unknown unit";
    if(site<0||site>=int(state.sites.size()))return "Select a production building";
    auto& s=state.sites[site];
    if(s.destroyed||!s.enabled||s.remaining>0||state.regions[s.region].owner!=state.player)return "Production building is not operational";
    Facility needed=naval(t)?Facility::Shipyard:(t==UnitType::Fighter||t==UnitType::Bomber)?Facility::Airbase:t==UnitType::MissileLauncher?Facility::Silo:Facility::ArmyBase;
    if(s.type!=needed&&!(s.type==Facility::Factory&&ground(t)))return std::string("Requires ")+facilityNames[int(needed)];
    static int fields[]={
      0,0,0,1,1,2,2,3,4
    },levels[]={
      0,1,2,1,3,1,2,1,3
    };
    if(state.levels[fields[n]]<levels[n])return "Research more "+std::string(fieldNames[fields[n]])+" technology";
    if(state.money<unitCost(t))return "Insufficient treasury";
    int alive=0;
    for(auto& u:state.units)if(u.owner==state.player&&u.health>0)++alive;
    if(alive>=120)return "120-unit command capacity reached";
    return "";
  }
  bool Simulation::queueUnit(UnitType t,int site){
    if(!unitReason(t,site).empty())return false;
    auto& source=state.sites[site];
    Unit u;
    u.owner=state.player;
    u.region=source.region;
    u.type=t;
    u.sourceSite=site;
    u.position=source.position;
    u.remaining=u.total=unitDays(t);
    u.destination=u.position;
    if(naval(t)){
      bool found=false;
      for(double radius=0.1;radius<=2&&!found;radius+=0.1)for(int n=0;n<24&&!found;++n){
        double a=n*0.2617993878;
        Point p{
          source.position.x+radius*std::cos(a),source.position.y+radius*std::sin(a)
        };
        if(world_->pick(p)<0){
          u.position=p;
          found=true;
        }
      }
      if(!found)return false;
    }
    state.money-=unitCost(t);
    state.units.push_back(u);
    log("Production queued: "+std::string(unitNames[int(t)]));
    return true;
  }
  void Simulation::holdUnit(size_t i){
    if(i<state.units.size()&&state.units[i].owner==state.player){
      auto& u=state.units[i];
      u.path.clear();
      u.moving=false;
      u.target=-1;
    }
  }
  bool Simulation::cancelUnit(size_t i){
    if(i>=state.units.size())return false;
    auto& u=state.units[i];
    if(u.owner!=state.player||u.remaining<=0||u.health<=0)return false;
    state.money+=unitCost(u.type)*0.5;
    u.health=0;
    u.remaining=0;
    return true;
  }
  bool Simulation::orderUnit(size_t i,int r){
    if(r<0||r>=int(state.regions.size()))return false;
    return orderUnitAt(i,state.regions[r].center);
  }
  bool Simulation::orderUnitAt(size_t i,Point p){
    if(i>=state.units.size())return false;
    auto& u=state.units[i];
    if(u.owner!=state.player||u.health<=0||u.remaining>0)return false;
    int destination=regionAt(p);
    if(ground(u.type)&&destination<0)return false;
    if(destination>=0&&state.regions[destination].owner!=state.player&&!state.war)return false;
    std::vector<int> path;
    if(ground(u.type)){
      if(u.region<0||u.region>=int(state.regions.size()))return false;
      if(world_->countries[state.player].neighbours.empty()&&state.regions[destination].nation!=state.player){
        bool transport=false;
        for(auto& s:state.sites)if(s.type==Facility::Shipyard&&!s.destroyed&&s.remaining<=0&&s.enabled&&state.regions[s.region].owner==state.player)transport=true;
        if(state.levels[2]<1||!transport)return false;
      }
      std::vector<int> previous(state.regions.size(),-1);
      std::queue<int> open;
      open.push(u.region);
      previous[u.region]=u.region;
      while(!open.empty()){
        int r=open.front();
        open.pop();
        if(r==destination)break;
        for(int next:state.regions[r].neighbours)if(previous[next]<0&&(state.regions[next].owner==state.player||next==destination)){
          previous[next]=r;
          open.push(next);
        }
      }
      if(previous[destination]<0){
        bool transport=false;
        for(auto& site:state.sites)if(site.type==Facility::Shipyard&&!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player)transport=true;
        if(state.levels[2]<1||!transport)return false;
        path.push_back(destination);
      }
      else{
        for(int r=destination;r!=u.region;r=previous[r])path.push_back(r);
        std::reverse(path.begin(),path.end());
      }
    }
    u.path=std::move(path);
    u.destination=p;
    u.target=destination;
    u.moving=true;
    if(destination>=0&&state.regions[destination].owner==state.enemy)state.target=destination;
    return true;
  }
  void Simulation::unitsTick(){
    for(auto& u:state.units){
      if(u.health<=0)continue;
      if(u.remaining>0){
        if(u.sourceSite>=0&&u.sourceSite<int(state.sites.size())){
          auto& site=state.sites[u.sourceSite];
          if(site.enabled&&!site.destroyed&&site.remaining<=0&&state.regions[site.region].owner==u.owner){
            u.remaining=std::max(0.0,u.remaining-1);
            if(u.remaining==0)log("Unit deployed: "+std::string(unitNames[int(u.type)]));
          }
        }
        continue;
      }
      if(!u.moving)continue;
      Point destination=u.path.size()>1?state.regions[u.path.front()].center:u.destination;
      double dx=std::remainder(destination.x-u.position.x,360.0),dy=destination.y-u.position.y,length=std::hypot(dx,dy);
      double speed=ground(u.type)?0.20:naval(u.type)?0.42:1.6;
      int roads=u.region>=0&&u.region<int(state.regions.size())?state.regions[u.region].roads:0;
      speed*=(1+roads*0.2)*(1+0.08*state.levels[7]);
      if(length<=speed){
        u.position=destination;
        if(!u.path.empty()){
          u.region=u.path.front();
          u.path.erase(u.path.begin());
        }
        if(u.path.empty()){
          u.moving=false;
          if(u.target>=0)u.region=u.target;
        }
      }
      else{
        u.position.x=std::remainder(u.position.x+dx/length*speed,360.0);
        u.position.y+=dy/length*speed;
      }
    }
    if(state.simpleEconomy){
      state.brigades=0;
      for(auto& u:state.units)if(u.owner==state.player&&ground(u.type)&&u.health>0&&u.remaining<=0)state.brigades+=u.health/100;
    }
  }
}
