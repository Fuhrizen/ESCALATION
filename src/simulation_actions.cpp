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
    return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces||t==UnitType::MissileLauncher;
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
  bool Simulation::buildAt(int r,Facility type,Point p){return placeBuilding(type,r,p);}
  std::string Simulation::placementReason(Facility type,int r,Point p) const {
    if(int(type)<0||int(type)>=16)return "Unknown building";
    if(state.buildingInventory[int(type)]<=0)return "Manufacture this building first";
    auto reason=buildReason(r,type);if(!reason.empty())return reason;
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||regionAt(p)!=r)return "Place on controlled land";
    if(type==Facility::Shipyard){
      bool coast=false;
      for(int angle=0;angle<32;++angle){double a=angle*6.28318530718/32;Point q{p.x+0.04*std::cos(a)/std::max(.2,std::cos(p.y*.01745329252)),p.y+0.04*std::sin(a)};if(world_->pick(q)<0){coast=true;break;}}
      if(!coast)return "Shipyards require controlled coast within 5 km of ocean";
    }
    return "";
  }
  bool Simulation::placeBuilding(Facility type,int r,Point p){
    if(!placementReason(type,r,p).empty())return false;
    --state.buildingInventory[int(type)];
    state.sites.push_back({r,type,0,1,true,p,false});
    log(std::string("Placed: ")+facilityNames[int(type)]);
    refreshRates();
    return true;
  }
  bool Simulation::cancelSite(size_t i){
    if(i>=state.sites.size())return false;
    auto& site=state.sites[i];
    if(site.destroyed||site.remaining<=0||state.regions[site.region].owner!=state.player)return false;

    site.destroyed=true;
    site.enabled=false;
    site.remaining=0;
    log("Construction cancelled; production redirected");
    refreshRates();
    return true;
  }
  double Simulation::roadCost(int) const { return 0; }
  bool Simulation::upgradeRoad(int){ return false; }
  double Simulation::unitCost(UnitType t) const {
    static double values[]={
      30,60,45,65,100,110,130,55,160
    };
    return int(t)>=0&&int(t)<9?values[int(t)]:1e30;
  }
  double Simulation::unitDays(UnitType t) const {
    static double values[]={
      12,20,18,24,35,45,50,25,45
    };
    return int(t)>=0&&int(t)<9?values[int(t)]:1e30;
  }
  double Simulation::unitBuildETA(UnitType type,int site) const {
    if(!unitReason(type,site).empty())return std::numeric_limits<double>::infinity();
    double rate=prospectiveProductionRate();
    if(rate<=0)return std::numeric_limits<double>::infinity();
    double duration=unitCost(type),pending=duration;
    for(const auto& source:state.sites)if(productionSiteActive(source))pending+=std::min(source.remaining,duration);
    for(const auto& unit:state.units)if(productionUnitActive(unit))pending+=std::min(unit.remaining,duration);
    double buildings=0;for(const auto& job:state.buildingQueue)buildings+=job.total-job.progress;pending+=std::min(buildings,duration);
    if(strategicProductionActive())pending+=std::min(state.warheadWork+1200*(state.warheadsQueued-1),duration);
    return pending/(rate*0.1);
  }
  std::string Simulation::unitReason(UnitType t,int site) const {
    if(state.player<0)return "Start a campaign";
    if(state.victory||state.defeat)return "Campaign has ended";
    if(state.units.size()>=1000)return "Campaign unit history capacity reached";
    int n=int(t);
    if(n<0||n>=9)return "Unknown unit";
    if(!activeUnit(t))return "Retired air unit";
    if(site<0||site>=int(state.sites.size()))return "Select a production building";
    auto& s=state.sites[site];
    if(s.region<0||s.region>=int(state.regions.size())||s.destroyed||!s.enabled||s.remaining>0||state.regions[s.region].owner!=state.player)return "Production building is not operational";
    Facility needed=navalUnit(t)?Facility::Shipyard:t==UnitType::MissileLauncher?Facility::Silo:Facility::ArmyBase;
    if(s.type!=needed&&!(s.type==Facility::Factory&&ground(t)))return std::string("Requires ")+facilityNames[int(needed)];
    int prerequisite=t==UnitType::Destroyer?113:t==UnitType::Submarine?114:t==UnitType::Armour?7:t==UnitType::Artillery?9:t==UnitType::SpecialForces?46:t==UnitType::MissileLauncher?62:-1;
    if(prerequisite>=0&&!hasTech(prerequisite))return "Requires "+techs[prerequisite].name;

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
    u.remaining=u.total=unitCost(t);
    u.destination=u.position;
    u.deployed=false;

    state.units.push_back(u);
    log("Production queued: "+std::string(unitNames[int(t)]));
    refreshRates();
    return true;
  }
  bool Simulation::cancelUnit(size_t i){
    if(i>=state.units.size())return false;
    auto& u=state.units[i];
    if(u.owner!=state.player||u.remaining<=0||u.health<=0)return false;

    u.health=0;
    u.remaining=0;
    refreshRates();
    return true;
  }
}
