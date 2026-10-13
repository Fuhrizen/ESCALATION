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
  static bool naval(UnitType t){
    return t==UnitType::Destroyer||t==UnitType::Submarine;
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

    site.destroyed=true;
    site.enabled=false;
    site.remaining=0;
    log("Construction cancelled; production redirected");
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
  std::string Simulation::unitReason(UnitType t,int site) const {
    if(state.player<0)return "Start a campaign";
    if(state.victory||state.defeat)return "Campaign has ended";
    if(state.units.size()>=1000)return "Campaign unit history capacity reached";
    int n=int(t);
    if(n<0||n>=9)return "Unknown unit";
    if(site<0||site>=int(state.sites.size()))return "Select a production building";
    auto& s=state.sites[site];
    if(s.region<0||s.region>=int(state.regions.size())||s.destroyed||!s.enabled||s.remaining>0||state.regions[s.region].owner!=state.player)return "Production building is not operational";
    Facility needed=naval(t)?Facility::Shipyard:(t==UnitType::Fighter||t==UnitType::Bomber)?Facility::Airbase:t==UnitType::MissileLauncher?Facility::Silo:Facility::ArmyBase;
    if(s.type!=needed&&!(s.type==Facility::Factory&&ground(t)))return std::string("Requires ")+facilityNames[int(needed)];
    static int fields[]={
      0,0,0,1,1,2,2,3,4
    },levels[]={
      0,1,2,1,3,1,2,1,3
    };
    if(state.levels[fields[n]]<levels[n])return "Research more "+std::string(fieldNames[fields[n]])+" technology";

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

    state.units.push_back(u);
    log("Production queued: "+std::string(unitNames[int(t)]));
    return true;
  }
  void Simulation::holdUnit(size_t i){
    if(i<state.units.size()&&state.units[i].owner==state.player){
      auto& u=state.units[i];
      u.path.clear();
      u.waypoints.clear();
      u.moving=false;
      u.target=-1;
    }
  }
  bool Simulation::cancelUnit(size_t i){
    if(i>=state.units.size())return false;
    auto& u=state.units[i];
    if(u.owner!=state.player||u.remaining<=0||u.health<=0)return false;

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
    if(!planUnitMovement(u,p))return false;
    if(u.target>=0&&state.regions[u.target].owner==state.enemy)state.target=u.target;
    return true;
  }
  bool Simulation::planUnitMovement(Unit& u,Point p){
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||p.y< -90||p.y>90)return false;
    p.x=std::remainder(p.x,360.0);
    int destination=regionAt(p);
    int rival=u.owner==state.player?state.enemy:state.player;
    auto accessible=[&](int region){return region>=0&&region<int(state.regions.size())&&
      (state.regions[region].owner==u.owner||(state.war&&state.regions[region].owner==rival));};
    if(ground(u.type)&&!accessible(destination))return false;
    if(naval(u.type)&&destination>=0)return false;
    if(destination>=0&&!accessible(destination))return false;
    bool transport=false;
    // A disconnected theatre link is never itself permission to sail ground forces.
    if(ground(u.type)&&state.levels[2]>=1)for(const auto& site:state.sites)
      if(site.region>=0&&site.region<int(state.regions.size())&&site.type==Facility::Shipyard&&
         !site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==u.owner)transport=true;
    auto traversable=[&](Point q){
      if(!ground(u.type)&&!naval(u.type))return true;
      int nation=-1;
      for(int candidate:{state.player,state.enemy})if(world_->countries[candidate].contains(q)){nation=candidate;break;}
      if(nation<0){
        // Distinguish actual ocean from a neutral country before allowing embarkation.
        return (naval(u.type)||transport)&&world_->pick(q)<0;
      }
      if(naval(u.type))return false;
      int nearest=-1;double best=1e30;
      for(size_t r=0;r<state.regions.size();++r)if(state.regions[r].nation==nation){
        Point center=state.regions[r].center;double dx=std::remainder(q.x-center.x,360.0),dy=q.y-center.y;
        double d=dx*dx+dy*dy;if(d<best){best=d;nearest=int(r);}
      }
      return accessible(nearest);
    };
    auto clearSegment=[&](Point a,Point b){
      double dx=std::remainder(b.x-a.x,360.0),dy=b.y-a.y;
      int samples=std::max(1,int(std::ceil(std::hypot(dx,dy)/0.08)));
      for(int n=1;n<=samples;++n){double t=double(n)/samples;
        if(!traversable({std::remainder(a.x+dx*t,360.0),a.y+dy*t}))return false;
      }
      return true;
    };
    std::vector<Point> route;
    if(clearSegment(u.position,p))route.push_back(p);
    else{
      // Visibility graph of safe regional centres and coastal bends. Edges are
      // sampled against actual land, so region adjacency cannot cut across a bay.
      std::vector<Point> nodes={u.position,p};
      if(ground(u.type))for(size_t r=0;r<state.regions.size();++r)if(accessible(int(r)))nodes.push_back(state.regions[r].center);
      for(int nation:{state.player,state.enemy})for(const auto& polygon:world_->countries[nation].polygons){
        if(polygon.rings.empty())continue;
        const auto& ring=polygon.rings.front();
        for(size_t k=0;k<ring.size()&&nodes.size()<120;++k){
          Point v=ring[k];
          if(distance(v,u.position)+distance(v,p)>distance(u.position,p)+6)continue;
          // Sample both sides of the coast; only traversable candidates survive.
          for(int angle=0;angle<8&&nodes.size()<120;++angle){
            Point q{v.x+0.12*std::cos(angle*0.7853981634),v.y+0.12*std::sin(angle*0.7853981634)};
            if(traversable(q)){nodes.push_back(q);break;}
          }
        }
      }
      std::vector<double> costs(nodes.size(),1e30);std::vector<int> previous(nodes.size(),-1);
      std::vector<bool> visited(nodes.size());costs[0]=0;
      for(size_t step=0;step<nodes.size();++step){
        int best=-1;for(size_t n=0;n<nodes.size();++n)if(!visited[n]&&(best<0||costs[n]<costs[best]))best=int(n);
        if(best<0||costs[best]>=1e30)break;
        if(best==1)break;
        visited[best]=true;
        for(size_t n=1;n<nodes.size();++n)if(!visited[n]){
          double candidate=costs[best]+distance(nodes[best],nodes[n]);
          if(candidate<costs[n]&&clearSegment(nodes[best],nodes[n])){costs[n]=candidate;previous[n]=best;}
        }
      }
      if(previous[1]<0)return false;
      for(int n=1;n!=0;n=previous[n])route.push_back(nodes[n]);
      std::reverse(route.begin(),route.end());
    }
    u.path.clear();
    for(Point waypoint:route){int r=regionAt(waypoint);if(r>=0&&std::find(u.path.begin(),u.path.end(),r)==u.path.end())u.path.push_back(r);}
    u.waypoints=std::move(route);u.destination=p;u.target=destination;u.moving=true;
    return true;
  }
  void Simulation::unitsTick(){
    for(auto& u:state.units){
      if(u.health<=0)continue;
      if(u.remaining>0)continue; // Production is allocated by economy(), per producer.
      if(!u.moving)continue;
      if(u.waypoints.empty()){
        // Older snapshots have region-only orders; validate a fresh geometric route.
        if(!planUnitMovement(u,u.destination)){u.moving=false;u.path.clear();continue;}
      }
      Point destination=u.waypoints.front();
      double scale=std::max(0.20,std::cos((destination.y+u.position.y)*0.00872664626));
      double dx=std::remainder(destination.x-u.position.x,360.0),dy=destination.y-u.position.y;
      double length=std::hypot(dx*scale,dy);
      double speed=ground(u.type)?0.20:naval(u.type)?0.42:1.6;
      if(u.owner==state.player)speed*=1+0.03*state.levels[7];
      if(ground(u.type))speed*=1+roadMovementBonus(u.position);
      if(length<=speed){
        u.position=destination;u.waypoints.erase(u.waypoints.begin());
        int reached=regionAt(destination);
        if(!u.path.empty()&&reached==u.path.front())u.path.erase(u.path.begin());
        if(u.waypoints.empty()){u.moving=false;u.path.clear();if(u.target>=0)u.region=u.target;}
      }
      else{
        u.position.x=std::remainder(u.position.x+dx/length*speed,360.0);
        u.position.y+=dy/length*speed;
      }
      int actual=regionAt(u.position);
      if(actual>=0)u.region=actual;
    }
    if(state.simpleEconomy){
      state.brigades=0;
      for(auto& u:state.units)if(u.owner==state.player&&ground(u.type)&&u.health>0&&u.remaining<=0)state.brigades+=u.health/100;
    }
  }
}
