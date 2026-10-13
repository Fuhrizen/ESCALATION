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
  const std::array<const char*,8> fieldNames={"Army","Air","Naval","Special operations","Ballistic","Economy","Research","Logistics"};
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
      t.effect=f==0?"+12% brigade strength and reduced combat attrition":f==1?"Air support improves offensive strength":f==2?"Naval support and amphibious operations improve":f==3?"Reconnaissance reduces regional defensive resistance":f==4?"Strategic deterrence readiness and silo capability":f==5?"+10% industrial output and +8% tax income":f==6?"+12% research output": "Improve strategic movement and legacy route throughput";
      if(f==4&&tier==3)t.effect="Unlock expensive nuclear warhead production; requires Computing centres";
      if(f==4&&tier==4)t.effect="Unlock nuclear delivery";
      if(f==0&&tier==0)t.effect="Unlock armoured brigades; upgrade ground combat";
      if(f==1&&tier==0)t.effect="Unlock fighters and airbases";
      if(f==2&&tier==0)t.effect="Unlock shipyards, destroyers and amphibious supply";
      if(tier>0)t.prerequisites.push_back(f*14+tier-1);
      if(f==4&&tier==3)t.prerequisites.push_back(6*14+3);
      if(f==0&&tier==2)t.prerequisites.push_back(5*14+1);
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
    nation=world_->canonical(nation);
    state=Campaign{};
    state.simpleEconomy=simpleEconomy;
    state.player=nation;
    state.enemy=world_->opponent(nation);
    if(state.enemy<0)throw std::runtime_error("No opponent available");
    state.specialization=focus;
    state.completed.resize(techs.size());
    state.stock={120,100,150,100,160,100,0};
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
      std::vector<Facility> starting=simpleEconomy?std::vector<Facility>{Facility::PowerPlant,Facility::Farm,Facility::Commercial,Facility::IndustrialPark,Facility::Factory,Facility::Laboratory,Facility::ArmyBase,Facility::Airbase}:std::vector<Facility>{Facility::PowerPlant,Facility::Mine,Facility::OilWell,Facility::Steelworks,Facility::Refinery,Facility::Factory,Facility::Laboratory};
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
    log("Command established. Build income, research and regional infrastructure before committing forces.");
    if(world_->countries[nation].neighbours.empty())log("Island start: nearest nation is a maritime rival. Naval research and a supplied shipyard enable a landing.");
    economy();
    state.delta.fill(0);
  }
  double Simulation::connection(int region) const{
    if(region<0||region>=int(state.regions.size())||state.regions[region].owner!=state.player)return 0;
    if(state.simpleEconomy)return 8*(1+0.2*state.regions[region].roads);
    std::vector<double> best(state.regions.size());
    std::priority_queue<std::pair<double,int>> q;
    best[state.capital]=1e9;
    q.push({best[state.capital],state.capital});
    while(!q.empty()){
      auto [capacity,u]=q.top();
      q.pop();
      if(capacity<best[u])continue;
      for(auto& route:state.routes){
        if(!route.enabled)continue;
        int v=route.a==u?route.b:route.b==u?route.a:-1;
        if(v<0||state.regions[v].owner!=state.player)continue;
        double c=std::min(capacity,route.capacity*(1+0.15*state.levels[7]));
        if(c>best[v]){
          best[v]=c;
          q.push({c,v});
        }
      }
    }
    return best[region];
  }
  std::string Simulation::lockReason(int tech) const {
    if(tech<0||tech>=int(techs.size()))return "Invalid technology";
    if(state.completed.empty())return "Start a campaign";
    if(state.completed[tech])return "Already researched";
    if(techs[tech].year>year())return "Available in "+std::to_string(techs[tech].year);
    for(int p:techs[tech].prerequisites)if(!state.completed[p])return "Requires "+techs[p].name;
    for(auto& j:state.research)if(j.tech==tech)return "Already queued";
    return "";
  }
  bool Simulation::available(int tech) const{
    return lockReason(tech).empty();
  }
  bool Simulation::queueResearch(int tech){
    if(!available(tech)||state.research.size()>=12)return false;
    state.research.push_back({tech,0});
    log("Queued research: "+techs[tech].name);
    return true;
  }
  void Simulation::removeResearch(size_t index){
    if(index<state.research.size())state.research.erase(state.research.begin()+std::ptrdiff_t(index));
  }
  double Simulation::buildCost(Facility type) const {
    static double cost[]={800,1000,1400,1400,1800,1200,2000,500,3500,18000,1600,2800,2200,650,2200,1500};
    int i=int(type);
    return i>=0&&i<16?cost[i]:1e30;
  }
  std::string Simulation::buildReason(int region,Facility type) const {
    if(region<0||region>=int(state.regions.size())||state.regions[region].owner!=state.player)return "Build on controlled land";
    if(int(type)<0||int(type)>=16)return "Invalid facility";
    if(state.money<buildCost(type))return "Insufficient treasury";
    if(!state.simpleEconomy&&state.stock[2]<30)return "Requires 30 steel";
    if(type==Facility::Airbase&&state.levels[1]<1)return "Research Biplane reconnaissance first";
    if(type==Facility::Radar&&state.levels[1]<2)return "Research Monoplane fighters first";
    if(type==Facility::Shipyard&&state.levels[2]<1)return "Research Fleet logistics first";
    if(type==Facility::Silo&&state.levels[4]<(state.simpleEconomy?2:5))return "Research Liquid fuel rockets first";
    int count=0;
    for(auto& s:state.sites)if(s.region==region&&!s.destroyed)++count;
    if(count>=(state.simpleEconomy?8:4))return "Region has reached its building capacity";
    return "";
  }
  bool Simulation::build(int region,Facility type){
    if(!buildReason(region,type).empty())return false;
    state.money-=buildCost(type);
    if(!state.simpleEconomy)state.stock[2]-=30;
    state.sites.push_back({region,type,type==Facility::Silo?120.0:35.0,0,true});
    state.sites.back().position=state.regions[region].center;
    log(std::string("Construction ordered: ")+facilityNames[int(type)]);
    return true;
  }
  bool Simulation::connect(int a,int b){
    if(a==b||a<0||b<0||a>=int(state.regions.size())||b>=int(state.regions.size())||state.regions[a].owner!=state.player||state.regions[b].owner!=state.player||state.money<250||state.stock[2]<10)return false;
    for(auto& r:state.routes)if((r.a==a&&r.b==b)||(r.b==a&&r.a==b))return false;
    state.money-=250;
    state.stock[2]-=10;
    state.routes.push_back({a,b,8,true});
    log("Supply link established");
    return true;
  }
  bool Simulation::upgradeRoute(size_t i){
    if(i>=state.routes.size()||state.regions[state.routes[i].a].owner!=state.player||state.regions[state.routes[i].b].owner!=state.player||state.money<600||state.stock[2]<20)return false;
    state.money-=600;
    state.stock[2]-=20;
    state.routes[i].capacity+=4;
    return true;
  }
  bool Simulation::recruit(){
    if(state.money<1000||state.stock[5]<20||state.brigades>=60)return false;
    state.money-=1000;
    state.stock[5]-=20;
    state.brigades+=1;
    log("Brigade equipped and mobilised");
    return true;
  }
  bool Simulation::declareWar(){
    if(state.war||state.victory||state.defeat)return false;
    state.war=true;
    log("War declared. Select a frontier region and order an offensive.");
    return true;
  }
  bool Simulation::attack(int region){
    if(!state.war||state.victory||state.defeat||region<0||region>=int(state.regions.size())||state.regions[region].owner!=state.enemy)return false;
    bool adjacent=false;
    for(int n:state.regions[region].neighbours)if(state.regions[n].owner==state.player&&connection(n)>0)adjacent=true;
    bool maritime=world_->countries[state.player].neighbours.empty()||!adjacent;
    if(maritime){
      bool shipyard=false;
      for(auto& site:state.sites)if(site.type==Facility::Shipyard&&site.remaining<=0&&site.enabled&&state.regions[site.region].owner==state.player&&connection(site.region)>0)shipyard=true;
      if(state.levels[2]<1||!shipyard)return false;
    }
    state.target=region;
    if(state.simpleEconomy)for(size_t i=0;i<state.units.size();++i)if(state.units[i].owner==state.player&&state.units[i].remaining<=0&&int(state.units[i].type)<=2)orderUnit(i,region);
    log("Offensive ordered in sector "+std::to_string(region+1));
    return true;
  }
  void Simulation::economy(){
    auto before=state.stock;
    if(state.simpleEconomy){
      state.income=20*(1+0.08*state.levels[5])*state.stability/100;state.upkeep=0;state.power=0;state.powerDemand=0;state.researchRate=0;
      for(auto& site:state.sites){site.efficiency=0;if(site.destroyed||!site.enabled||state.regions[site.region].owner!=state.player)continue;
        if(site.remaining>0){site.remaining=std::max(0.0,site.remaining-1);continue;}
        site.efficiency=1;state.upkeep+=site.type==Facility::Silo?8:1;
        double factor=(1+0.15*state.regions[site.region].roads)*(1+0.1*state.levels[5]);state.income+=buildingIncome(site.type)*factor*state.stability/100;
        if(site.type==Facility::PowerPlant)state.power+=30;
        if(site.type==Facility::Laboratory){state.researchRate+=6*(0.2+1.6*state.researchBudget)*(1+0.12*state.levels[6]);state.upkeep+=12*state.researchBudget;}
      }
      for(auto& unit:state.units)if(unit.owner==state.player&&unit.health>0)state.upkeep+=unit.remaining>0?0.25:0.6+int(unit.type)*0.1;
      state.money=std::max(0.0,state.money+state.income-state.upkeep);if(state.money<=0)state.stability=std::max(10.0,state.stability-0.1);
      state.delta.fill(0);return;
    }
    state.income=(22+std::min(40.0,std::sqrt(std::max(0.0,world_->countries[state.player].gdp))/80))*(1+0.08*state.levels[5])*state.stability/100;
    state.upkeep=state.brigades*0.65;
    state.power=0;
    state.powerDemand=0;
    state.researchRate=0;
    for(auto& s:state.sites){
      s.efficiency=0;
      if(state.regions[s.region].owner!=state.player||!s.enabled)continue;
      if(s.remaining>0){
        if(state.money>0&&connection(s.region)>0)s.remaining=std::max(0.0,s.remaining-1);
        continue;
      }
      if(s.type==Facility::PowerPlant)state.power+=18*(1+0.1*state.levels[5])*std::min(1.0,connection(s.region)/8);
      else if(s.type!=Facility::Depot)state.powerDemand+=s.type==Facility::Laboratory?4:3;
      state.upkeep+=s.type==Facility::Silo?12:1.2;
    }
    double powerFactor=state.powerDemand>0?std::min(1.0,state.power/state.powerDemand):1;
    double budgetFactor=state.money>0?1:0.25;
    // Stable recipe order is explicit: extraction -> refining -> equipment -> labs.
    for(int kind=0;kind<10;++kind)for(auto& s:state.sites)if(int(s.type)==kind&&s.remaining<=0&&s.enabled&&state.regions[s.region].owner==state.player){
      double throughput=connection(s.region);
      int consumers=0;
      for(auto& other:state.sites)if(other.region==s.region&&other.enabled&&other.remaining<=0&&other.type!=Facility::Depot)++consumers;
      double f=std::min(1.0,throughput/(8*std::max(1,consumers)))*powerFactor*budgetFactor*(1+0.1*state.levels[5]);
      if(s.type==Facility::PowerPlant||s.type==Facility::Depot)f=std::min(1.0,throughput/8);
      s.efficiency=f;
      auto recipe=[&](std::initializer_list<std::pair<int,double>> inputs,std::initializer_list<std::pair<int,double>> outputs){
        double n=f;
        for(auto [r,amount]:inputs)n=std::min(n,state.stock[r]/amount);
        for(auto [r,amount]:inputs)state.stock[r]-=amount*n;
        for(auto [r,amount]:outputs)state.stock[r]+=amount*n;
        s.efficiency=n;
      };
      switch(s.type){
        case Facility::Mine:state.stock[0]+=3*f*state.regions[s.region].ore;
        break;
        case Facility::OilWell:state.stock[1]+=2*f*state.regions[s.region].oil;
        break;
        case Facility::Steelworks:recipe({{0,2}},{{2,1.4}});
        break;
        case Facility::Refinery:recipe({{1,1.5}},{{3,1.2}});
        break;
        case Facility::Factory:recipe({{2,0.6},{3,0.25}},{{4,1.4},{5,0.45}});
        break;
        case Facility::Laboratory:state.researchRate+=4*f*(0.2+1.6*state.researchBudget)*(1+0.12*state.levels[6]);
        state.upkeep+=10*f*state.researchBudget;
        break;
        case Facility::Shipyard:recipe({{2,1},{3,0.5}},{{5,0.8}});
        break;
        default:break;
      }
    }
    state.money=std::max(0.0,state.money+state.income-state.upkeep);
    for(int i=0;i<resourceCount;++i){
      state.stock[i]=std::clamp(state.stock[i],0.0,i==6?20.0:20000.0);
      state.delta[i]=state.stock[i]-before[i];
    }
    if(state.money==0)state.stability=std::max(15.0,state.stability-0.02);
    else state.stability=std::min(100.0,state.stability+0.005);
  }
  void Simulation::researchTick(){
    if(state.research.empty())return;
    auto& j=state.research.front();
    auto& t=techs[j.tech];
    j.progress+=state.researchRate*(t.field==state.specialization?1.35:1);
    if(j.progress>=t.cost){
      state.completed[j.tech]=true;
      state.levels[int(t.field)]=std::max(state.levels[int(t.field)],t.tier+1);
      log("Research complete: "+t.name);
      state.research.pop_front();
    }
  }
  void Simulation::combat(){
    if(state.simpleEconomy){
      if(!state.war||state.target<0)return;auto& region=state.regions[state.target];if(region.owner!=state.enemy){state.target=-1;return;}
      double strength=0;int groundCount=0;for(auto& unit:state.units)if(unit.owner==state.player&&unit.health>0&&unit.remaining<=0&&unit.target==state.target&&!unit.moving){double d=distance(unit.position,region.center);if(d>1.5)continue;double weight=unit.type==UnitType::Armour?1.8:unit.type==UnitType::Artillery?1.4:0.8;strength+=weight*unit.health/100;if(int(unit.type)<=2||unit.type==UnitType::SpecialForces)++groundCount;unit.health=std::max(0.0,unit.health-0.10*region.defense);}
      if(groundCount==0)return;strength*=1+0.12*state.levels[0]+0.03*state.levels[1]+0.02*state.levels[3];
      region.capture=std::clamp(region.capture+(strength/(4*region.defense)-0.35)*0.018,0.0,1.0);
      if(region.capture>=1){region.owner=state.player;state.money+=500;log("Sector secured. Its economy is now under your control.");for(auto& unit:state.units)if(unit.owner==state.enemy&&unit.region==state.target)unit.health=0;state.target=-1;int held=0,total=0;for(auto& r:state.regions)if(r.nation==state.enemy){++total;if(r.owner==state.player)++held;}if(held==total){state.victory=true;state.war=false;log("Rival territory secured: victory");}}
      return;
    }
    if(!state.war||state.target<0)return;
    auto& r=state.regions[state.target];
    if(r.owner!=state.enemy){
      state.target=-1;
      return;
    }
    double supplied=0;
    for(int n:r.neighbours)if(state.regions[n].owner==state.player)supplied=std::max(supplied,std::min(1.0,connection(n)/8));
    if((supplied==0||world_->countries[state.player].neighbours.empty())&&state.levels[2]>=1)for(auto& site:state.sites)if(site.type==Facility::Shipyard&&site.remaining<=0&&site.enabled&&state.regions[site.region].owner==state.player&&connection(site.region)>0)supplied=std::max(supplied,0.65);
    double need=state.brigades*0.08;
    double f=need>0?std::min({1.0,state.stock[3]/need,state.stock[4]/(need*2),supplied}):0;
    state.stock[3]-=need*f;
    state.stock[4]-=need*2*f;
    double support=1+0.12*state.levels[0]+0.035*state.levels[1]+0.025*state.levels[2]+0.03*state.levels[3];
    double strength=state.brigades*support*f;
    double resistance=5*r.defense;
    r.capture=std::clamp(r.capture+(strength/resistance-0.5)*0.015,0.0,1.0);
    state.brigades=std::max(0.0,state.brigades-0.004*r.defense*f/(1+0.1*state.levels[0]));
    if(r.capture>=1){
      r.owner=state.player;
      state.target=-1;
      state.money+=1000;
      log("Sector captured. Connect a supply route before advancing.");
      int held=0,total=0;
      for(auto& sector:state.regions)if(sector.nation==state.enemy){
        ++total;
        if(sector.owner==state.player)++held;
      }
      if(held>=total){
        state.victory=true;
        state.war=false;
        log("Campaign victory: the rival's territory is controlled.");
      }
    }
    if(state.brigades<0.1){
      state.defeat=true;
      state.war=false;
      log("Campaign lost: armed forces exhausted.");
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
    if(state.levels[4]<4||state.levels[6]<4||state.money<(state.simpleEconomy?75000:25000)||(!state.simpleEconomy&&(state.stock[2]<400||state.stock[3]<250))||state.stock[6]>=20)return false;
    state.money-=state.simpleEconomy?75000:25000;
    if(!state.simpleEconomy){state.stock[2]-=400;state.stock[3]-=250;}
    state.stock[6]+=1;
    log("Strategic warhead stockpiled. Conventional victory remains possible.");
    return true;
  }
  bool Simulation::launchNuclear(int region){
    if(!state.war||state.stock[6]<1||region<0||region>=int(state.regions.size())||state.regions[region].owner!=state.enemy)return false;
    bool ready=false;
    for(auto& s:state.sites)if(s.type==Facility::Silo&&s.enabled&&s.remaining<=0&&state.regions[s.region].owner==state.player&&connection(s.region)>0)ready=true;
    if(!ready||(state.simpleEconomy&&state.levels[4]<5))return false;
    state.stock[6]-=1;
    state.nuclearUsed=true;
    state.regions[region].owner=state.player;
    state.regions[region].capture=1;
    for(auto& s:state.sites)if(s.region==region)s.enabled=false;
    state.stability=std::max(0.0,state.stability-65);
    state.money*=0.2;
    state.brigades*=0.45;
    for(int i=0;i<6;++i)state.stock[i]*=0.2;
    for(auto& s:state.sites)if(state.regions[s.region].owner==state.player&&s.type!=Facility::Depot)s.enabled=false;
    state.target=-1;
    state.defeat=true;
    state.war=false;
    log("Scripted second strike: industry devastated. Mutual destruction ends the campaign.");
    return true;
  }
  void Simulation::save(const std::string& path) const {
    if(state.player<0)throw std::runtime_error("No campaign to save");
    Object o;
    auto n=[&](const char* k,double v){
      o[k]=num(v);
    };
    n("version",2);
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
    Array units;for(auto& u:state.units){Object v;v["owner"]=str(world_->countries[u.owner].code);v["region"]=num(u.region);v["target"]=num(u.target);v["source"]=num(u.sourceSite);v["type"]=num(int(u.type));v["x"]=num(u.position.x);v["y"]=num(u.position.y);v["dx"]=num(u.destination.x);v["dy"]=num(u.destination.y);v["health"]=num(u.health);v["remaining"]=num(u.remaining);v["total"]=num(u.total);v["moving"]=num(u.moving);Array path;for(int r:u.path)path.push_back(num(r));v["path"]=Json(path);units.push_back(Json(v));}o["units"]=Json(units);
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
    int version=integer(o,"version",1,2);
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
    auto& stock=asArray(field(o,"stock"));
    if(stock.size()!=resourceCount)throw std::runtime_error("Invalid inventory");
    for(int i=0;i<resourceCount;++i){
      double v=asNumber(stock[i]);
      if(!std::isfinite(v)||v<0||v>(i==6?20:20000))throw std::runtime_error("Invalid stockpile");
      c.stock[i]=v;
    }
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
    c.sites.clear();
    auto& sites=asArray(field(o,"sites"));
    if(sites.size()>1000)throw std::runtime_error("Too many sites");
    for(auto& s:sites){Site site{integer(s,"region",0,int(c.regions.size())-1),Facility(integer(s,"type",0,version>=2?15:9)),bounded(s,"remaining",0,120),0,bool(integer(s,"enabled",0,1))};site.position=version>=2?Point{bounded(s,"x",-720,720),bounded(s,"y",-90,90)}:c.regions[site.region].center;site.destroyed=version>=2?bool(integer(s,"destroyed",0,1)):false;c.sites.push_back(site);}
    if(version>=2){c.units.clear();auto& units=asArray(field(o,"units"));if(units.size()>1000)throw std::runtime_error("Too many units");for(auto& v:units){Unit u;u.owner=world_->find(string(v,"owner"));if(u.owner!=c.player&&u.owner!=c.enemy)throw std::runtime_error("Invalid unit owner");u.region=integer(v,"region",0,int(c.regions.size())-1);u.target=integer(v,"target",-1,int(c.regions.size())-1);u.sourceSite=integer(v,"source",-1,int(c.sites.size())-1);u.type=UnitType(integer(v,"type",0,8));u.position={bounded(v,"x",-720,720),bounded(v,"y",-90,90)};u.destination={bounded(v,"dx",-720,720),bounded(v,"dy",-90,90)};u.health=bounded(v,"health",0,100);u.remaining=bounded(v,"remaining",0,120);u.total=bounded(v,"total",0,120);u.moving=bool(integer(v,"moving",0,1));for(auto& r:asArray(field(v,"path"))){double n=asNumber(r);if(!std::isfinite(n)||n<0||n>=c.regions.size()||std::floor(n)!=n)throw std::runtime_error("Invalid unit path");u.path.push_back(int(n));}c.units.push_back(u);}}
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
      c.research.push_back({t,bounded(j,"progress",0,techs[t].cost)});
    }
    c.events.clear();
    for(auto& e:asArray(field(o,"events"))){
      if(c.events.size()==80)break;
      c.events.push_back(asString(e));
    }
    c.lastAutosave=c.day;
    candidate.log("Campaign restored");
    state=std::move(c);
  }
}
