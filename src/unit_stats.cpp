#include "escalation/simulation.hpp"
#include <algorithm>
#include <cmath>

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
    case UnitType::Fighter:return {8,35,1.6};
    case UnitType::Bomber:return {5,20,1.5};
    case UnitType::Destroyer:return {1.4,45,1.5};
    case UnitType::Submarine:return {1.1,20,1.6};
    case UnitType::SpecialForces:return {.8,2,1.2};
    case UnitType::MissileLauncher:return {.6,120,1.2};
    case UnitType::Count:break;
  }
  return {0,0,0};
}
bool canEngage(UnitType a,UnitType b){
  auto sea=[](UnitType t){return t==UnitType::Destroyer||t==UnitType::Submarine;};
  auto land=[](UnitType t){return t==UnitType::Infantry||t==UnitType::Armour||t==UnitType::Artillery||t==UnitType::SpecialForces||t==UnitType::MissileLauncher;};
  if(sea(a))return sea(b)||b==UnitType::Bomber;
  if(a==UnitType::Fighter)return b==UnitType::Fighter||b==UnitType::Bomber;
  if(a==UnitType::Bomber)return land(b)||sea(b);
  return land(b);
}
double Simulation::unitSpeedKmPerSecond(const Unit& unit) const {
  double speed=unitStats(unit.type).speedKmPerSecond;
  if(unit.owner==state.player)speed*=1+.03*state.levels[7];
  if(unit.type==UnitType::Infantry||unit.type==UnitType::Armour||unit.type==UnitType::Artillery||unit.type==UnitType::SpecialForces||unit.type==UnitType::MissileLauncher)
    speed*=1+roadMovementBonus(unit.position);
  return speed;
}
double Simulation::unitRangeKm(const Unit& unit) const {return unitStats(unit.type).rangeKm;}
int Simulation::mainSettlement(int region) const {
  int result=-1;
  for(size_t i=0;i<state.settlements.size();++i)if(state.settlements[i].region==region&&(result<0||state.settlements[i].population>state.settlements[result].population))result=int(i);
  return result;
}
}
