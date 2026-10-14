#include "escalation/simulation.hpp"
#include "escalation/json.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <queue>
#include <set>
#include <limits>
namespace escalation {
  const std::array<const char*,resourceCount> resourceNames={"Ore","Crude oil","Steel","Fuel","Supplies","Equipment","Warheads"};
  const std::array<const char*,8> fieldNames={"Land warfare","Defence","Sensors","Special operations","Missiles","Industry","Science","Support"};
  const std::array<const char*,16> facilityNames={"Mine","Oil field","Steelworks","Refinery","Arms plant","Power station","Research centre","Command centre","Shipyard","Missile site","Army base","Airbase","Radar station","Farm","Industrial district","Commercial district"};
  std::vector<Tech> makeTechTree(){
    static const char* labels[8][14]={
      {"Infantry foundations","Mobility","Infantry weapons","Protection","Motor transport","Support weapons","Body armour","Mechanised transport","Small arms","Artillery batteries","Precision artillery","Long-range guns","Field medicine","Combined assault"},
      {"Defence foundation","Field fortification","Body armour","MANPADS","Anti-tank weapons","Composite armour","Reactive armour","Active protection","Dispersed formations","Counterbattery defence","Hardened vehicles","Survivability drills","Advanced composites","Layered protection"},
      {"Sensors foundation","Ground surveillance","Fire control","Counterbattery radar","Optical ranging","Thermal sights","Digital targeting","Precision fire control","Passive observation","Target fusion","Artillery ranging","Advanced fire control","Distributed observers","Integrated ground sensors"},
      {"Patrol doctrine","Infantry reconnaissance","Raiding doctrine","Long-range patrols","Special forces","Concealment drills","Precision rifles","Patrol mobility","Urban assault","Night operations","Advanced reconnaissance","Specialist weapons","Distributed patrols","Adaptive reconnaissance"},
      {"Rocket experiments","Liquid fuel rockets","Guided missiles","Nuclear deterrence","Intercontinental missiles","Multiple warheads","Mobile launchers","Precision guidance","Missile defence","Hypersonic research","Boost glide systems","Strategic surveillance","Resilient deterrence","Strategic command networks"},
      {"Industrial foundation","Assembly lines","Industrial districts","Factory automation","Efficient production","Manufacturing systems","Flexible manufacturing","Production planning","Lean production","Smart factories","Industrial robotics","Circular production","Advanced fabrication","Autonomous industry"},
      {"Research foundation","Research institutes","Applied science","Computing centres","Scientific instruments","Scientific computing","Supercomputers","Networked laboratories","Research collaboration","Machine learning tools","Advanced simulation","Materials discovery","Quantum research","Accelerated discovery"},
      {"Support foundation","Truck logistics","Forward depots","Ground transport","Supply planning","Route planning","Digital inventories","Convoy coordination","Predictive routing","Resilient distribution","Autonomous convoys","Adaptive depots","Distributed reserves","Self organising logistics"}};
    // Three branches from each foundation, then forks within those branches.
    static const int parent[14]={-1,0,0,0,1,2,3,4,5,5,9,9,6,7};
    std::vector<Tech> result;
    for(int f=0;f<8;++f)for(int tier=0;tier<14;++tier){
      Tech t{std::to_string(f)+"-"+std::to_string(tier),labels[f][tier],"Unlock a ground research branch",Field(f),tier,1920+tier*10,400+double(tier)*220,{}};
      if(parent[tier]>=0)t.prerequisites.push_back(f*14+parent[tier]);
      result.push_back(t);
    }
    result[7].prerequisites={1};result[8].prerequisites={2};result[12].prerequisites={3};
    result[17].prerequisites={5};result[18].prerequisites={5};
    result[59].prerequisites={58,87};result[60].prerequisites={59};result[61].prerequisites={59};result[62].prerequisites={58};result[63].prerequisites={62};result[65].prerequisites={63};
    auto effect=[&](int id,const char* value){result[id].effect=value;};
    effect(1,"Ground movement +10%");effect(2,"Infantry weapon damage +10%");effect(3,"Ground incoming damage -5%");effect(4,"Infantry movement +15%");effect(6,"Infantry incoming damage -10%");effect(7,"Unlock armoured brigades; armour movement +15%");effect(8,"Infantry weapon damage +10%");effect(9,"Unlock artillery batteries");effect(10,"Artillery damage +20%");effect(11,"Artillery range +20%");effect(12,"Ground formations recover 0.10 health/s while disengaged");effect(13,"Ground damage +10%");
    for(int i=14;i<28;++i)effect(i,"Ground incoming damage -2%");
    for(int i=28;i<42;++i)effect(i,"Ground weapon range +2%");
    effect(46,"Unlock special forces");effect(48,"Special forces damage +15%");effect(49,"Special forces movement +15%");
    effect(17,"MANPADS equipment reserved; air targets are not implemented");effect(18,"Infantry anti-tank damage +30%");effect(59,"Unlock nuclear warhead production; requires Computing centres");effect(60,"Unlock nuclear delivery from an operational missile site");effect(62,"Unlock mobile missile batteries and missile sites");effect(63,"Missile damage +15%");effect(65,"Missile range +20%");
    for(int i=70;i<84;++i)effect(i,"Regional production +6%");
    for(int i=84;i<98;++i)effect(i,"Research output +8%");
    for(int i=98;i<112;++i)effect(i,"Ground movement +3%");
    // Reference years are descriptive, never availability gates. Keep existing
    // references where possible, correcting cross-branch chronology only.
    bool changed=true;while(changed){changed=false;for(auto& tech:result)for(int p:tech.prerequisites)if(tech.year<result[p].year){tech.year=result[p].year;changed=true;}}
    return result;
  }
  void Simulation::log(const std::string& s){
    state.events.push_front("Day "+std::to_string(state.day)+": "+s);
    if(state.events.size()>80)state.events.pop_back();
  }
  void Simulation::start(int nation,Field focus,bool simpleEconomy){
    if(nation<0||nation>=int(world_->countries.size())||world_->countries[nation].code=="ATA")throw std::runtime_error("Select a playable nation");
    if(int(focus)<0||int(focus)>=8)throw std::runtime_error("Invalid research specialization");
    nation=world_->canonical(nation);
    state=Campaign{};
    (void)simpleEconomy;
    state.simpleEconomy=true;
    state.player=nation;
    state.enemy=world_->opponent(nation);
    if(state.enemy<0)throw std::runtime_error("No opponent available");
    state.specialization=focus;
    state.completed.resize(techs.size());
    state.stock.fill(0);
    state.money=0;
    for(int country:{state.player,state.enemy}){
      auto shapes=world_->subdivide(country);
      int offset=int(state.regions.size());
      for(size_t i=0;i<shapes.size();++i){
        Region r;
        r.nation=country;
        r.owner=country;
        r.center=shapes[i].center;
        r.ore=0.7+double((i*7+country)%13)/10;
        r.oil=0.6+double((i*11+country)%11)/10;
        r.development=0.8+double((i*3+country)%9)/10;
        r.defense=country==state.enemy?1.0+double(i%4)*0.15:1;
        for(int j:shapes[i].neighbours)r.neighbours.push_back(offset+j);
        state.regions.push_back(r);
      }
      // Greedy land-site planner: separate extraction, industry, labs and strategic targets.
      auto choose=[&](Facility type){
        int best=offset;
        double score=-1;
        for(int i=offset;i<int(state.regions.size());++i){
          auto& r=state.regions[i];
          double s=type==Facility::Mine?r.ore:type==Facility::OilWell?r.oil:r.development;
          for(auto& site:state.sites)if(site.region==i)s*=0.15;
          if(s>score){
            score=s;
            best=i;
          }
        }
        state.sites.push_back({best,type,0,1,true});
        state.sites.back().position=state.regions[best].center;
        return best;
      };
      int cap=choose(Facility::Depot);
      if(country==state.player)state.capital=cap;
      else state.enemyCapital=cap;
      std::vector<Facility> starting={Facility::IndustrialPark,Facility::Factory,Facility::Laboratory,Facility::ArmyBase};
      for(Facility f:starting){
        int r=choose(f);
        state.routes.push_back({cap,r,8,true});
      }
    }
    // Connect touching frontier regions; islands receive a declared maritime theatre link.
    double closest=1e30;
    int a=-1,b=-1;
    for(int i=0;i<24;++i)for(int j=24;j<48;++j){
      double d=distance(state.regions[i].center,state.regions[j].center);
      if(d<closest){
        closest=d;
        a=i;
        b=j;
      }
    }
    state.regions[a].neighbours.push_back(b);
    state.regions[b].neighbours.push_back(a);
    for(int country:{state.player,state.enemy}){
      std::vector<int> sectors;for(size_t i=0;i<state.regions.size();++i)if(state.regions[i].nation==country)sectors.push_back(int(i));
      Point enemyCentre=world_->countries[country==state.player?state.enemy:state.player].center;
      std::sort(sectors.begin(),sectors.end(),[&](int x,int y){return distance(state.regions[x].center,enemyCentre)<distance(state.regions[y].center,enemyCentre);});
      for(int n=0;n<4;++n){Unit unit;unit.owner=country;unit.region=sectors[n];unit.position=state.regions[unit.region].center;unit.destination=unit.position;state.units.push_back(unit);}
    }
    for(const auto& city:world_->cities){
      // Imported national affiliation survives gameplay grouping and coastlines
      // simplified by the world map; a nearby real city must not disappear.
      int country=world_->canonical(world_->find(city.countryCode));
      if(country!=state.player&&country!=state.enemy)continue;
      Point position=city.position;
      if(!world_->countries[country].contains(position)){
        double best=std::numeric_limits<double>::max();
        for(const auto& triangle:world_->countries[country].triangles)for(int edge=0;edge<3;++edge){
          Point a=triangle[edge],b=triangle[(edge+1)%3];
          double dx=b.x-a.x,dy=b.y-a.y,den=dx*dx+dy*dy;
          double t=den>0?std::clamp(((city.position.x-a.x)*dx+(city.position.y-a.y)*dy)/den,0.0,1.0):0;
          Point q{a.x+t*dx,a.y+t*dy};
          double score=distance(q,city.position);
          if(score<best){best=score;Point centre{(triangle[0].x+triangle[1].x+triangle[2].x)/3,(triangle[0].y+triangle[1].y+triangle[2].y)/3};position={q.x*0.999+centre.x*0.001,q.y*0.999+centre.y*0.001};}
        }
      }
      int region=-1;double best=std::numeric_limits<double>::max();
      for(size_t r=0;r<state.regions.size();++r)if(state.regions[r].nation==country){Point q=state.regions[r].center;double dx=std::remainder(position.x-q.x,360.0),dy=position.y-q.y,score=dx*dx+dy*dy;if(score<best){best=score;region=int(r);}}
      if(region>=0)state.settlements.push_back({city.name,position,city.population,region,country,0});
    }
    for(size_t region=0;region<state.regions.size();++region){
      bool hasCity=false;
      for(const auto& city:state.settlements)if(city.region==int(region))hasCity=true;
      if(!hasCity)state.settlements.push_back({"Strategic objective "+std::to_string(region+1),state.regions[region].center,0,int(region),state.regions[region].owner,0});
    }
    log("Command established. Set the balance between research and construction.");
    if(world_->countries[nation].neighbours.empty())log("Island start: ground forces cannot cross ocean; select a connected theatre for conquest.");
    economy(false);
    state.delta.fill(0);
  }
  double Simulation::connection(int region) const{
    return region>=0&&region<int(state.regions.size())&&state.regions[region].owner==state.player?8:0;
  }
  double Simulation::researchCost(int tech) const{
    if(tech<0||tech>=int(techs.size()))return 1e30;
    double ahead=std::max(0,techs[tech].year-year());
    return techs[tech].cost*(1+0.12*ahead+0.002*ahead*ahead);
  }
  bool Simulation::productionSiteActive(const Site& site) const {
    return !site.destroyed&&site.enabled&&site.remaining>0&&site.region>=0&&site.region<int(state.regions.size())&&state.regions[site.region].owner==state.player;
  }
  bool Simulation::productionUnitActive(const Unit& unit) const {
    if(unit.owner!=state.player||unit.health<=0||unit.remaining<=0||unit.sourceSite<0||unit.sourceSite>=int(state.sites.size()))return false;
    const auto& source=state.sites[unit.sourceSite];return !source.destroyed&&source.enabled&&source.remaining<=0&&state.regions[source.region].owner==state.player;
  }
  bool Simulation::strategicProductionActive() const {
    if(state.warheadsQueued<=0)return false;
    bool factory=false,lab=false;for(const auto& site:state.sites)if(!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player){factory|=site.type==Facility::Factory;lab|=site.type==Facility::Laboratory;}return factory&&lab;
  }
  double Simulation::prospectiveProductionRate() const {
    bool lab=false;for(const auto& site:state.sites)if(site.type==Facility::Laboratory&&!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player)lab=true;
    return state.outputRate*((!state.research.empty()&&lab)?1-state.researchBudget:1);
  }
  double Simulation::prospectiveResearchRate() const {
    double labs=0;for(const auto& site:state.sites)if(site.type==Facility::Laboratory&&!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player)++labs;
    bool producing=!state.buildingQueue.empty()||strategicProductionActive();for(const auto& site:state.sites)producing|=productionSiteActive(site);for(const auto& unit:state.units)producing|=productionUnitActive(unit);
    return labs>0?state.outputRate*(producing?state.researchBudget:1)*(1+0.08*labs)*(1+0.08*std::count(state.completed.begin()+84,state.completed.begin()+98,true)):0;
  }
  double Simulation::researchETA(int tech) const {
    if(tech<0||tech>=int(techs.size()))return std::numeric_limits<double>::infinity();
    if(state.completed[tech])return 0;
    double rate=prospectiveResearchRate();
    if(rate<=0)return std::numeric_limits<double>::infinity();
    double seconds=0;
    for(const auto& job:state.research){seconds+=(job.cost-job.progress)/(rate*0.4*(techs[job.tech].field==state.specialization?1.35:1));if(job.tech==tech)return seconds;}
    return seconds+researchCost(tech)/(rate*0.4*(techs[tech].field==state.specialization?1.35:1));
  }
  double Simulation::buildingETA(size_t index) const {
    if(index>=state.buildingQueue.size()||state.productionRate<=0)return std::numeric_limits<double>::infinity();
    double pending=0;for(size_t i=0;i<=index;++i)pending+=state.buildingQueue[i].total-state.buildingQueue[i].progress;
    double parallel=0;for(const auto& site:state.sites)if(productionSiteActive(site))parallel+=std::min(site.remaining,pending);for(const auto& unit:state.units)if(productionUnitActive(unit))parallel+=std::min(unit.remaining,pending);
    if(strategicProductionActive())parallel+=std::min(state.warheadWork+1200*(state.warheadsQueued-1),pending);
    return (pending+parallel)/(state.productionRate*0.1);
  }
  double Simulation::buildingETA(Facility type) const {
    double rate=prospectiveProductionRate();if(rate<=0)return std::numeric_limits<double>::infinity();
    double pending=buildCost(type);for(const auto& job:state.buildingQueue)pending+=job.total-job.progress;
    double parallel=0;for(const auto& site:state.sites)if(productionSiteActive(site))parallel+=std::min(site.remaining,pending);for(const auto& unit:state.units)if(productionUnitActive(unit))parallel+=std::min(unit.remaining,pending);
    if(strategicProductionActive())parallel+=std::min(state.warheadWork+1200*(state.warheadsQueued-1),pending);
    return (pending+parallel)/(rate*0.1);
  }
  double Simulation::unitETA(size_t index) const {
    if(index>=state.units.size())return std::numeric_limits<double>::infinity();
    const auto& target=state.units[index];if(target.remaining<=0)return 0;
    if(state.productionRate<=0||!productionUnitActive(target))return std::numeric_limits<double>::infinity();
    double pending=target.remaining;for(const auto& site:state.sites)if(productionSiteActive(site))pending+=std::min(site.remaining,target.remaining);for(size_t i=0;i<state.units.size();++i)if(i!=index&&productionUnitActive(state.units[i]))pending+=std::min(state.units[i].remaining,target.remaining);
    double buildings=0;for(const auto& job:state.buildingQueue)buildings+=job.total-job.progress;pending+=std::min(buildings,target.remaining);
    if(strategicProductionActive())pending+=std::min(state.warheadWork+1200*(state.warheadsQueued-1),target.remaining);
    return pending/(state.productionRate*0.1);
  }
  std::string Simulation::lockReason(int tech) const {
    if(tech<0||tech>=int(techs.size()))return "Invalid technology";
    if(state.player<0||state.completed.empty()||state.victory||state.defeat)return "No active campaign";
    if(state.completed[tech])return "Already researched";
    for(int p:techs[tech].prerequisites)if(!state.completed[p])return "Requires "+techs[p].name;
    for(auto& j:state.research)if(j.tech==tech)return "Already queued";
    return "";
  }
  bool Simulation::available(int tech) const{
    return lockReason(tech).empty();
  }
  bool Simulation::queueResearch(int tech){
    if(!available(tech)||state.research.size()>=12)return false;
    state.research.push_back({tech,0,researchCost(tech)});
    log("Queued research: "+techs[tech].name);
    economy(false);
    return true;
  }
  void Simulation::removeResearch(size_t index){
    if(index<state.research.size()){state.research.erase(state.research.begin()+std::ptrdiff_t(index));economy(false);}
  }
  static bool activeFacility(Facility f){
    return f==Facility::Factory||f==Facility::Laboratory||f==Facility::Depot||f==Facility::Silo||f==Facility::ArmyBase||f==Facility::Radar||f==Facility::IndustrialPark;
  }
  double Simulation::buildCost(Facility type) const {
    static double cost[]={0,0,0,0,100,0,120,40,160,180,80,120,90,0,140,0};
    return int(type)>=0&&int(type)<16&&activeFacility(type)?cost[int(type)]:1e30;
  }
  std::string Simulation::buildReason(int region,Facility type) const {
    if(state.player<0||state.victory||state.defeat)return "No active campaign";
    if(region<0||region>=int(state.regions.size())||state.regions[region].owner!=state.player)return "Build on controlled land";
    if(!activeFacility(type))return "Retired economic facility";
    if(state.sites.size()>=1000)return "Campaign construction history capacity reached";
    if(type==Facility::Radar&&!hasTech(29))return "Requires Ground surveillance";
    if(type==Facility::Silo&&!hasTech(62)&&!hasTech(59))return "Requires Mobile launchers or Nuclear deterrence";
    int count=0;
    for(auto& s:state.sites)if(s.region==region&&!s.destroyed)++count;
    if(count>=8)return "Region has reached its building capacity";
    return "";
  }
  std::string Simulation::buildingQueueReason(Facility type) const {
    if(state.player<0||state.victory||state.defeat)return "No active campaign";
    if(!activeFacility(type))return "Retired economic facility";
    if(state.buildingQueue.size()>=64)return "Construction queue is full";
    int reserved=state.buildingInventory[int(type)];for(const auto& job:state.buildingQueue)if(job.type==type)++reserved;
    if(reserved>=1000)return "Ready inventory is full";
    if(type==Facility::Radar&&!hasTech(29))return "Requires Ground surveillance";
    if(type==Facility::Silo&&!hasTech(62)&&!hasTech(59))return "Requires Mobile launchers or Nuclear deterrence";
    return "";
  }
  bool Simulation::queueBuilding(Facility type){
    if(!buildingQueueReason(type).empty())return false;
    state.buildingQueue.push_back({type,0,buildCost(type)});
    log(std::string("Manufacture queued: ")+facilityNames[int(type)]);
    economy(false);
    return true;
  }
  bool Simulation::cancelBuilding(size_t index){
    if(index>=state.buildingQueue.size())return false;
    state.buildingQueue.erase(state.buildingQueue.begin()+std::ptrdiff_t(index));
    log("Manufacture cancelled; capacity redirected");
    economy(false);
    return true;
  }
  bool Simulation::build(int region,Facility type){
    if(region<0||region>=int(state.regions.size()))return false;
    return placeBuilding(type,region,state.regions[region].center);
  }
  double Simulation::buildingIncome(Facility type) const { return type==Facility::Factory?2:type==Facility::IndustrialPark?3:0; }
  double Simulation::regionIncome(int region) const {
    if(region<0||region>=int(state.regions.size()))return 0;
    const auto& r=state.regions[region];
    double output=std::max(0.1,r.development)*0.5;
    for(const auto& site:state.sites)if(site.region==region&&!site.destroyed&&site.enabled&&site.remaining<=0)output+=buildingIncome(site.type);
    return output*(1+0.06*std::count(state.completed.begin()+70,state.completed.begin()+84,true));
  }
  bool Simulation::connect(int,int){return false;}
  bool Simulation::upgradeRoute(size_t){return false;}
  bool Simulation::recruit(){return false;}
  bool Simulation::declareWar(){
    if(state.player<0||state.war||state.victory||state.defeat)return false;
    state.war=true;
    log("War declared. Move forces to hostile towns and strategic objectives.");
    return true;
  }
  bool Simulation::attack(int){return false;}
  void Simulation::economy(bool advance){
    state.outputRate=0;state.researchRate=0;state.productionRate=0;
    state.income=state.upkeep=state.power=state.powerDemand=0;
    state.money=0;state.delta.fill(0);
    for(size_t i=0;i<state.regions.size();++i)if(state.regions[i].owner==state.player)state.outputRate+=regionIncome(int(i));
    double labs=0;
    for(auto& site:state.sites){
      site.efficiency=0;
      if(!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player){site.efficiency=1;if(site.type==Facility::Laboratory)labs+=1;}
    }
    std::vector<double*> jobs;
    for(auto& site:state.sites)if(productionSiteActive(site))jobs.push_back(&site.remaining);
    for(auto& unit:state.units)if(productionUnitActive(unit))jobs.push_back(&unit.remaining);
    bool warheadProduction=strategicProductionActive();
    if(warheadProduction)jobs.push_back(&state.warheadWork);
    double buildingRemaining=state.buildingQueue.empty()?0:state.buildingQueue.front().total-state.buildingQueue.front().progress;
    if(!state.buildingQueue.empty())jobs.push_back(&buildingRemaining);
    bool researching=!state.research.empty()&&labs>0;
    double fraction=std::clamp(state.researchBudget,0.0,1.0);
    if(!researching)fraction=0;
    // Idle allocations redirect; an explicit zero research share still pauses research when production is active.
    if(jobs.empty()&&researching)fraction=1;
    state.researchRate=researching?state.outputRate*fraction*(1+0.08*labs)*(1+0.08*std::count(state.completed.begin()+84,state.completed.begin()+98,true)):0;
    state.productionRate=state.outputRate*(1-fraction);
    if(!advance)return;
    std::vector<size_t> manufacturing;for(size_t i=0;i<state.units.size();++i)if(productionUnitActive(state.units[i]))manufacturing.push_back(i);
    double remaining=state.productionRate*simulationTickSeconds*0.1;
    while(!jobs.empty()&&remaining>1e-9){
      // Advance all active streams to the next completion, then start the next
      // FIFO item before redistributing the remaining national capacity.
      double share=remaining/jobs.size();
      for(double* work:jobs)share=std::min(share,*work);
      for(double* work:jobs)*work-=share;
      remaining-=share*jobs.size();
      jobs.erase(std::remove_if(jobs.begin(),jobs.end(),[](double* work){return *work<=1e-9;}),jobs.end());
      if(!state.buildingQueue.empty()){
        auto& building=state.buildingQueue.front();
        building.progress=std::clamp(building.total-buildingRemaining,0.0,building.total);
        if(buildingRemaining<=1e-9){
          ++state.buildingInventory[int(building.type)];
          log(std::string("Ready to place: ")+facilityNames[int(building.type)]);
          state.buildingQueue.pop_front();
          if(!state.buildingQueue.empty()){
            buildingRemaining=state.buildingQueue.front().total-state.buildingQueue.front().progress;
            jobs.push_back(&buildingRemaining);
          }
        }
      }
      if(warheadProduction&&state.warheadsQueued>0&&state.warheadWork<=1e-9){
        state.stock[int(Resource::Warheads)]+=1;--state.warheadsQueued;
        state.warheadWork=state.warheadsQueued>0?1200:0;
        log("Strategic warhead production complete");
        if(state.warheadsQueued>0)jobs.push_back(&state.warheadWork);
      }
      if(share<=1e-9&&jobs.empty())break;
    }
    for(size_t i:manufacturing)if(state.units[i].remaining<=1e-9){state.units[i].remaining=0;log(std::string("Ready to deploy: ")+unitNames[int(state.units[i].type)]);}
  }
  void Simulation::researchTick(){
    if(state.research.empty())return;
    auto& j=state.research.front();
    auto& t=techs[j.tech];
    j.progress=std::min(j.cost,j.progress+state.researchRate*simulationTickSeconds*0.4*(t.field==state.specialization?1.35:1));
    if(j.progress>=j.cost){
      state.completed[j.tech]=true;
      state.levels[int(t.field)]=std::max(state.levels[int(t.field)],t.tier+1);
      log("Research complete: "+t.name);
      state.research.pop_front();
    }
  }
  void Simulation::tick(){
    if(state.player<0||state.victory||state.defeat)return;
    ++state.day;
    auto before=state.stock;
    economy();
    researchTick();
    unitsTick();
    combat();
    for(int i=0;i<resourceCount;++i)state.delta[i]=state.stock[i]-before[i];
  }
  bool Simulation::produceWarhead(){
    if(state.player<0||state.victory||state.defeat||!hasTech(59)||!hasTech(87)||state.stock[6]+state.warheadsQueued>=20)return false;
    bool factory=false,lab=false;
    for(const auto& site:state.sites)if(!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player){factory|=site.type==Facility::Factory;lab|=site.type==Facility::Laboratory;}
    if(!factory||!lab)return false;
    if(state.warheadsQueued++==0)state.warheadWork=1200;
    log("Strategic warhead manufacture queued");
    return true;
  }
  bool Simulation::launchNuclear(int region){
    if(state.victory||state.defeat||!state.war||state.stock[6]<1||!hasTech(60)||region<0||region>=int(state.regions.size())||state.regions[region].owner!=state.enemy)return false;
    bool ready=false;
    for(const auto& site:state.sites)if(site.type==Facility::Silo&&!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player)ready=true;
    if(!ready)return false;
    state.stock[6]-=1;state.nuclearUsed=true;
    for(auto& site:state.sites)if(site.region==region){site.destroyed=true;site.enabled=false;}
    for(auto& unit:state.units){if(unit.region==region)unit.health=0;else if(unit.owner==state.player)unit.health*=0.45;unit.moving=false;unit.path.clear();unit.waypoints.clear();unit.pendingDestinations.clear();}
    state.stability=std::max(0.0,state.stability-65);
    for(auto& site:state.sites)if(state.regions[site.region].owner==state.player&&site.type!=Facility::Depot)site.enabled=false;
    state.target=-1;state.defeat=true;state.war=false;
    log("Scripted second strike: industry devastated. Mutual destruction ends the campaign.");
    return true;
  }
  void Simulation::save(const std::string& path) const {
    if(state.player<0)throw std::runtime_error("No campaign to save");
    Object o;
    auto n=[&](const char* k,double v){
      o[k]=num(v);
    };
    n("version",6);
    n("simple",state.simpleEconomy);
    o["player"]=str(world_->countries[state.player].code);
    o["enemy"]=str(world_->countries[state.enemy].code);
    n("day",state.day);
    n("money",state.money);
    n("stability",state.stability);
    n("brigades",state.brigades);
    n("budget",state.researchBudget);
    n("focus",int(state.specialization));
    n("target",state.target);
    n("war",state.war);
    n("victory",state.victory);
    n("defeat",state.defeat);
    n("nuclear",state.nuclearUsed);
    n("warheadsQueued",state.warheadsQueued);
    n("warheadWork",state.warheadWork);
    Array stock;
    for(double v:state.stock)stock.push_back(num(v));
    o["stock"]=Json(stock);
    Array done;
    for(bool v:state.completed)done.push_back(num(v));
    o["completed"]=Json(done);
    Array regions;
    for(auto& r:state.regions){
      Object v;
      v["owner"]=str(world_->countries[r.owner].code);
      v["capture"]=num(r.capture);
      v["roads"]=num(r.roads);
      regions.push_back(Json(v));
    }
    o["regions"]=Json(regions);
    Array settlements;
    for(const auto& city:state.settlements){Object v;v["name"]=str(city.name);v["x"]=num(city.position.x);v["y"]=num(city.position.y);v["population"]=num(city.population);v["region"]=num(city.region);v["owner"]=str(world_->countries[city.owner].code);v["capture"]=num(city.capture);settlements.push_back(Json(v));}
    o["settlements"]=Json(settlements);
    Array sites;
    for(auto& s:state.sites){
      Object v;
      v["region"]=num(s.region);
      v["type"]=num(int(s.type));
      v["remaining"]=num(s.remaining);
      v["enabled"]=num(s.enabled);
      v["x"]=num(s.position.x);v["y"]=num(s.position.y);v["destroyed"]=num(s.destroyed);
      sites.push_back(Json(v));
    }
    o["sites"]=Json(sites);
    Array buildings;
    for(const auto& job:state.buildingQueue){Object v;v["type"]=num(int(job.type));v["progress"]=num(job.progress);v["total"]=num(job.total);buildings.push_back(Json(v));}
    o["buildingQueue"]=Json(buildings);
    Array ready;for(int count:state.buildingInventory)ready.push_back(num(count));o["buildingInventory"]=Json(ready);
    Array units;for(auto& u:state.units){Object v;v["owner"]=str(world_->countries[u.owner].code);v["region"]=num(u.region);v["target"]=num(u.target);v["source"]=num(u.sourceSite);v["type"]=num(int(u.type));v["x"]=num(u.position.x);v["y"]=num(u.position.y);v["dx"]=num(u.destination.x);v["dy"]=num(u.destination.y);v["health"]=num(u.health);v["remaining"]=num(u.remaining);v["total"]=num(u.total);v["moving"]=num(u.moving);v["deployed"]=num(u.deployed);Array path;for(int r:u.path)path.push_back(num(r));v["path"]=Json(path);Array waypoints;for(Point p:u.waypoints){Array xy{num(p.x),num(p.y)};waypoints.push_back(Json(xy));}v["waypoints"]=Json(waypoints);Array pending;for(Point p:u.pendingDestinations){Array xy{num(p.x),num(p.y)};pending.push_back(Json(xy));}v["pendingDestinations"]=Json(pending);units.push_back(Json(v));}o["units"]=Json(units);
    Array routes;
    for(auto& r:state.routes){
      Object v;
      v["a"]=num(r.a);
      v["b"]=num(r.b);
      v["capacity"]=num(r.capacity);
      v["enabled"]=num(r.enabled);
      routes.push_back(Json(v));
    }
    o["routes"]=Json(routes);
    Array queue;
    for(auto& r:state.research){
      Object v;
      v["tech"]=num(r.tech);
      v["progress"]=num(r.progress);
      v["cost"]=num(r.cost);
      queue.push_back(Json(v));
    }
    o["queue"]=Json(queue);
    Array events;
    for(auto& e:state.events)events.push_back(str(e));
    o["events"]=Json(events);
    std::filesystem::path destination(path);
    if(destination.has_parent_path())std::filesystem::create_directories(destination.parent_path());
    std::string temporary=path+".tmp";
    {
      std::ofstream f(temporary);
      if(!f)throw std::runtime_error("Cannot write save");
      f<<Json(o).serialize(true);
      f.flush();
      if(!f)throw std::runtime_error("Save write failed");
    }
    // Keep the prior save until the new snapshot is fully written (Windows-safe replacement).
    std::string backup=path+".bak";
    std::error_code ec;
    std::filesystem::remove(backup,ec);
    bool existed=std::filesystem::exists(destination);
    if(existed)std::filesystem::rename(path,backup);
    try{
      std::filesystem::rename(temporary,path);
    }
    catch(...){
      if(existed)std::filesystem::rename(backup,path);
      throw;
    }
  }
  void Simulation::load(const std::string& path){
    auto o=readJson(path);
    auto bounded=[&](const Json& j,const char* key,double lo,double hi){
      double v=number(j,key);
      if(!std::isfinite(v)||v<lo||v>hi)throw std::runtime_error(std::string("Invalid save: ")+key);
      return v;
    };
    auto integer=[&](const Json& j,const char* key,int lo,int hi){
      double v=bounded(j,key,lo,hi);
      if(std::floor(v)!=v)throw std::runtime_error("Non-integral save index");
      return int(v);
    };
    int version=integer(o,"version",1,6);
    int player=world_->find(string(o,"player")),enemy=world_->find(string(o,"enemy"));
    Simulation candidate(*world_);
    candidate.start(player,Field(integer(o,"focus",0,7)),version>=2?bool(integer(o,"simple",0,1)):false);
    auto& c=candidate.state;
    if(enemy!=c.enemy)throw std::runtime_error("Save opponent differs from world data");
    c.day=integer(o,"day",0,365*300);
    c.money=bounded(o,"money",0,1e12);
    c.stability=bounded(o,"stability",0,100);
    c.brigades=bounded(o,"brigades",0,version>=2?120:60);
    c.researchBudget=bounded(o,"budget",0,1);
    c.target=integer(o,"target",-1,int(c.regions.size())-1);
    c.war=integer(o,"war",0,1);
    c.victory=integer(o,"victory",0,1);
    c.defeat=integer(o,"defeat",0,1);
    c.nuclearUsed=integer(o,"nuclear",0,1);
    if(version>=3&&o.contains("warheadsQueued")){
      c.warheadsQueued=integer(o,"warheadsQueued",0,20);
      c.warheadWork=bounded(o,"warheadWork",0,1200);
      if((c.warheadsQueued>0)!=(c.warheadWork>0))throw std::runtime_error("Invalid strategic production queue");
    }
    auto& stock=asArray(field(o,"stock"));
    if(stock.size()!=resourceCount)throw std::runtime_error("Invalid inventory");
    for(int i=0;i<resourceCount;++i){
      double v=asNumber(stock[i]);
      if(!std::isfinite(v)||v<0||v>(i==6?20:20000))throw std::runtime_error("Invalid stockpile");
      c.stock[i]=v;
    }
    if(std::floor(c.stock[6])!=c.stock[6]||c.stock[6]+c.warheadsQueued>20)throw std::runtime_error("Invalid strategic stockpile");
    auto& done=asArray(field(o,"completed"));
    if(done.size()!=techs.size())throw std::runtime_error("Invalid technology tree");
    c.levels.fill(0);
    for(size_t i=0;i<done.size();++i){
      double flag=asNumber(done[i]);
      if(flag!=0&&flag!=1)throw std::runtime_error("Invalid research flag");
      c.completed[i]=flag==1;
      if(version<6&&i>=14&&i<42)c.completed[i]=false;
      if(c.completed[i]){
        if(version>=6)for(int p:techs[i].prerequisites)if(!asNumber(done[p]))throw std::runtime_error("Broken research prerequisite");
        c.levels[int(techs[i].field)]=std::max(c.levels[int(techs[i].field)],techs[i].tier+1);
      }
    }
    if(version<6){
      // Preserve ground/industry/strategic achievements, filling the changed
      // prerequisite ancestry. Former aviation/fleet research grants no new
      // defence or sensor equipment.
      if(asNumber(done[2]))c.completed[7]=true;
      if(asNumber(done[1]))c.completed[4]=true;
      bool changed=true;while(changed){changed=false;for(size_t i=0;i<c.completed.size();++i)if(c.completed[i])for(int p:techs[i].prerequisites)if(!c.completed[p]){c.completed[p]=true;changed=true;}}
      c.levels.fill(0);for(size_t i=0;i<c.completed.size();++i)if(c.completed[i])c.levels[int(techs[i].field)]=std::max(c.levels[int(techs[i].field)],techs[i].tier+1);
    }
    auto& regions=asArray(field(o,"regions"));
    if(regions.size()!=c.regions.size())throw std::runtime_error("Invalid region count");
    for(size_t i=0;i<regions.size();++i){
      int owner=world_->find(string(regions[i],"owner"));
      if(owner!=c.player&&owner!=c.enemy)throw std::runtime_error("Invalid owner");
      c.regions[i].owner=owner;
      c.regions[i].capture=bounded(regions[i],"capture",0,1);
      c.regions[i].roads=version>=2?integer(regions[i],"roads",0,5):0;
    }
    if(version>=3){
      c.settlements.clear();
      const auto& cities=asArray(field(o,"settlements"));
      if(cities.size()>10000)throw std::runtime_error("Too many settlements");
      std::vector<int> objectives(c.regions.size());
      for(const auto& city:cities){Settlement v;v.name=string(city,"name");if(v.name.empty()||v.name.size()>200)throw std::runtime_error("Invalid settlement name");v.position={bounded(city,"x",-720,720),bounded(city,"y",-90,90)};v.population=bounded(city,"population",0,1e9);v.region=integer(city,"region",0,int(c.regions.size())-1);v.owner=world_->find(string(city,"owner"));if(v.owner!=c.player&&v.owner!=c.enemy)throw std::runtime_error("Invalid settlement owner");v.capture=bounded(city,"capture",0,1);++objectives[v.region];c.settlements.push_back(v);}
      for(int count:objectives)if(!count)throw std::runtime_error("Region has no capture objective");
    }else for(auto& city:c.settlements){city.owner=c.regions[city.region].owner;city.capture=c.regions[city.region].capture;}
    c.sites.clear();
    auto& sites=asArray(field(o,"sites"));
    if(sites.size()>1000)throw std::runtime_error("Too many sites");
    for(auto& s:sites){Site site{integer(s,"region",0,int(c.regions.size())-1),Facility(integer(s,"type",0,version>=2?15:9)),bounded(s,"remaining",0,120000),0,bool(integer(s,"enabled",0,1))};site.position=version>=2?Point{bounded(s,"x",-720,720),bounded(s,"y",-90,90)}:c.regions[site.region].center;site.destroyed=version>=2?bool(integer(s,"destroyed",0,1)):false;c.sites.push_back(site);}
    if(version>=4){
      const auto& buildings=asArray(field(o,"buildingQueue"));
      if(buildings.size()>64)throw std::runtime_error("Construction queue too long");
      for(const auto& v:buildings){Facility type=Facility(integer(v,"type",0,15));if(version<6&&(type==Facility::Airbase||type==Facility::Shipyard))continue;if(!activeFacility(type))throw std::runtime_error("Invalid building type");double total=bounded(v,"total",candidate.buildCost(type),candidate.buildCost(type));c.buildingQueue.push_back({type,bounded(v,"progress",0,total),total});}
      const auto& ready=asArray(field(o,"buildingInventory"));if(ready.size()!=16)throw std::runtime_error("Invalid ready inventory");
      for(int i=0;i<16;++i){double count=asNumber(ready[i]);if(!std::isfinite(count)||count<0||count>1000||std::floor(count)!=count||(!activeFacility(Facility(i))&&count&&!(version<6&&(i==8||i==11))))throw std::runtime_error("Invalid ready building count");c.buildingInventory[i]=int(count);int reserved=int(count);for(const auto& job:c.buildingQueue)if(int(job.type)==i)++reserved;if(reserved>1000)throw std::runtime_error("Too many reserved buildings");}
    }
    if(version>=2){
      c.units.clear();
      const auto& units=asArray(field(o,"units"));
      if(units.size()>1000)throw std::runtime_error("Too many units");
      for(const auto& v:units){
        Unit u;
        u.owner=world_->find(string(v,"owner"));
        if(u.owner!=c.player&&u.owner!=c.enemy)throw std::runtime_error("Invalid unit owner");
        u.region=integer(v,"region",0,int(c.regions.size())-1);
        u.target=integer(v,"target",-1,int(c.regions.size())-1);
        u.sourceSite=integer(v,"source",-1,int(c.sites.size())-1);
        u.type=UnitType(integer(v,"type",0,8));
        u.position={bounded(v,"x",-720,720),bounded(v,"y",-90,90)};
        u.destination={bounded(v,"dx",-720,720),bounded(v,"dy",-90,90)};
        u.health=bounded(v,"health",0,100);
        u.remaining=bounded(v,"remaining",0,120000);
        u.deployed=version>=6?bool(integer(v,"deployed",0,1)):u.remaining<=0;
        u.total=bounded(v,"total",0,120000);
        u.moving=bool(integer(v,"moving",0,1));
        if(u.remaining>u.total||(u.moving&&(u.remaining>0||u.health<=0||!u.deployed)))throw std::runtime_error("Invalid unit readiness");
        if(version>=6&&u.remaining>0&&u.deployed)throw std::runtime_error("Manufacturing unit cannot be deployed");
        if(u.remaining>0&&u.sourceSite<0)throw std::runtime_error("Missing unit producer");
        const auto& path=asArray(field(v,"path"));
        if(path.size()>1000)throw std::runtime_error("Unit route too long");
        for(const auto& r:path){
          double n=asNumber(r);
          if(!std::isfinite(n)||n<0||n>=c.regions.size()||std::floor(n)!=n)throw std::runtime_error("Invalid unit path");
          u.path.push_back(int(n));
        }
        if(version>=3&&v.contains("waypoints")){
          const auto& points=asArray(field(v,"waypoints"));
          if(points.size()>1000)throw std::runtime_error("Unit waypoints too long");
          for(const auto& point:points){
            const auto& xy=asArray(point);
            if(xy.size()!=2)throw std::runtime_error("Invalid unit waypoint");
            Point p{asNumber(xy[0]),asNumber(xy[1])};
            if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>720||std::abs(p.y)>90)throw std::runtime_error("Invalid unit waypoint");
            u.waypoints.push_back(p);
          }
        }
        if(v.contains("pendingDestinations")){
          const auto& points=asArray(field(v,"pendingDestinations"));
          if(points.size()>64)throw std::runtime_error("Unit order queue too long");
          for(const auto& point:points){
            const auto& xy=asArray(point);
            if(xy.size()!=2)throw std::runtime_error("Invalid queued destination");
            Point p{asNumber(xy[0]),asNumber(xy[1])};
            if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>720||std::abs(p.y)>90)throw std::runtime_error("Invalid queued destination");
            u.pendingDestinations.push_back(p);
          }
          if(!u.pendingDestinations.empty()&&(!u.moving||u.health<=0||u.remaining>0||!u.deployed))throw std::runtime_error("Invalid queued unit readiness");
        }
        c.units.push_back(std::move(u));
      }
    }
    c.routes.clear();
    auto& routes=asArray(field(o,"routes"));
    if(routes.size()>2500)throw std::runtime_error("Too many routes");
    for(auto& r:routes){
      Route v{
        integer(r,"a",0,int(c.regions.size())-1),integer(r,"b",0,int(c.regions.size())-1),bounded(r,"capacity",1,1e6),bool(integer(r,"enabled",0,1))
      };
      if(v.a==v.b)throw std::runtime_error("Invalid self route");
      c.routes.push_back(v);
    }
    c.research.clear();
    std::set<int> queued;
    auto& queue=asArray(field(o,"queue"));
    if(queue.size()>12)throw std::runtime_error("Research queue too long");
    for(auto& j:queue){
      int t=integer(j,"tech",0,int(techs.size())-1);
      if(version<6&&((t>=14&&t<42)||!candidate.available(t)))continue;
      if(!candidate.available(t)||!queued.insert(t).second)throw std::runtime_error("Invalid research queue");
      double cost=version>=3?bounded(j,"cost",techs[t].cost,1e9):candidate.researchCost(t);
      c.research.push_back({t,bounded(j,"progress",0,cost),cost});
    }
    c.events.clear();
    for(auto& e:asArray(field(o,"events"))){
      if(c.events.size()==80)break;
      c.events.push_back(asString(e));
    }
    if(version<3){
      for(auto& site:c.sites){if(!activeFacility(site.type)){site.destroyed=true;site.enabled=false;}else if(site.remaining>0)site.remaining=candidate.buildCost(site.type)*std::clamp(site.remaining/(site.type==Facility::Silo?120.0:35.0),0.0,1.0);}
      for(auto& unit:c.units)if(unit.remaining>0){double fraction=unit.total>0?unit.remaining/unit.total:1;unit.total=candidate.unitCost(unit.type);unit.remaining=unit.total*std::clamp(fraction,0.0,1.0);}
      candidate.log("Legacy save migrated to regional production and city objectives");
    }
    if(version<4){
      // Keep historical site indices for unit producers; each unfinished foundation
      // becomes one fabrication item, never a ready placement as well.
      for(auto& site:c.sites)if(!site.destroyed&&site.remaining>0&&c.regions[site.region].owner==c.player&&activeFacility(site.type)){
        double total=candidate.buildCost(site.type);
        c.buildingQueue.push_back({site.type,total-std::clamp(site.remaining,0.0,total),total});
        site.destroyed=true;site.enabled=false;site.remaining=0;
      }
      if(c.buildingQueue.size()>64)throw std::runtime_error("Legacy construction queue too long");
      candidate.log("Construction migrated to manufacture and ready placement");
    }
    c.money=0;c.simpleEconomy=true;
    for(int i=0;i<int(Resource::Warheads);++i)c.stock[i]=0;
    // Legacy movement did not store a safe geometric route. Replan through the
    // current land/sea rules rather than resuming an obsolete straight line.
    if(version<3)for(auto& unit:c.units)if(unit.moving&&!candidate.planUnitMovement(unit,unit.destination)){unit.moving=false;unit.path.clear();unit.waypoints.clear();unit.pendingDestinations.clear();}
    for(auto& site:c.sites)if(site.type==Facility::Airbase||site.type==Facility::Shipyard){site.destroyed=true;site.enabled=false;site.remaining=0;}
    c.buildingInventory[int(Facility::Airbase)]=0;c.buildingInventory[int(Facility::Shipyard)]=0;
    c.buildingQueue.erase(std::remove_if(c.buildingQueue.begin(),c.buildingQueue.end(),[](const BuildingJob& j){return j.type==Facility::Airbase||j.type==Facility::Shipyard;}),c.buildingQueue.end());
    for(auto& u:c.units)if(!groundUnit(u.type)){u.health=0;u.remaining=0;u.deployed=false;u.moving=false;u.path.clear();u.waypoints.clear();u.pendingDestinations.clear();}
    if(version<6){candidate.log("Version 6 migration: ground research ancestry migrated; air and naval assets and research retired");for(auto& u:c.units)if(u.moving&&!candidate.planUnitMovement(u,u.destination)){u.moving=false;u.path.clear();u.waypoints.clear();u.pendingDestinations.clear();}}
    candidate.economy(false);
    c.lastAutosave=c.day;
    candidate.log("Campaign restored");
    state=std::move(c);
  }
}
