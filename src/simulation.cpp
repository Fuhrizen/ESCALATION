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
  const std::array<const char*,8> fieldNames={"Land warfare","Aircraft","Naval warfare","Special operations","Missiles","Industry","Science","Support"};
  const std::array<const char*,16> facilityNames={"Mine","Oil field","Steelworks","Refinery","Arms plant","Power station","Research centre","Command centre","Shipyard","Missile site","Army base","Airbase","Radar station","Farm","Industrial district","Commercial district"};
  static const char* names[8][14]={ {"Combined arms","Motorised infantry","Armoured divisions","Main battle tanks","Mechanised doctrine","Reactive armour","Digital command","Precision artillery","Networked brigades","Active protection","Autonomous support","Robotic formations","Distributed armour","Adaptive battle groups"}, {"Biplane reconnaissance","Monoplane fighters","Strategic bombers","Jet propulsion","Supersonic aviation","Airborne early warning","Multirole fighters","Stealth aircraft","Uncrewed reconnaissance","Precision air support","Collaborative drones","Adaptive propulsion","Distributed air combat","Autonomous air wings"}, {"Fleet logistics","Carrier aviation","Radar escorts","Nuclear propulsion","Missile destroyers","Attack submarines","Aegis screening","Quiet propulsion","Littoral operations","Stealth surface ships","Uncrewed vessels","Distributed fleets","Autonomous escorts","Integrated maritime command"}, {"Raiding doctrine","Signals interception","Airborne operations","Long range patrols","Special forces","Electronic warfare","Counterterror teams","Satellite reconnaissance","Joint special operations","Cyber reconnaissance","Uncrewed infiltration","Decision support","Distributed intelligence","Adaptive reconnaissance"}, {"Rocket experiments","Liquid fuel rockets","Guided missiles","Nuclear deterrence","Intercontinental missiles","Multiple warheads","Mobile launchers","Precision guidance","Missile defence","Hypersonic research","Boost glide systems","Space surveillance","Resilient deterrence","Strategic command networks"}, {"Industrial electrification","Assembly lines","Wartime conversion","Automation","Containerisation","Computerised industry","Flexible manufacturing","Global supply chains","Lean production","Smart factories","Industrial robotics","Circular production","Fusion pilot grids","Autonomous industry"}, {"National laboratories","Research institutes","Applied science","Computing centres","Integrated circuits","Scientific computing","Supercomputers","Networked laboratories","Open research networks","Machine learning tools","Advanced simulation","Materials discovery","Quantum research","Accelerated discovery"}, {"Rail mobilisation","Truck logistics","Forward depots","Strategic airlift","Intermodal freight","Supply planning","Digital inventories","Satellite tracking","Predictive routing","Resilient distribution","Autonomous convoys","Adaptive depots","Distributed reserves","Self organising logistics"}};
  std::vector<Tech> makeTechTree(){
    std::vector<Tech> result;
    for(int f=0;f<8;++f)for(int tier=0;tier<14;++tier){
      Tech t;
      t.id=std::to_string(f)+"-"+std::to_string(tier);
      t.name=names[f][tier];
      t.field=Field(f);
      t.tier=tier;
      t.year=1920+tier*10;
      t.cost=400+double(tier)*220;
      t.effect=f==0?"+12% ground weapon strength per highest researched tier":f==1?"Upgrade aircraft weapons and unlock advanced squadrons":f==2?"Upgrade naval weapons and unlock shipyards and sea transport":f==3?"Upgrade special forces and unlock reconnaissance formations":f==4?"Upgrade missile weapons; unlock strategic production and delivery":f==5?"+6% regional production per highest researched tier":f==6?"+8% research output per highest researched tier": "+3% formation movement speed per highest researched tier";
      if(f==4&&tier==3)t.effect="Unlock expensive nuclear warhead production; requires Computing centres";
      if(f==4&&tier==4)t.effect="Unlock nuclear delivery";
      if(f==0&&tier==0)t.effect="Unlock armoured brigades; upgrade ground combat";
      if(f==1&&tier==0)t.effect="Unlock fighters and airbases";
      if(f==2&&tier==0)t.effect="Unlock shipyards, destroyers and amphibious supply";
      // Two parallel investment paths share a foundation, so one specialist path
      // can advance without buying every upgrade in its sibling path.
      if(tier>0)t.prerequisites.push_back(f*14+std::max(0,tier-2));
      if(f==4&&tier==3)t.prerequisites.push_back(6*14+3);
      if((f==1||f==2)&&tier==3)t.prerequisites.push_back(6*14+3);
      if(f==3&&tier==4)t.prerequisites.push_back(0*14+3);
      if(f==5&&tier==6)t.prerequisites.push_back(6*14+5);
      if(f==7&&tier==4)t.prerequisites.push_back(5*14+3);
      if(f==6&&tier==4)t.prerequisites.push_back(5*14+3);
      result.push_back(t);
    }
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
      std::vector<Facility> starting={Facility::IndustrialPark,Facility::Factory,Facility::Laboratory,Facility::ArmyBase,Facility::Airbase};
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
    log("Command established. Allocate regional output between research and military production.");
    if(world_->countries[nation].neighbours.empty())log("Island start: nearest nation is a maritime rival. Naval research and a supplied shipyard enable a landing.");
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
    return true;
  }
  void Simulation::removeResearch(size_t index){
    if(index<state.research.size())state.research.erase(state.research.begin()+std::ptrdiff_t(index));
  }
  static bool activeFacility(Facility f){
    return f==Facility::Factory||f==Facility::Laboratory||f==Facility::Depot||f==Facility::Shipyard||f==Facility::Silo||f==Facility::ArmyBase||f==Facility::Airbase||f==Facility::Radar||f==Facility::IndustrialPark;
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
    if(type==Facility::Airbase&&state.levels[1]<1)return "Requires Biplane reconnaissance";
    if(type==Facility::Radar&&state.levels[1]<2)return "Requires Monoplane fighters";
    if(type==Facility::Shipyard&&state.levels[2]<1)return "Requires Fleet logistics";
    if(type==Facility::Silo&&state.levels[4]<2)return "Requires Liquid fuel rockets";
    int count=0;
    for(auto& s:state.sites)if(s.region==region&&!s.destroyed)++count;
    if(count>=8)return "Region has reached its building capacity";
    return "";
  }
  bool Simulation::build(int region,Facility type){
    if(!buildReason(region,type).empty())return false;
    state.sites.push_back({region,type,buildCost(type),0,true});
    state.sites.back().position=state.regions[region].center;
    log(std::string("Construction queued: ")+facilityNames[int(type)]);
    return true;
  }
  double Simulation::buildingIncome(Facility type) const { return type==Facility::Factory?2:type==Facility::IndustrialPark?3:0; }
  double Simulation::regionIncome(int region) const {
    if(region<0||region>=int(state.regions.size()))return 0;
    const auto& r=state.regions[region];
    double output=std::max(0.1,r.development)*0.5;
    for(const auto& site:state.sites)if(site.region==region&&!site.destroyed&&site.enabled&&site.remaining<=0)output+=buildingIncome(site.type);
    return output*(1+0.06*state.levels[5]);
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
    double labs=0;bool factory=false;
    for(auto& site:state.sites){
      site.efficiency=0;
      if(!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player){site.efficiency=1;if(site.type==Facility::Laboratory)labs+=1;if(site.type==Facility::Factory)factory=true;}
    }
    std::vector<double*> jobs;
    for(auto& site:state.sites)if(!site.destroyed&&site.enabled&&site.remaining>0&&state.regions[site.region].owner==state.player)jobs.push_back(&site.remaining);
    for(auto& unit:state.units)if(unit.owner==state.player&&unit.health>0&&unit.remaining>0&&unit.sourceSite>=0&&unit.sourceSite<int(state.sites.size())){
      const auto& source=state.sites[unit.sourceSite];
      if(!source.destroyed&&source.enabled&&source.remaining<=0&&state.regions[source.region].owner==state.player)jobs.push_back(&unit.remaining);
    }
    bool warheadProduction=state.warheadsQueued>0&&factory&&labs>0;
    if(warheadProduction)jobs.push_back(&state.warheadWork);
    bool researching=!state.research.empty()&&labs>0;
    double fraction=std::clamp(state.researchBudget,0.0,1.0);
    if(!researching)fraction=0;
    // Idle allocations redirect; an explicit zero research share still pauses research when production is active.
    if(jobs.empty()&&researching)fraction=1;
    state.researchRate=researching?state.outputRate*fraction*(1+0.08*labs)*(1+0.08*state.levels[6]):0;
    state.productionRate=state.outputRate*(1-fraction);
    if(!advance)return;
    double remaining=state.productionRate;
    while(!jobs.empty()&&remaining>1e-9){
      double share=remaining/jobs.size(),spent=0;
      for(double* work:jobs){double amount=std::min(*work,share);*work-=amount;spent+=amount;}
      remaining-=spent;
      jobs.erase(std::remove_if(jobs.begin(),jobs.end(),[](double* work){return *work<=1e-9;}),jobs.end());
      if(spent<=1e-9)break;
    }
    if(warheadProduction&&state.warheadWork<=1e-9){
      state.stock[int(Resource::Warheads)]+=1;--state.warheadsQueued;
      state.warheadWork=state.warheadsQueued>0?1200:0;
      log("Strategic warhead production complete");
    }
  }
  void Simulation::researchTick(){
    if(state.research.empty())return;
    auto& j=state.research.front();
    auto& t=techs[j.tech];
    j.progress+=state.researchRate*(t.field==state.specialization?1.35:1);
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
    if(state.player<0||state.victory||state.defeat||state.levels[4]<4||state.levels[6]<4||state.stock[6]+state.warheadsQueued>=20)return false;
    bool factory=false,lab=false;
    for(const auto& site:state.sites)if(!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player){factory|=site.type==Facility::Factory;lab|=site.type==Facility::Laboratory;}
    if(!factory||!lab)return false;
    if(state.warheadsQueued++==0)state.warheadWork=1200;
    log("Strategic warhead queued: 1,200 shared production work");
    return true;
  }
  bool Simulation::launchNuclear(int region){
    if(state.victory||state.defeat||!state.war||state.stock[6]<1||state.levels[4]<5||region<0||region>=int(state.regions.size())||state.regions[region].owner!=state.enemy)return false;
    bool ready=false;
    for(const auto& site:state.sites)if(site.type==Facility::Silo&&!site.destroyed&&site.enabled&&site.remaining<=0&&state.regions[site.region].owner==state.player)ready=true;
    if(!ready)return false;
    state.stock[6]-=1;state.nuclearUsed=true;
    for(auto& site:state.sites)if(site.region==region){site.destroyed=true;site.enabled=false;}
    for(auto& unit:state.units){if(unit.region==region)unit.health=0;else if(unit.owner==state.player)unit.health*=0.45;unit.moving=false;unit.path.clear();unit.waypoints.clear();}
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
    n("version",3);
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
    Array units;for(auto& u:state.units){Object v;v["owner"]=str(world_->countries[u.owner].code);v["region"]=num(u.region);v["target"]=num(u.target);v["source"]=num(u.sourceSite);v["type"]=num(int(u.type));v["x"]=num(u.position.x);v["y"]=num(u.position.y);v["dx"]=num(u.destination.x);v["dy"]=num(u.destination.y);v["health"]=num(u.health);v["remaining"]=num(u.remaining);v["total"]=num(u.total);v["moving"]=num(u.moving);Array path;for(int r:u.path)path.push_back(num(r));v["path"]=Json(path);Array waypoints;for(Point p:u.waypoints){Array xy{num(p.x),num(p.y)};waypoints.push_back(Json(xy));}v["waypoints"]=Json(waypoints);units.push_back(Json(v));}o["units"]=Json(units);
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
    int version=integer(o,"version",1,3);
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
      if(c.completed[i]){
        for(int p:techs[i].prerequisites)if(!asNumber(done[p]))throw std::runtime_error("Broken research prerequisite");
        c.levels[int(techs[i].field)]=std::max(c.levels[int(techs[i].field)],techs[i].tier+1);
      }
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
        u.total=bounded(v,"total",0,120000);
        u.moving=bool(integer(v,"moving",0,1));
        if(u.remaining>u.total||(u.moving&&(u.remaining>0||u.health<=0)))throw std::runtime_error("Invalid unit readiness");
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
    c.money=0;c.simpleEconomy=true;
    for(int i=0;i<int(Resource::Warheads);++i)c.stock[i]=0;
    // Legacy movement did not store a safe geometric route. Replan through the
    // current land/sea rules rather than resuming an obsolete straight line.
    if(version<3)for(auto& unit:c.units)if(unit.moving&&!candidate.planUnitMovement(unit,unit.destination)){unit.moving=false;unit.path.clear();unit.waypoints.clear();}
    candidate.economy(false);
    c.lastAutosave=c.day;
    candidate.log("Campaign restored");
    state=std::move(c);
  }
}
