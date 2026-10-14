#include "escalation/simulation.hpp"
#include <utility>
#include <algorithm>
namespace escalation {
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
    effect(17,"MANPADS equipment reserved; air targets are not implemented");effect(18,"Unlock independent anti-tank guided missile mount: 0.5–40 km, armour targets only");effect(58,"Unlock conventional ballistic missile sites and point strikes");effect(59,"Unlock nuclear warhead production; requires Computing centres");effect(60,"Unlock nuclear delivery from an operational missile site");effect(62,"Unlock mobile missile batteries and missile sites");effect(63,"Missile damage +15%");effect(65,"Missile range +20%");
    for(int i=70;i<84;++i)effect(i,"Regional production +6%");
    for(int i=84;i<98;++i)effect(i,"Research output +8%");
    for(int i=98;i<112;++i)effect(i,"Ground movement +3%");
    // Appended IDs preserve every existing ground technology and save reference.
    auto naval=[&](const char* key,const char* name,const char* effect,int tier,int year,std::vector<int> parents){result.push_back({key,name,effect,Field::Naval,tier,year,500+double(tier)*250,std::move(parents)});};
    naval("naval-foundations","Naval foundations","Unlock coastal shipyard manufacture",0,1920,{});
    naval("surface-warfare","Surface warfare","Unlock destroyer manufacture",1,1930,{112});
    naval("submarine-warfare","Submarine warfare","Unlock submarine manufacture",1,1930,{112});
    naval("naval-gunnery","Naval gunnery","Destroyer naval cannon damage +20%",2,1940,{113});
    naval("antisubmarine-warfare","Anti-submarine warfare","Unlock destroyer anti-submarine weapons",2,1940,{113});
    naval("antiship-missiles","Guided anti-ship missiles","Unlock guided anti-ship weapons",3,1960,{115});
    naval("submarine-torpedoes","Submarine torpedoes","Submarine torpedo damage +20%",2,1940,{114});
    naval("naval-cruise-missiles","Naval cruise missiles","Unlock naval land-attack cruise missiles",4,1980,{117,118});
    naval("naval-sensors","Integrated naval sensors","Naval weapon range +10%",3,1970,{116,118});
    naval("naval-propulsion","Naval propulsion","Naval movement +15%",2,1950,{113,114});
    // Reference years are descriptive, never availability gates. Keep existing
    // references where possible, correcting cross-branch chronology only.
    bool changed=true;while(changed){changed=false;for(auto& tech:result)for(int p:tech.prerequisites)if(tech.year<result[p].year){tech.year=result[p].year;changed=true;}}
    auto mapExtract=[&](int id,const char* key,const char* image,const char* category,double level,double points,double contribution){
      auto& tech=result.at(id);
      tech.sourceKey=key;tech.sourceImage=image;tech.sourceCategory=category;
      tech.sourceLevel=level;tech.sourcePoints=points;tech.levelContribution=contribution;
      tech.name=key;std::replace(tech.name.begin(),tech.name.end(),'_',' ');
      // Extract points use a different clock: 20 native work/point, then the
      // supplied GeneralMultiplier 0.8. Values are frozen when queued.
      tech.cost=points*20.0*0.8;
    };
#include "research_extracts.inc"
    return result;
  }
}
