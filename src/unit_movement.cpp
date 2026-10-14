#include "escalation/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
namespace escalation {
  static bool ground(UnitType t){return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces||t==UnitType::MissileLauncher;}
  static bool naval(UnitType t){return t==UnitType::Destroyer||t==UnitType::Submarine;}
  void Simulation::holdUnit(size_t i){
    if(i<state.units.size()&&state.units[i].owner==state.player){
      auto& u=state.units[i];
      u.path.clear();
      u.waypoints.clear();
      u.pendingDestinations.clear();
      u.destination=u.position;
      u.moving=false;
      u.target=-1;
    }
  }
  bool Simulation::orderUnit(size_t i,int r,bool append){
    if(r<0||r>=int(state.regions.size()))return false;
    return orderUnitAt(i,state.regions[r].center,append);
  }
  bool Simulation::orderUnitAt(size_t i,Point p,bool append){
    if(i>=state.units.size())return false;
    auto& u=state.units[i];
    if(u.owner!=state.player||u.health<=0||u.remaining>0)return false;
    // Validate a queued leg from its eventual start without changing the live route.
    Unit planned=u;
    if(append&&u.moving){
      if(u.pendingDestinations.size()>=64)return false;
      planned.position=u.pendingDestinations.empty()?u.destination:u.pendingDestinations.back();
      if(!planUnitMovement(planned,p))return false;
      u.pendingDestinations.push_back(planned.destination);
      return true;
    }
    if(!planUnitMovement(planned,p))return false;
    planned.pendingDestinations.clear();
    u=std::move(planned);
    if(u.target>=0&&state.regions[u.target].owner==state.enemy)state.target=u.target;
    return true;
  }
  bool Simulation::planUnitMovement(Unit& u,Point p) const{
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
  std::vector<Point> Simulation::unitPlannedPath(size_t i) const{
    if(i>=state.units.size())return {};
    Unit unit=state.units[i];
    if(!unit.moving||unit.health<=0||unit.remaining>0)return {};
    if(unit.waypoints.empty()&&!planUnitMovement(unit,unit.destination))return {};
    std::vector<Point> path=unit.waypoints;
    unit.position=unit.destination;
    for(Point destination:state.units[i].pendingDestinations){
      if(!planUnitMovement(unit,destination))break;
      path.insert(path.end(),unit.waypoints.begin(),unit.waypoints.end());
      unit.position=unit.destination;
    }
    return path;
  }
  double Simulation::unitArrivalETA(size_t i) const{
    if(i>=state.units.size())return std::numeric_limits<double>::infinity();
    Unit unit=state.units[i];
    if(unit.health<=0||unit.remaining>0)return std::numeric_limits<double>::infinity();
    if(!unit.moving)return 0;
    if(unit.waypoints.empty()&&!planUnitMovement(unit,unit.destination))return std::numeric_limits<double>::infinity();
    double seconds=0;
    auto routeETA=[&](){
      for(Point point:unit.waypoints){
        double length=distanceKm(unit.position,point);
        // Sample changing road bonuses along each leg, retaining the current
        // support-technology modifier. Future combat and research are unknown.
        int samples=std::max(1,int(std::ceil(length/5.0)));
        Point start=unit.position;
        double dx=std::remainder(point.x-start.x,360.0),dy=point.y-start.y;
        for(int n=0;n<samples;++n){
          double t=(n+.5)/samples;
          unit.position={std::remainder(start.x+dx*t,360.0),start.y+dy*t};
          double speed=unitSpeedKmPerSecond(unit);
          if(speed<=0){seconds=std::numeric_limits<double>::infinity();return;}
          seconds+=length/(samples*speed);
        }
        unit.position=point;
      }
    };
    routeETA();
    for(Point point:state.units[i].pendingDestinations){
      if(!planUnitMovement(unit,point))return std::numeric_limits<double>::infinity();
      routeETA();
    }
    return seconds;
  }
  void Simulation::unitsTick(){
    for(auto& u:state.units){
      if(u.health<=0){u.pendingDestinations.clear();continue;}
      if(u.remaining>0)continue; // Production is allocated by economy(), per producer.
      if(!u.moving)continue;
      if(u.waypoints.empty()){
        // Older snapshots have region-only orders; validate a fresh geometric route.
        if(!planUnitMovement(u,u.destination)){u.moving=false;u.path.clear();u.pendingDestinations.clear();continue;}
      }
      Point destination=u.waypoints.front();
      double dx=std::remainder(destination.x-u.position.x,360.0),dy=destination.y-u.position.y;
      double length=distanceKm(u.position,destination);
      double speed=unitSpeedKmPerSecond(u)*simulationTickSeconds;
      if(length<=speed){
        u.position=destination;u.waypoints.erase(u.waypoints.begin());
        int reached=regionAt(destination);
        if(!u.path.empty()&&reached==u.path.front())u.path.erase(u.path.begin());
        if(u.waypoints.empty()){
          u.moving=false;u.path.clear();if(u.target>=0)u.region=u.target;
          if(!u.pendingDestinations.empty()){
            Point next=u.pendingDestinations.front();
            u.pendingDestinations.erase(u.pendingDestinations.begin());
            // Ownership or transport capacity may have changed since the order.
            if(!planUnitMovement(u,next))u.pendingDestinations.clear();
          }
        }
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
