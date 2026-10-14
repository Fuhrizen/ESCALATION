#include "escalation/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
namespace escalation {
  static bool ground(UnitType t){return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces||t==UnitType::MissileLauncher;}
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
  bool Simulation::returnToPort(size_t i){
    if(i>=state.units.size())return false;
    const auto& ship=state.units[i];
    if(ship.owner!=state.player||!navalUnit(ship.type)||ship.health<=0||ship.remaining>0||!ship.deployed)return false;
    std::vector<size_t> yards;
    for(size_t n=0;n<state.sites.size();++n){const auto& yard=state.sites[n];if(yard.type==Facility::Shipyard&&!yard.destroyed&&yard.enabled&&yard.remaining<=0&&yard.region>=0&&yard.region<int(state.regions.size())&&state.regions[yard.region].owner==ship.owner)yards.push_back(n);}
    std::sort(yards.begin(),yards.end(),[&](size_t a,size_t b){return distanceKm(ship.position,state.sites[a].position)<distanceKm(ship.position,state.sites[b].position);});
    for(size_t index:yards){const auto& yard=state.sites[index];for(double radius:{5.0,10.0,20.0})for(int n=0;n<24;++n){double a=n*6.28318530718/24;Point p{yard.position.x+radius/111.2*std::cos(a)/std::max(.2,std::cos(yard.position.y*.01745329252)),yard.position.y+radius/111.2*std::sin(a)};
      // Returning an existing ship does not consume ready deployment inventory.
      if(world_->pick(p)>=0||world_->canonical(nearestCoastNation(*world_,p))!=ship.owner||distanceKm(p,yard.position)>30)continue;
      Unit planned=ship;if(!planUnitMovement(planned,p))continue;
      planned.pendingDestinations.clear();planned.portServiceSeconds=0;state.units[i]=std::move(planned);log("Return to port ordered");return true;
    }}
    return false;
  }
  bool Simulation::orderUnit(size_t i,int r,bool append){
    if(r<0||r>=int(state.regions.size()))return false;
    return orderUnitAt(i,state.regions[r].center,append);
  }
  bool Simulation::orderUnitAt(size_t i,Point p,bool append){
    if(i>=state.units.size())return false;
    auto& u=state.units[i];
    if(u.owner!=state.player||u.health<=0||u.remaining>0||!u.deployed)return false;
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
    const bool sea=navalUnit(u.type);
    if(!activeUnit(u.type)||!u.deployed||(!sea&&!accessible(destination))||(sea&&world_->pick(p)>=0))return false;
    // Bounds are prepared once per sea order. Country::contains otherwise
    // rescans every coastline just to reconstruct longitude bounds per sample.
    struct CoastBounds{const Polygon* polygon;double west,east,south,north;};
    std::vector<CoastBounds> coasts;
    if(sea)for(const auto& country:world_->countries)for(const auto& polygon:country.polygons){
      if(polygon.rings.empty()||polygon.rings.front().empty())continue;
      CoastBounds box{&polygon,1e30,-1e30,1e30,-1e30};
      for(Point v:polygon.rings.front()){box.west=std::min(box.west,v.x);box.east=std::max(box.east,v.x);box.south=std::min(box.south,v.y);box.north=std::max(box.north,v.y);}coasts.push_back(box);
    }
    auto inRing=[](const Ring& ring,Point q){bool inside=false;for(size_t i=0,j=ring.size()-1;i<ring.size();j=i++){Point a=ring[i],b=ring[j];if((a.y>q.y)!=(b.y>q.y)&&q.x<(b.x-a.x)*(q.y-a.y)/(b.y-a.y)+a.x)inside=!inside;}return inside;};
    auto traversable=[&](Point q){
      if(sea){for(const auto& box:coasts){if(q.y<box.south||q.y>box.north)continue;Point shifted=q;double reference=(box.west+box.east)/2;shifted.x=reference+std::remainder(q.x-reference,360.0);if(shifted.x<box.west||shifted.x>box.east||!inRing(box.polygon->rings.front(),shifted))continue;bool hole=false;for(size_t r=1;r<box.polygon->rings.size();++r)if(inRing(box.polygon->rings[r],shifted)){hole=true;break;}if(!hole)return false;}return true;}
      int nation=-1;
      for(int candidate:{state.player,state.enemy})if(world_->countries[candidate].contains(q)){nation=candidate;break;}
      if(nation<0)return false;
      int nearest=-1;double best=1e30;
      for(size_t r=0;r<state.regions.size();++r)if(state.regions[r].nation==nation){
        Point center=state.regions[r].center;double dx=std::remainder(q.x-center.x,360.0),dy=q.y-center.y;
        double d=dx*dx+dy*dy;if(d<best){best=d;nearest=int(r);}
      }
      return accessible(nearest);
    };
    if(!traversable(u.position))return false;
    auto clearSegment=[&](Point a,Point b){
      double dx=std::remainder(b.x-a.x,360.0),dy=b.y-a.y;
      int samples=std::max(1,int(std::ceil(std::hypot(dx,dy)/(sea?0.025:0.08))));
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
      std::vector<int> coastalNations;
      if(sea){for(size_t n=0;n<world_->countries.size();++n)coastalNations.push_back(int(n));}
      else coastalNations={state.player,state.enemy};
      const size_t nodeLimit=sea?360:120;
      for(int nation:coastalNations)for(const auto& polygon:world_->countries[nation].polygons){
        if(polygon.rings.empty())continue;
        const auto& ring=polygon.rings.front();
        for(size_t k=0;k<ring.size()&&nodes.size()<nodeLimit;++k){
          Point v=ring[k];
          if(distance(v,u.position)+distance(v,p)>distance(u.position,p)+6)continue;
          // Sample both sides of the coast; only traversable candidates survive.
          for(int angle=0;angle<8&&nodes.size()<nodeLimit;++angle){
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
    if(!unit.moving||unit.health<=0||unit.remaining>0||!unit.deployed)return {};
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
    if(unit.health<=0||unit.remaining>0||!unit.deployed)return std::numeric_limits<double>::infinity();
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
      if(u.remaining>0||!u.deployed)continue; // Production is allocated by economy(), per producer.
      if(u.owner==state.player&&ground(u.type)&&hasTech(12)&&u.health<100){
        bool engaged=false;
        if(state.war)for(const auto& enemy:state.units)if(enemy.owner!=u.owner&&enemy.deployed&&enemy.health>0&&enemy.remaining<=0&&activeUnit(enemy.type)){
          if(canEngage(u,enemy)||canEngage(enemy,u)){engaged=true;break;}
        }
        for(const auto& shot:state.engagements)if(shot.defender==int(&u-state.units.data()))engaged=true;
        if(!engaged)u.health=std::min(100.0,u.health+.10*simulationTickSeconds);
      }
      if(navalUnit(u.type)){
        bool port=false,engaged=false;
        for(const auto& yard:state.sites)if(yard.type==Facility::Shipyard&&!yard.destroyed&&yard.enabled&&yard.remaining<=0&&yard.region>=0&&yard.region<int(state.regions.size())&&state.regions[yard.region].owner==u.owner&&distanceKm(u.position,yard.position)<=30){port=true;break;}
        if(state.war)for(const auto& enemy:state.units)if(enemy.owner!=u.owner&&enemy.deployed&&enemy.health>0&&enemy.remaining<=0&&activeUnit(enemy.type)&&distanceKm(u.position,enemy.position)<=std::max(engagementRange(u,enemy),engagementRange(enemy,u))){engaged=true;break;}
        for(const auto& shot:state.engagements)if(shot.defender==int(&u-state.units.data())||shot.attacker==int(&u-state.units.data()))engaged=true;
        if(u.moving||!port||engaged)u.portServiceSeconds=0;
        else{
          u.portServiceSeconds+=simulationTickSeconds;
          if(u.portServiceSeconds+1e-9>=30){const auto weapons=baseWeapons(u.type);u.weaponCooldowns.assign(weapons.size(),0);u.weaponReloads.assign(weapons.size(),0);u.weaponAmmo.clear();u.weaponReserve.clear();for(const auto& weapon:weapons){u.weaponAmmo.push_back(weapon.magazine);u.weaponReserve.push_back(weapon.reserveAmmo);}u.health=std::min(100.0,u.health+10);u.portServiceSeconds=0;}
        }
      }
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
      for(auto& u:state.units)if(u.owner==state.player&&ground(u.type)&&u.health>0&&u.remaining<=0&&u.deployed)state.brigades+=u.health/100;
    }
  }
}
