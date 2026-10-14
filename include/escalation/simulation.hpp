#pragma once
#include "world.hpp"
#include <array>
#include <deque>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
namespace escalation {
  constexpr double simulationTickSeconds=0.25;
  double distanceKm(Point a,Point b);
  int nearestCoastNation(const World& world,Point position);
  enum class Resource {
    Ore,Oil,Steel,Fuel,Supplies,Equipment,Warheads,Count
  };
  constexpr int resourceCount=int(Resource::Count);
  extern const std::array<const char*,resourceCount> resourceNames;
  enum class Field {
    Army,Defence,Sensors,SpecialOps,Ballistic,Economy,Research,Logistics,Naval,Count
  };
  extern const std::array<const char*,9> fieldNames;
  enum class Facility {
    Mine,OilWell,Steelworks,Refinery,Factory,PowerPlant,Laboratory,Depot,Shipyard,Silo,ArmyBase,Airbase,Radar,Farm,IndustrialPark,Commercial,Count
  };
  extern const std::array<const char*,16> facilityNames;
  enum class UnitType { Infantry, Armour, Artillery, Fighter, Bomber, Destroyer, Submarine, SpecialForces, MissileLauncher, Cruiser, MissileSubmarine, Count };
  extern const std::array<const char*,int(UnitType::Count)> unitNames;
  enum class TargetDomain : unsigned { Ground=1, Naval=2, Submerged=4, Air=8 };
  struct WeaponProfile {
    std::string id,name,behaviour;
    unsigned targetMask=0;
    double minRangeKm=0,maxRangeKm=0,damage=0,cooldownSeconds=1;
    int magazine=1,reserveAmmo=0;
    double reloadSeconds=1,projectileSpeedKmPerSecond=0,impactRadiusKm=2;
    bool armourOnly=false;
  };
  struct Engagement {
    Point origin,target;
    int attacker=-1,defender=-1;
    std::string weaponId;
    unsigned targetMask=0;
    double damage=0,remaining=0,total=0,impactRadiusKm=2,impactSeconds=0;
    bool guided=false,ballistic=false,nuclear=false,areaStrike=false;
    int owner=-1,sourceSite=-1;
  };
  struct Unit {
    int owner=-1,region=0,target=-1,sourceSite=-1;
    UnitType type=UnitType::Infantry;
    Point position,destination;
    double health=100,remaining=0,total=0;
    std::vector<int> path;
    std::vector<Point> waypoints;
    std::vector<Point> pendingDestinations;
    std::vector<double> weaponCooldowns,weaponReloads;
    std::vector<int> weaponAmmo,weaponReserve;
    double portServiceSeconds=0;
    bool moving=false;
    bool deployed=true;
  };
  struct UnitStats { double speedKmPerSecond,rangeKm,damagePerSecond; };
  UnitStats unitStats(UnitType type);
  bool canEngage(UnitType attacker,UnitType defender);
  struct Capability { std::string name,value,icon; bool active=true; };
  bool groundUnit(UnitType type);
  bool navalUnit(UnitType type);
  bool activeUnit(UnitType type);
  TargetDomain targetDomain(UnitType type);
  std::vector<WeaponProfile> baseWeapons(UnitType type);
  int weaponSlot(UnitType type,const std::string& id);
  bool weaponTargets(const WeaponProfile& weapon,UnitType target);
  struct Tech {
    std::string id,name,effect;
    Field field;
    int tier,year;
    double cost;
    std::vector<int> prerequisites;
    std::string sourceKey,sourceImage,sourceCategory;
    double sourceLevel=0,sourcePoints=0,levelContribution=0;
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
    double missileCooldown=0;
  };
  struct Route {
    int a=0,b=0;
    double capacity=8;
    bool enabled=true;
  };
  struct BuildingJob {
    Facility type=Facility::Factory;
    double progress=0,total=0;
  };
  struct ResearchJob {
    int tech=0;
    double progress=0,cost=0;
  };
  struct Settlement {
    std::string name;
    Point position;
    double population=0;
    int region=-1,owner=-1;
    double capture=0;
  };
  struct TerritoryCell { int region=-1,owner=-1; bool occupied=false; };
  constexpr double territoryCellDegrees=0.1;
  struct Campaign {
    int player=-1,enemy=-1,day=0,capital=0,enemyCapital=0,target=-1;
    double money=8000,stability=100,brigades=4,researchBudget=0.5;
    bool war=false,victory=false,defeat=false,nuclearUsed=false;
    bool simpleEconomy=true;
    Field specialization=Field::Economy;
    std::array<double,resourceCount> stock{},delta{};
    std::array<int,9> levels{};
    std::vector<Region> regions;
    std::unordered_map<int,TerritoryCell> territory;
    std::uint64_t territoryRevision=0;
    std::vector<Settlement> settlements;
    std::vector<Site> sites;
    std::vector<Unit> units;
    std::vector<Engagement> engagements;
    std::vector<Route> routes;
    std::vector<bool> completed;
    std::deque<ResearchJob> research;
    std::deque<BuildingJob> buildingQueue;
    std::array<int,16> buildingInventory{};
    std::deque<std::string> events;
    double outputRate=0,productionRate=0,researchRate=0,power=0,powerDemand=0,income=0,upkeep=0;
    int warheadsQueued=0;
    double warheadWork=0;
    int lastAutosave=0;
  };
  class Simulation {
    const World* world_;
    std::size_t territoryFingerprint_=0;
    void log(const std::string& text);
    void economy(bool advance=true);
    bool productionSiteActive(const Site& site) const;
    bool productionUnitActive(const Unit& unit) const;
    bool strategicProductionActive() const;
    double prospectiveResearchRate() const;
    double prospectiveProductionRate() const;
    void researchTick();
    void combat();
    void updateTerritory();
    void unitsTick();
    bool planUnitMovement(Unit& unit,Point destination) const;
    public: std::vector<Tech> techs;
    Campaign state;
    std::uint64_t campaignRevision=0;
    explicit Simulation(const World& world):world_(&world),techs(makeTechTree()){} void start(int nation,Field focus=Field::Economy,bool simpleEconomy=true);
    void tick();
    void refreshRates(){economy(false);}
    int year() const{
      return 1920+state.day/365;
    }
    double connection(int region) const;
    double researchCost(int tech) const;
    double researchLevel() const;
    double researchETA(int tech) const;
    double buildingETA(Facility type) const;
    double buildingETA(size_t queueIndex) const;
    double unitETA(size_t index) const;
    double unitArrivalETA(size_t index) const;
    std::vector<Point> unitPlannedPath(size_t index) const;
    double unitBuildETA(UnitType type,int site) const;
    double unitSpeedKmPerSecond(const Unit& unit) const;
    double unitRangeKm(const Unit& unit) const;
    std::vector<WeaponProfile> unitWeapons(const Unit& unit) const;
    double engagementRange(const Unit& attacker,const Unit& defender) const;
    bool canEngage(const Unit& attacker,const Unit& defender) const;
    int mainSettlement(int region) const;
    double frontlinePressure(Point position) const;
    int territoryOwner(Point position) const;
    static int territoryKey(Point position);
    static Point territoryCenter(int key);
    double roadMovementBonus(Point position) const;
    bool hasTech(int tech) const;
    bool visibleResearch(int tech) const;
    std::vector<Capability> unitCapabilities(size_t index) const;
    std::vector<Capability> siteCapabilities(size_t index) const;
    double sonarRangeKm(const Unit& unit) const;
    bool detectsSubmarine(const Unit& observer,const Unit& target) const;
    double unitDamagePerSecond(const Unit& unit,UnitType defender=UnitType::Infantry) const;
    double unitProtection(const Unit& unit) const;
    int readyUnitCount(UnitType type) const;
    std::string deploymentReason(UnitType type,Point position) const;
    bool deployUnit(UnitType type,Point position);
    bool available(int tech) const;
    std::string lockReason(int tech) const;
    bool queueResearch(int tech);
    void removeResearch(size_t index);
    double buildCost(Facility type) const;
    std::string buildReason(int region,Facility type) const;
    std::string buildingQueueReason(Facility type) const;
    bool queueBuilding(Facility type);
    bool cancelBuilding(size_t index);
    std::string placementReason(Facility type,int region,Point position) const;
    bool placeBuilding(Facility type,int region,Point position);
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
    bool orderUnit(size_t index,int destination,bool append=false);
    bool orderUnitAt(size_t index,Point destination,bool append=false);
    void holdUnit(size_t index);
    bool returnToPort(size_t index);
    bool cancelUnit(size_t index);
    bool connect(int a,int b);
    bool upgradeRoute(size_t route);
    bool recruit();
    bool declareWar();
    bool attack(int region);
    bool produceWarhead();
    bool launchNuclear(int region);
    double siteMissileRange(size_t index) const;
    std::string missileStrikeReason(size_t index,Point target,bool nuclear=false) const;
    bool launchMissile(size_t index,Point target,bool nuclear=false);
    std::string unitAttackReason(size_t index,Point target) const;
    bool orderUnitAttackAt(size_t index,Point target);
    void save(const std::string& path) const;
    void load(const std::string& path);
  };
}
