#pragma once
#include "world.hpp"
#include <array>
#include <deque>
#include <string>
#include <vector>
namespace escalation {
  enum class Resource {
    Ore,Oil,Steel,Fuel,Supplies,Equipment,Warheads,Count
  };
  constexpr int resourceCount=int(Resource::Count);
  extern const std::array<const char*,resourceCount> resourceNames;
  enum class Field {
    Army,Air,Naval,SpecialOps,Ballistic,Economy,Research,Logistics,Count
  };
  extern const std::array<const char*,8> fieldNames;
  enum class Facility {
    Mine,OilWell,Steelworks,Refinery,Factory,PowerPlant,Laboratory,Depot,Shipyard,Silo,ArmyBase,Airbase,Radar,Farm,IndustrialPark,Commercial,Count
  };
  extern const std::array<const char*,16> facilityNames;
  enum class UnitType { Infantry, Armour, Artillery, Fighter, Bomber, Destroyer, Submarine, SpecialForces, MissileLauncher, Count };
  extern const std::array<const char*,9> unitNames;
  struct Unit {
    int owner=-1,region=0,target=-1,sourceSite=-1;
    UnitType type=UnitType::Infantry;
    Point position,destination;
    double health=100,remaining=0,total=0;
    std::vector<int> path;
    bool moving=false;
  };
  struct Tech {
    std::string id,name,effect;
    Field field;
    int tier,year;
    double cost;
    std::vector<int> prerequisites;
  };
  std::vector<Tech> makeTechTree();
  struct Region {
    int nation=0,owner=0;
    Point center;
    std::vector<int> neighbours;
    double ore=1,oil=1,development=1,defense=1,capture=0;
    int roads=0;
  };
  struct Site {
    int region=0;
    Facility type=Facility::Mine;
    double remaining=0,efficiency=0;
    bool enabled=true;
    Point position;
    bool destroyed=false;
  };
  struct Route {
    int a=0,b=0;
    double capacity=8;
    bool enabled=true;
  };
  struct ResearchJob {
    int tech=0;
    double progress=0;
  };
  struct Campaign {
    int player=-1,enemy=-1,day=0,capital=0,enemyCapital=0,target=-1;
    double money=8000,stability=100,brigades=4,researchBudget=0.5;
    bool war=false,victory=false,defeat=false,nuclearUsed=false;
    bool simpleEconomy=true;
    Field specialization=Field::Economy;
    std::array<double,resourceCount> stock{},delta{};
    std::array<int,8> levels{};
    std::vector<Region> regions;
    std::vector<Site> sites;
    std::vector<Unit> units;
    std::vector<Route> routes;
    std::vector<bool> completed;
    std::deque<ResearchJob> research;
    std::deque<std::string> events;
    double researchRate=0,power=0,powerDemand=0,income=0,upkeep=0;
    int lastAutosave=0;
  };
  class Simulation {
    const World* world_;
    void log(const std::string& text);
    void economy();
    void researchTick();
    void combat();
    void unitsTick();
    public: std::vector<Tech> techs;
    Campaign state;
    explicit Simulation(const World& world):world_(&world),techs(makeTechTree()){} void start(int nation,Field focus=Field::Economy,bool simpleEconomy=true);
    void tick();
    int year() const{
      return 1920+state.day/365;
    }
    double connection(int region) const;
    bool available(int tech) const;
    std::string lockReason(int tech) const;
    bool queueResearch(int tech);
    void removeResearch(size_t index);
    double buildCost(Facility type) const;
    std::string buildReason(int region,Facility type) const;
    bool build(int region,Facility type);
    bool buildAt(int region,Facility type,Point position);
    int regionAt(Point position) const;
    bool cancelSite(size_t index);
    double buildingIncome(Facility type) const;
    double regionIncome(int region) const;
    double roadCost(int region) const;
    bool upgradeRoad(int region);
    double unitCost(UnitType type) const;
    double unitDays(UnitType type) const;
    std::string unitReason(UnitType type,int site) const;
    bool queueUnit(UnitType type,int site);
    bool orderUnit(size_t index,int destination);
    bool orderUnitAt(size_t index,Point destination);
    void holdUnit(size_t index);
    bool cancelUnit(size_t index);
    bool connect(int a,int b);
    bool upgradeRoute(size_t route);
    bool recruit();
    bool declareWar();
    bool attack(int region);
    bool produceWarhead();
    bool launchNuclear(int region);
    void save(const std::string& path) const;
    void load(const std::string& path);
  };
}
