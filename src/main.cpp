#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "escalation/renderer.hpp"
#include "escalation/headless_context.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#ifndef ESCALATION_DATA_DIR
#define ESCALATION_DATA_DIR "data"
#endif
using namespace escalation;
static ImVec4 muted(0.54f,0.64f,0.68f,1),accent(0.31f,0.81f,0.72f,1);
static ImVec2 vec(Point p){
  return {float(p.x),float(p.y)};
}
static void wrapped(const std::string& text){
  ImGui::TextWrapped("%s",text.c_str());
}
static void tooltip(const std::string& text){
  if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)){
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(360);
    wrapped(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}
static bool button(const char* label,bool enabled=true){
  ImGui::BeginDisabled(!enabled);
  bool clicked=ImGui::Button(label);
  ImGui::EndDisabled();
  return clicked;
}
static void heading(const char* text){
  ImGui::Spacing();
  ImGui::TextColored(accent,"%s",text);
  ImGui::Separator();
}
static void fixedWindow(const char* name,float x,float y,float w,float h){
  ImGui::SetNextWindowPos({x,y});
  ImGui::SetNextWindowSize({w,h});
  ImGui::Begin(name,nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar);
}
static void style(){
  ImGui::StyleColorsDark();
  auto& s=ImGui::GetStyle();
  s.WindowRounding=5;
  s.FrameRounding=3;
  s.ScrollbarRounding=3;
  s.WindowPadding={16,14};
  s.FramePadding={8,6};
  s.ItemSpacing={8,8};
  s.Colors[ImGuiCol_WindowBg]={0.045f,0.065f,0.085f,0.98f};
  s.Colors[ImGuiCol_FrameBg]={0.09f,0.14f,0.17f,1};
  s.Colors[ImGuiCol_Button]={0.11f,0.27f,0.27f,1};
  s.Colors[ImGuiCol_ButtonHovered]={0.17f,0.42f,0.39f,1};
  s.Colors[ImGuiCol_Header]={0.10f,0.26f,0.26f,1};
  s.Colors[ImGuiCol_CheckMark]=accent;
  s.Colors[ImGuiCol_PlotHistogram]=accent;
  s.Colors[ImGuiCol_Text]={0.86f,0.91f,0.92f,1};
}
static std::filesystem::path userData(){
#ifdef _WIN32
  if(auto p=std::getenv("APPDATA"))return std::filesystem::path(p)/"Escalation";
#else
  if(auto p=std::getenv("XDG_DATA_HOME"))return std::filesystem::path(p)/"escalation";
  if(auto p=std::getenv("HOME"))return std::filesystem::path(p)/".local/share/escalation";
#endif
  return "userdata";
}
static std::string dataDirectory(const char* executable){
  if(auto p=std::getenv("ESCALATION_DATA"))return p;
  auto beside=std::filesystem::absolute(executable).parent_path()/"data";
  if(std::filesystem::exists(beside/"world.geojson"))return beside.string();
  return ESCALATION_DATA_DIR;
}
struct App {
  World world;
  Simulation sim;
  Renderer renderer;
  Camera camera;
  enum Screen{
    Menu,Select,Game
  };
  enum Panel{
    None,Economy,Build,Research,Forces,Infrastructure,Log
  };
  Screen screen=Menu;
  Panel panel=None;
  int selectedNation=-1,selectedRegion=-1,selectedSite=-1,selectedUnit=-1,construction=-1,focus=5,speed=1;
  bool paused=true,regionOpen=false,unitOpen=false,nationOpen=false,commandMode=false,showRanges=true,autoResearch=false,leftStarted=false,rightDragged=false,treeFocus=false;
  double accumulator=0;
  Point pointer,leftStart,popup;
  std::vector<int> selectedUnits;
  std::string message;
  std::filesystem::path saves;
  explicit App(World w,std::filesystem::path p):world(std::move(w)),sim(world),saves(std::move(p)){
  }
  void notice(const std::string& text){
    message=text;
  }
  void flag(int nation,ImVec2 size={
    28,21
  }){
    auto uv=renderer.assets.flag(nation);
    ImGui::Image(ImTextureID(renderer.assets.flags),size,vec(uv.min),vec(uv.max));
  }
  bool save(bool automatic=false){
    try{
      sim.save((saves/(automatic?"autosave.json":"campaign.json")).string());
      notice("Campaign saved");
      return true;
    }
    catch(const std::exception& e){
      notice(e.what());
      return false;
    }
  }
  void resume(){
    try{
      auto a=saves/"campaign.json",b=saves/"autosave.json";
      if(!std::filesystem::exists(a)||(std::filesystem::exists(b)&&std::filesystem::last_write_time(b)>std::filesystem::last_write_time(a)))a=b;
      sim.load(a.string());
      sim.state.simpleEconomy=true;
      renderer.campaign(world,sim.state);
      selectedNation=sim.state.player;
      screen=Game;
      panel=None;
      paused=true;
      selectedRegion=-1;
      centerNation(selectedNation);
      notice("Campaign restored; paused");
    }
    catch(const std::exception& e){
      notice(e.what());
    }
  }
  void center(Point point){
    camera.longitude=std::remainder(point.x,360.0);
    camera.latitude=std::clamp(point.y,-89.0,89.0);
  }
  void centerNation(int nation){
    if(nation<0)return;
    auto& country=world.countries[nation];
    center(country.center);
    double span=0;
    for(auto& polygon:country.polygons){
      double area=0;
      auto& ring=polygon.rings[0];
      for(size_t i=0;i<ring.size();++i)area+=ring[i].x*ring[(i+1)%ring.size()].y-ring[(i+1)%ring.size()].x*ring[i].y;
      if(std::abs(area)>span)span=std::abs(area);
    }
    double size=std::sqrt(std::max(0.1,span));
    camera.zoom=std::clamp((camera.globe?45.0:170.0)/size,camera.globe?1.0:1.2,camera.globe?20.0:48.0);
  }
  void start(){
    if(selectedNation<0)return;
    sim.start(selectedNation,Field(focus));
    renderer.campaign(world,sim.state);
    screen=Game;
    panel=None;
    selectedRegion=-1;
    selectedSite=-1;
    selectedUnit=-1;
    selectedUnits.clear();
    nationOpen=false;
    regionOpen=unitOpen=false;
    paused=true;
    accumulator=0;
    centerNation(selectedNation);
    notice("Select units or land. Construction [B], research [R], Space resumes.");
  }
  void toggle(Panel wanted){
    panel=panel==wanted?None:wanted;
    if(panel==Research)treeFocus=true;
  }
  void selectPoint(Point geo){
    if(screen==Select){
      int nation=world.pick(geo);
      if(nation>=0&&world.countries[nation].code!="ATA"){
        selectedNation=nation;
        nationOpen=true;
        popup=pointer;
      }
      return;
    }
    if(screen!=Game)return;
    if(construction>=0){
      int region=sim.regionAt(geo);
      if(sim.buildAt(region,Facility(construction),geo)){
        notice(std::string("Constructing ")+facilityNames[construction]);
        if(!ImGui::GetIO().KeyShift)construction=-1;
      }
      else notice("Cannot build here: choose controlled land, an unlocked building and sufficient funds; shipyards need a coast.");
      return;
    }
    if(commandMode&&!selectedUnits.empty()){
      bool issued=false;
      for(int id:selectedUnits)issued=sim.orderUnitAt(size_t(id),geo)||issued;
      notice(issued?"Movement / attack order issued":"Order unavailable: check war status, land access and unit readiness");
      commandMode=false;
      return;
    }
    selectedUnit=-1;
    double nearest=17;
    for(size_t i=0;i<sim.state.units.size();++i){
      auto& u=sim.state.units[i];
      if(u.health<=0)continue;
      bool visible;
      auto p=camera.project(u.position,&visible);
      double d=std::hypot(p.x-pointer.x,p.y-pointer.y);
      if(visible&&d<nearest){
        nearest=d;
        selectedUnit=int(i);
      }
    }
    if(selectedUnit>=0){
      if(!ImGui::GetIO().KeyShift)selectedUnits.clear();
      if(sim.state.units[selectedUnit].owner==sim.state.player&&std::find(selectedUnits.begin(),selectedUnits.end(),selectedUnit)==selectedUnits.end())selectedUnits.push_back(selectedUnit);
      unitOpen=true;
      regionOpen=false;
      popup=pointer;
      return;
    }
    selectedUnits.clear();
    unitOpen=false;
    selectedRegion=sim.regionAt(geo);
    selectedSite=-1;
    nearest=16;
    for(size_t i=0;i<sim.state.sites.size();++i){
      auto& s=sim.state.sites[i];
      if(s.destroyed)continue;
      bool visible;
      auto p=camera.project(s.position,&visible);
      double d=std::hypot(p.x-pointer.x,p.y-pointer.y);
      if(visible&&d<nearest){
        nearest=d;
        selectedSite=int(i);
        selectedRegion=s.region;
      }
    }
    regionOpen=selectedRegion>=0;
    popup=pointer;
  }
  void controls(){
    auto& io=ImGui::GetIO();
    pointer={
      io.MousePos.x,io.MousePos.y
    };
    if(!io.WantTextInput){
      if(ImGui::IsKeyPressed(ImGuiKey_G))camera.globe=!camera.globe;
      if(ImGui::IsKeyPressed(ImGuiKey_H))centerNation(selectedNation);
      if(ImGui::IsKeyPressed(ImGuiKey_F)&&selectedUnit>=0){
        center(sim.state.units[selectedUnit].position);
        camera.zoom=std::max(camera.zoom,40.0);
      }
      if(screen==Game){
        if(ImGui::IsKeyPressed(ImGuiKey_Space))paused=!paused;
        if(ImGui::IsKeyPressed(ImGuiKey_B))toggle(Build);
        if(ImGui::IsKeyPressed(ImGuiKey_R))toggle(Research);
        if(ImGui::IsKeyPressed(ImGuiKey_U))toggle(Forces);
        if(ImGui::IsKeyPressed(ImGuiKey_F5))save();
        if(ImGui::IsKeyPressed(ImGuiKey_F9))resume();
      }
      if(ImGui::IsKeyPressed(ImGuiKey_Escape)){
        if(panel!=None||regionOpen||unitOpen||construction>=0||commandMode){
          panel=None;
          regionOpen=unitOpen=false;
          construction=-1;
          commandMode=false;
        }
        else if(screen==Game){
          paused=true;
          ImGui::OpenPopup("Command pause");
        }
        else if(screen==Select){
          nationOpen=false;
          selectedNation=-1;
        }
      }
    }
    bool over=pointer.x>=camera.x&&pointer.x<camera.x+camera.width&&pointer.y>=camera.y&&pointer.y<camera.y+camera.height;
    if(over&&!io.WantCaptureMouse){
      if(io.MouseWheel!=0){
        auto before=camera.unproject(pointer);
        camera.zoom=std::clamp(camera.zoom*std::exp(io.MouseWheel*0.16),camera.globe?0.55:0.8,512.0);
        auto after=camera.unproject(pointer);
        if(before&&after){
          center({
            camera.longitude+std::remainder(before->x-after->x,360.0),camera.latitude+before->y-after->y
          });
        }
      }
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Right))rightDragged=false;
      if(ImGui::IsMouseDragging(ImGuiMouseButton_Right,5)){
        rightDragged=true;
        double scale=camera.globe?114.6:360.0;
        center({
          camera.longitude-io.MouseDelta.x*scale/camera.width/camera.zoom,camera.latitude+io.MouseDelta.y*scale/camera.width/camera.zoom
        });
      }
      if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)&&!rightDragged){
        if(construction>=0){
          construction=-1;
          notice("Construction placement cancelled");
        }
        else if(screen==Game&&!selectedUnits.empty()){
          auto geo=camera.unproject(pointer);
          if(geo){
            bool issued=false;
            for(int id:selectedUnits)issued=sim.orderUnitAt(id,*geo)||issued;
            notice(issued?"Orders issued":"Order blocked: check access, readiness and war status");
          }
        }
      }
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        leftStarted=true;
        leftStart=pointer;
      }
    }
    if(leftStarted&&ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
      leftStarted=false;
      double drag=std::hypot(pointer.x-leftStart.x,pointer.y-leftStart.y);
      if(drag>8&&screen==Game&&construction<0){
        selectedUnits.clear();
        for(size_t i=0;i<sim.state.units.size();++i){
          auto& u=sim.state.units[i];
          if(u.owner!=sim.state.player||u.health<=0||u.remaining>0)continue;
          bool visible;
          auto p=camera.project(u.position,&visible);
          if(visible&&p.x>=std::min(leftStart.x,pointer.x)&&p.x<=std::max(leftStart.x,pointer.x)&&p.y>=std::min(leftStart.y,pointer.y)&&p.y<=std::max(leftStart.y,pointer.y))selectedUnits.push_back(int(i));
        }
        if(!selectedUnits.empty()){
          selectedUnit=selectedUnits.front();
          unitOpen=true;
          regionOpen=false;
          popup=pointer;
          notice("Formation selected; right click to order");
        }
      }
      else if(over&&!io.WantCaptureMouse){
        auto geo=camera.unproject(pointer);
        if(geo)selectPoint(*geo);
      }
    }
  }
  void hud(float w,float h){
    fixedWindow("Main HUD",0,0,w,52);
    ImGui::TextColored(accent,"ESCALATION");
    ImGui::SameLine(132);
    if(screen==Game){
      flag(sim.state.player,{
        24,18
      });
      ImGui::SameLine();
      ImGui::Text("$%.0f (%+.0f/day)  | RP %.1f | %d",sim.state.money,sim.state.income-sim.state.upkeep,sim.state.researchRate,sim.year());
      ImGui::SameLine();
      if(button(paused?"Resume [Space]":"Pause [Space]"))paused=!paused;
      ImGui::SameLine();
      ImGui::SetNextItemWidth(65);
      const char* values[]={
        "1x","2x","4x","8x"
      };
      int v=speed==1?0:speed==2?1:speed==4?2:3;
      if(ImGui::Combo("##speed",&v,values,4))speed=1<<v;
      ImGui::SameLine();
      if(button("Save [F5]"))save();
    }
    else ImGui::TextColored(accent,"SELECT A NATION ON THE MAP");
    ImGui::SameLine();
    if(button(camera.globe?"Globe [G]":"Map [G]"))camera.globe=!camera.globe;
    ImGui::SameLine();
    if(button("Fit [H]",selectedNation>=0))centerNation(selectedNation);
    ImGui::SameLine();
    if(button(camera.terrain?"Relief":"Political"))camera.terrain=!camera.terrain;
    ImGui::End();
    fixedWindow("Command HUD",0,h-46,w,46);
    if(screen==Game){
      if(button("Build [B]"))toggle(Build);
      ImGui::SameLine();
      if(button("Research [R]"))toggle(Research);
      ImGui::SameLine();
      if(button("Forces [U]"))toggle(Forces);
      ImGui::SameLine();
      if(button("Economy"))toggle(Economy);
      ImGui::SameLine();
      if(button("Roads"))toggle(Infrastructure);
      ImGui::SameLine();
      if(button("Log"))toggle(Log);
      ImGui::SameLine();
    }
    else{
      if(button("Back")){
        screen=Menu;
        nationOpen=false;
        selectedNation=-1;
      }
      ImGui::SameLine();
    }
    std::string status=construction>=0?std::string("Place ")+facilityNames[construction]+" on land | Shift repeats | Right click cancels":commandMode?"Click a destination for your selected formation":message.empty()?"Left click selects | Drag selects units | Right click orders / drag pans | Wheel zooms":message;
    ImGui::TextColored(muted,"%s",status.substr(0,100).c_str());
    tooltip(status);
    ImGui::End();
  }
  void mainMenu(float w,float h){
    camera.width=w;
    camera.height=h;
    camera.x=camera.y=0;
    camera.zoom=0.9;
    fixedWindow("Main menu",42,h*0.24f,355,360);
    ImGui::SetWindowFontScale(1.7f);
    ImGui::TextColored(accent,"ESCALATION");
    ImGui::SetWindowFontScale(1);
    wrapped("Command on the map. Build an economy. Research a doctrine. Move visible forces into battle.");
    if(ImGui::Button("Start game",{
      -1,42
    })){
      screen=Select;
      selectedNation=-1;
      camera.longitude=12;
      camera.latitude=28;
      camera.zoom=0.9;
      message="Click land to preview a nation. Zoom in for smaller countries.";
    }
    if(button("Continue saved campaign",std::filesystem::exists(saves/"campaign.json")||std::filesystem::exists(saves/"autosave.json")))resume();
    if(button("Quit"))std::exit(0);
    heading("PROTOTYPE 0.2");
    wrapped("Modern-border alternate history. Direct construction, currency economy and nine unit families. One static rival.");
    ImGui::End();
  }
  void positionPopup(const char* name,bool& open,float height){
    ImGui::SetNextWindowPos({
      float(std::clamp(popup.x+14.0,12.0,double(camera.width-350))),float(std::clamp(popup.y+8.0,62.0,double(camera.height+52-height)))
    },ImGuiCond_Appearing);
    ImGui::SetNextWindowSize({
      330,height
    },ImGuiCond_Appearing);
    ImGui::Begin(name,&open,ImGuiWindowFlags_NoCollapse);
  }
  void selection(float w,float h){
    hud(w,h);
    if(!nationOpen||selectedNation<0)return;
    positionPopup("Nation preview",nationOpen,440);
    flag(selectedNation,{
      64,48
    });
    ImGui::SameLine();
    wrapped(world.countries[selectedNation].name);
    ImGui::Separator();
    wrapped(world.countries[selectedNation].history);
    ImGui::Text("Reference population: %.1f M",world.countries[selectedNation].population/1e6);
    ImGui::Text("Reference GDP: $%.1f B",world.countries[selectedNation].gdp/1000);
    wrapped("Rival: "+world.countries[world.opponent(selectedNation)].name);
    ImGui::Combo("Doctrine",&focus,fieldNames.data(),8);
    if(ImGui::Button("Take command",{
      -1,34
    }))start();
    if(button("Zoom to country"))centerNation(selectedNation);
    ImGui::End();
  }
  void buildPanel(){
    heading("CONSTRUCTION");
    wrapped("Choose a building, then click controlled land. Every cost is paid from your treasury.");
    const int types[]={
      13,14,15,6,10,11,4,8,9,12,5,7
    };
    for(int id:types){
      ImGui::PushID(id);
      if(ImGui::Button(facilityNames[id],{
        -1,30
      })){
        construction=id;
        panel=None;
        regionOpen=unitOpen=false;
        notice("Click controlled land to place construction");
      }
      ImGui::TextColored(muted,"$%.0f | +$%.0f/day | %d days",sim.buildCost(Facility(id)),sim.buildingIncome(Facility(id)),id==9?120:35);
      if(id==6)ImGui::TextColored(accent,"Produces research points");
      if(id==8)ImGui::TextColored(muted,"Requires a coastal location and naval research");
      ImGui::PopID();
    }
  }
  void production(int site){
    for(int type=0;type<9;++type){
      auto reason=sim.unitReason(UnitType(type),site);
      ImGui::PushID(type);
      std::string name=std::string(unitNames[type])+" $"+std::to_string(int(sim.unitCost(UnitType(type))));
      if(button(name.c_str(),reason.empty())){
        sim.queueUnit(UnitType(type),site);
        notice("Production queued");
      }
      tooltip(reason.empty()?std::to_string(int(sim.unitDays(UnitType(type))))+" days to deploy":reason);
      ImGui::PopID();
    }
  }
  void regionPopup(){
    if(!regionOpen||selectedRegion<0)return;
    positionPopup(selectedSite>=0?"Building / production":"Region orders",regionOpen,440);
    auto& r=sim.state.regions[selectedRegion];
    flag(r.owner);
    ImGui::SameLine();
    ImGui::Text("Sector %d - %s",selectedRegion+1,r.owner==sim.state.player?"Controlled":"Rival");
    ImGui::Text("Regional income: $%.1f/day",sim.regionIncome(selectedRegion));
    ImGui::Text("Roads: level %d / 5",r.roads);
    if(r.owner==sim.state.player){
      std::string road="Upgrade roads: $"+std::to_string(int(sim.roadCost(selectedRegion)));
      if(button(road.c_str(),r.roads<5&&sim.state.money>=sim.roadCost(selectedRegion)))sim.upgradeRoad(selectedRegion);
      tooltip("Each road level adds 15% regional income and 20% ground movement speed.");
      if(button("Place a new building"))toggle(Build);
    }
    else if(button(sim.state.war?"Order ground offensive":"Declare war",!sim.state.victory&&!sim.state.defeat)){
      if(!sim.state.war)sim.declareWar();
      else if(!sim.attack(selectedRegion))notice("Ground offensive needs an accessible frontier or amphibious capability");
    }
    if(selectedSite>=0){
      auto& site=sim.state.sites[selectedSite];
      heading(facilityNames[int(site.type)]);
      if(site.remaining>0){
        ImGui::Text("%.0f construction days left",site.remaining);
        ImGui::ProgressBar(float(1-site.remaining/(site.type==Facility::Silo?120:35)));
        if(button("Cancel construction",r.owner==sim.state.player)){
          sim.cancelSite(selectedSite);
          selectedSite=-1;
          notice("Construction cancelled");
        }
      }
      else if(r.owner==sim.state.player){
        ImGui::Checkbox("Operational",&site.enabled);
        if(site.type==Facility::ArmyBase||site.type==Facility::Factory||site.type==Facility::Airbase||site.type==Facility::Shipyard||site.type==Facility::Silo){
          heading("UNIT PRODUCTION");
          production(selectedSite);
        }
      }
    }
    if(r.capture>0&&r.owner!=sim.state.player)ImGui::ProgressBar(float(r.capture),{
      -1,0
    },"Offensive progress");
    ImGui::End();
  }
  void unitPopup(){
    if(!unitOpen||selectedUnit<0||selectedUnit>=int(sim.state.units.size()))return;
    positionPopup("Unit orders",unitOpen,310);
    auto& unit=sim.state.units[selectedUnit];
    flag(unit.owner);
    ImGui::SameLine();
    wrapped(unitNames[int(unit.type)]);
    ImGui::ProgressBar(float(unit.health/100),{
      -1,0
    },"Strength");
    if(unit.remaining>0){
      ImGui::Text("Deploying in %.0f days",unit.remaining);
      ImGui::ProgressBar(float(1-unit.remaining/unit.total));
      if(button("Cancel production",unit.owner==sim.state.player))sim.cancelUnit(selectedUnit);
    }
    else if(unit.owner==sim.state.player){
      ImGui::Text("Selected formation: %zu units",selectedUnits.size());
      if(button("Move / attack - click destination")){
        commandMode=true;
        unitOpen=false;
      }
      if(button("Hold")){
        for(int id:selectedUnits)sim.holdUnit(id);
      }
      if(button("Focus unit [F]")){
        center(unit.position);
        camera.zoom=std::max(camera.zoom,50.0);
      }
      wrapped("Right-click a destination on the map. Drag a box to select multiple friendly units.");
    }
    else wrapped("Static defending formation. No enemy command AI in this prototype.");
    ImGui::End();
  }
  void economyPanel(){
    heading("NATIONAL ECONOMY");
    ImGui::Text("Treasury: $%.0f",sim.state.money);
    ImGui::Text("Income: $%.1f/day",sim.state.income);
    ImGui::Text("Upkeep: $%.1f/day",sim.state.upkeep);
    ImGui::Text("Net: %+.1f/day",sim.state.income-sim.state.upkeep);
    wrapped("Farms, commercial districts and industrial districts generate money. Spend it on facilities, unit production, research and roads.");
    ImGui::Separator();
    for(size_t i=0;i<sim.state.sites.size();++i){
      auto& site=sim.state.sites[i];
      if(site.destroyed||sim.state.regions[site.region].owner!=sim.state.player)continue;
      ImGui::PushID(int(i));
      if(ImGui::Selectable(facilityNames[int(site.type)])){
        selectedSite=int(i);
        selectedRegion=site.region;
        regionOpen=true;
        popup={
          350,90
        };
        center(site.position);
      }
      ImGui::TextColored(muted,"Sector %d | +$%.0f/day | %s",site.region+1,sim.buildingIncome(site.type),site.remaining>0?"constructing":site.enabled?"active":"off");
      ImGui::PopID();
    }
  }
  void forcesPanel(){
    heading("ARMED FORCES");
    ImGui::Text("%.1f ground brigades",sim.state.brigades);
    if(button(sim.state.war?"War in progress":"Declare war",!sim.state.war&&!sim.state.victory&&!sim.state.defeat))sim.declareWar();
    ImGui::Checkbox("Selected-unit / radar ranges",&showRanges);
    for(size_t i=0;i<sim.state.units.size();++i){
      auto& unit=sim.state.units[i];
      if(unit.owner!=sim.state.player||unit.health<=0)continue;
      ImGui::PushID(int(i));
      if(ImGui::Selectable(unitNames[int(unit.type)],int(i)==selectedUnit)){
        selectedUnit=int(i);
        selectedUnits={
          int(i)
        };
        unitOpen=true;
        popup={
          350,90
        };
        center(unit.position);
      }
      ImGui::TextColored(muted,"%.0f%% | %s",unit.health,unit.remaining>0?"production":unit.moving?"moving":"ready");
      ImGui::PopID();
    }
    heading("STRATEGIC WEAPONS");
    ImGui::Text("Warheads: %.0f",sim.state.stock[6]);
    if(button("Produce warhead: $75000",sim.state.money>=75000&&sim.state.levels[4]>=4&&sim.state.levels[6]>=4))sim.produceWarhead();
    tooltip("Requires nuclear deterrence and Computing centres research. Delivery requires intercontinental missiles and an operational missile site.");
    if(button("Review nuclear release",sim.state.war&&selectedRegion>=0&&sim.state.regions[selectedRegion].owner==sim.state.enemy&&sim.state.stock[6]>=1))ImGui::OpenPopup("Strategic release");
    if(ImGui::BeginPopupModal("Strategic release",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
      wrapped("This prototype's fixed retaliatory deterrent causes mutual destruction and ends the campaign. Confirming requires an operational missile site and delivery research.");
      if(button("Confirm strategic release")){
        if(!sim.launchNuclear(selectedRegion))notice("Delivery unavailable: check missile site and technology");
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if(button("Cancel"))ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
  }
  void roadsPanel(){
    heading("REGIONAL INFRASTRUCTURE");
    wrapped("Select a region on the map to upgrade its roads, income and movement capacity.");
    for(size_t i=0;i<sim.state.regions.size();++i){
      auto& r=sim.state.regions[i];
      if(r.owner!=sim.state.player)continue;
      ImGui::PushID(int(i));
      std::string label="Sector "+std::to_string(i+1)+" | roads "+std::to_string(r.roads);
      if(ImGui::Selectable(label.c_str())){
        selectedRegion=int(i);
        selectedSite=-1;
        regionOpen=true;
        popup={
          350,90
        };
        center(r.center);
      }
      ImGui::TextColored(muted,"$%.0f/day | next upgrade $%.0f",sim.regionIncome(int(i)),sim.roadCost(int(i)));
      ImGui::PopID();
    }
  }
  void researchTree(float w,float h){
    bool open=true;
    ImGui::SetNextWindowSize({
      std::min(w-48,1400.0f),h-136
    },ImGuiCond_Appearing);
    ImGui::SetNextWindowPos({
      24,66
    },ImGuiCond_Appearing);
    ImGui::Begin("Research tree - 1920 to 2050",&open,ImGuiWindowFlags_NoCollapse);
    auto& s=sim.state;
    ImGui::Text("%.1f RP/day | specialization %s (+35%%)",s.researchRate,fieldNames[int(s.specialization)]);
    float budget=float(s.researchBudget*100);
    ImGui::SetNextItemWidth(230);
    if(ImGui::SliderFloat("Research funding",&budget,0,100,"%.0f%%"))s.researchBudget=budget/100;
    ImGui::SameLine();
    ImGui::Checkbox("Auto queue doctrine",&autoResearch);
    if(!s.research.empty()){
      auto job=s.research.front();
      ImGui::Text("Active: %s | %zu queued",sim.techs[job.tech].name.c_str(),s.research.size());
      ImGui::ProgressBar(float(job.progress/sim.techs[job.tech].cost),{
        300,0
      });
      ImGui::SameLine();
      if(button("Cancel active project"))sim.removeResearch(0);
    }
    else ImGui::TextColored(muted,"Click an available node to queue it. Drag scrollbars to browse the full century.");
    for(int f=0;f<8;++f){
      if(f)ImGui::SameLine();
      ImGui::PushID(f);
      if(button(fieldNames[f])){
        focus=f;
        treeFocus=true;
      }
      ImGui::PopID();
    }
    ImGui::BeginChild("Technology canvas",{
      0,0
    },ImGuiChildFlags_Borders,ImGuiWindowFlags_HorizontalScrollbar);
    if(treeFocus){
      ImGui::SetScrollX(float(focus*198));
      ImGui::SetScrollY(float(std::max(0,(sim.year()-1920)/10*112-120)));
      treeFocus=false;
    }
    ImVec2 origin{
      ImGui::GetWindowPos().x-ImGui::GetScrollX(),ImGui::GetWindowPos().y-ImGui::GetScrollY()
    };
    auto* draw=ImGui::GetWindowDrawList();
    const float cellX=198,cellY=112,nodeW=178,nodeH=76;
    for(auto& tech:sim.techs){
      int id=int(&tech-sim.techs.data());
      float x=28+int(tech.field)*cellX,y=52+tech.tier*cellY;
      for(int parent:tech.prerequisites){
        auto& prerequisite=sim.techs[parent];
        ImVec2 a{
          origin.x+28+int(prerequisite.field)*cellX+nodeW/2,origin.y+52+prerequisite.tier*cellY+nodeH
        },b{
          origin.x+x+nodeW/2,origin.y+y
        };
        float middle=(a.y+b.y)/2;
        draw->AddBezierCubic(a,{
          a.x,middle
        },{
          b.x,middle
        },b,s.completed[parent]?IM_COL32(73,174,139,210):IM_COL32(85,101,112,140),1.6f);
      }
    }
    for(int f=0;f<8;++f)draw->AddText({
      origin.x+28+f*cellX,origin.y+12
    },IM_COL32(126,211,190,255),fieldNames[f]);
    for(size_t id=0;id<sim.techs.size();++id){
      auto& tech=sim.techs[id];
      float x=28+int(tech.field)*cellX,y=52+tech.tier*cellY;
      bool done=s.completed[id],queued=false;
      for(auto& job:s.research)if(job.tech==int(id))queued=true;
      bool available=sim.available(int(id));
      ImU32 fill=done?IM_COL32(23,69,56,255):queued?IM_COL32(86,62,27,255):available?IM_COL32(25,66,73,255):IM_COL32(29,40,49,255);
      ImU32 border=done?IM_COL32(88,187,141,255):queued?IM_COL32(224,169,83,255):available?IM_COL32(86,184,192,255):IM_COL32(62,79,93,255);
      ImGui::SetCursorPos({
        x,y
      });
      ImGui::PushID(int(id));
      ImGui::InvisibleButton("tech",{
        nodeW,nodeH
      });
      auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
      draw->AddRectFilled(a,b,fill,5);
      draw->AddRect(a,b,border,5,0,1.5f);
      draw->AddText({
        a.x+9,a.y+7
      },IM_COL32(147,164,173,255),std::to_string(tech.year).c_str());
      draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{
        a.x+9,a.y+25
      },done?IM_COL32(175,219,185,255):available?IM_COL32(220,235,233,255):IM_COL32(157,168,176,255),tech.name.c_str(),nullptr,nodeW-18);
      std::string state=done?"RESEARCHED":queued?"QUEUED":std::to_string(int(tech.cost))+" RP";
      draw->AddText({
        a.x+9,b.y-17
      },border,state.c_str());
      if(ImGui::IsItemClicked()){
        if(sim.queueResearch(int(id)))notice("Research queued: "+tech.name);
        else notice(sim.lockReason(int(id)));
      }
      std::string detail=tech.effect;
      for(int parent:tech.prerequisites)detail+="\nRequires: "+sim.techs[parent].name;
      auto reason=sim.lockReason(int(id));
      if(!reason.empty())detail+="\n"+reason;
      tooltip(detail);
      ImGui::PopID();
    }
    ImGui::SetCursorPos({
      28+8*cellX,52+14*cellY
    });
    ImGui::Dummy({
      1,1
    });
    ImGui::EndChild();
    ImGui::End();
    if(!open)panel=None;
  }
  void sidePanel(float w,float h){
    if(panel==None||panel==Research)return;
    bool open=true;
    ImGui::SetNextWindowPos({
      16,66
    },ImGuiCond_Appearing);
    ImGui::SetNextWindowSize({
      330,std::min(h-134,610.0f)
    },ImGuiCond_Appearing);
    const char* title=panel==Build?"Construction":panel==Economy?"Economy":panel==Forces?"Forces":panel==Infrastructure?"Infrastructure":"Command log";
    ImGui::Begin(title,&open,ImGuiWindowFlags_NoCollapse);
    switch(panel){
      case Build:buildPanel();
      break;
      case Economy:economyPanel();
      break;
      case Forces:forcesPanel();
      break;
      case Infrastructure:roadsPanel();
      break;
      case Log:for(auto& event:sim.state.events)wrapped(event);
      break;
      default:break;
    }
    ImGui::End();
    if(!open)panel=None;
  }
  void unitSymbol(ImDrawList* draw,Point point,UnitType type,ImU32 color,bool selected){
    ImVec2 p=vec(point);
    float r=selected?11:9;
    draw->AddRectFilled({
      p.x-r-2,p.y-r+1
    },{
      p.x+r+2,p.y+r-1
    },IM_COL32(7,17,22,235),2);
    draw->AddRect({
      p.x-r-2,p.y-r+1
    },{
      p.x+r+2,p.y+r-1
    },color,2,0,selected?2:1.2f);
    if(type==UnitType::Infantry){
      draw->AddLine({
        p.x-r,p.y-r+3
      },{
        p.x+r,p.y+r-3
      },color);
      draw->AddLine({
        p.x+r,p.y-r+3
      },{
        p.x-r,p.y+r-3
      },color);
    }
    else if(type==UnitType::Armour)draw->AddEllipse(p,{
      r-1,4
    },color);
    else if(type==UnitType::Artillery)draw->AddCircleFilled(p,3,color);
    else if(type==UnitType::Fighter||type==UnitType::Bomber){
      draw->AddTriangleFilled({
        p.x,p.y-r+3
      },{
        p.x-r+2,p.y+r-3
      },{
        p.x+r-2,p.y+r-3
      },color);
    }
    else if(type==UnitType::Destroyer||type==UnitType::Submarine){
      draw->AddLine({
        p.x-r+2,p.y+3
      },{
        p.x+r-2,p.y+3
      },color,2);
      draw->AddTriangleFilled({
        p.x-r+2,p.y
      },{
        p.x+r-2,p.y
      },{
        p.x,p.y+6
      },color);
      if(type==UnitType::Submarine)draw->AddCircle(p,3,color);
    }
    else if(type==UnitType::SpecialForces)draw->AddQuad({
      p.x,p.y-6
    },{
      p.x+6,p.y
    },{
      p.x,p.y+6
    },{
      p.x-6,p.y
    },color);
    else draw->AddTriangleFilled({
      p.x,p.y-7
    },{
      p.x-4,p.y+6
    },{
      p.x+4,p.y+6
    },color);
  }
  void mapLabels(){
    if(screen==Menu)return;
    auto* draw=ImGui::GetBackgroundDrawList();
    draw->PushClipRect({
      camera.x,camera.y
    },{
      camera.x+camera.width,camera.y+camera.height
    },true);
    std::vector<Point> named;
    if(camera.zoom>2.5)for(size_t i=0;i<world.countries.size();++i){
      if(world.canonical(int(i))!=int(i)||world.countries[i].code=="ATA")continue;
      bool visible;
      auto point=camera.project(world.countries[i].center,&visible);
      if(!visible)continue;
      bool clear=true;
      for(auto previous:named)if(std::hypot(previous.x-point.x,previous.y-point.y)<150)clear=false;
      if(clear){
        draw->AddText({
          float(point.x)-25,float(point.y)+18
        },IM_COL32(155,177,176,145),world.countries[i].name.c_str());
        named.push_back(point);
      }
    }
    if(screen==Game){
      auto& s=sim.state;
      for(size_t i=0;i<s.sites.size();++i){
        auto& site=s.sites[i];
        if(site.destroyed)continue;
        bool visible;
        auto p=camera.project(site.position,&visible);
        if(!visible)continue;
        ImU32 color=s.regions[site.region].owner==s.player?IM_COL32(112,216,185,255):IM_COL32(219,117,103,255);
        if(!site.enabled)color=IM_COL32(130,132,133,255);
        draw->AddCircleFilled(vec(p),10,IM_COL32(10,22,26,235));
        draw->AddCircle(vec(p),10,color,16,selectedSite==int(i)?2.5f:1.2f);
        const char* symbols[]={
          "M","O","S","F","A","P","R","C","N","X","B","H","!","$","$","$"
        };
        draw->AddText({
          float(p.x)-4,float(p.y)-7
        },color,symbols[int(site.type)]);
        if(site.remaining>0){
          float progress=float(1-site.remaining/(site.type==Facility::Silo?120:35));
          draw->PathArcTo(vec(p),13,-1.5708f,-1.5708f+progress*6.28318f,20);
          draw->PathStroke(IM_COL32(234,180,84,255),0,2);
        }
        if(showRanges&&site.type==Facility::Radar&&site.remaining<=0){
          auto edge=camera.project({
            site.position.x+2,site.position.y
          });
          draw->AddCircle(vec(p),float(std::hypot(edge.x-p.x,edge.y-p.y)),IM_COL32(93,173,161,75),64,1);
        }
      }
      for(size_t i=0;i<s.units.size();++i){
        auto& unit=s.units[i];
        if(unit.health<=0)continue;
        bool visible;
        auto p=camera.project(unit.position,&visible);
        if(!visible)continue;
        bool selected=std::find(selectedUnits.begin(),selectedUnits.end(),int(i))!=selectedUnits.end();
        auto color=unit.owner==s.player?IM_COL32(135,229,211,255):IM_COL32(243,140,117,255);
        if(unit.remaining>0)color=IM_COL32(222,184,95,210);
        if(selected&&unit.moving){
          auto target=camera.project(unit.destination);
          if(std::abs(target.x-p.x)<camera.width)draw->AddLine(vec(p),vec(target),IM_COL32(128,222,200,150),1.5f);
        }
        unitSymbol(draw,p,unit.type,color,selected);
        draw->AddRectFilled({
          float(p.x)-10,float(p.y)+10
        },{
          float(p.x)-10+20*float(unit.health/100),float(p.y)+12
        },color);
        if(selected||camera.zoom>35){
          std::string text=unit.remaining>0?std::to_string(int(std::ceil(unit.remaining)))+"d":unitNames[int(unit.type)];
          draw->AddText({
            float(p.x)+14,float(p.y)-7
          },color,text.c_str());
        }
        if(selected&&showRanges&&(unit.type==UnitType::Fighter||unit.type==UnitType::Bomber||unit.type==UnitType::MissileLauncher)){
          auto edge=camera.project({
            unit.position.x+3,unit.position.y
          });
          draw->AddCircle(vec(p),float(std::hypot(edge.x-p.x,edge.y-p.y)),IM_COL32(129,215,197,75),64,1);
        }
      }
      for(size_t r=0;r<s.regions.size();++r)if(s.regions[r].owner==s.player)for(int next:s.regions[r].neighbours)if(s.regions[next].owner==s.enemy){
        bool av,bv;
        auto a=camera.project(s.regions[r].center,&av),b=camera.project(s.regions[next].center,&bv);
        if(av&&bv){
          double dx=b.x-a.x,dy=b.y-a.y,len=std::hypot(dx,dy);
          if(len>1){
            Point mid{
              (a.x+b.x)/2,(a.y+b.y)/2
            };
            double half=std::min(35.0,len*0.3);
            draw->AddLine(vec({
              mid.x-dy/len*half,mid.y+dx/len*half
            }),vec({
              mid.x+dy/len*half,mid.y-dx/len*half
            }),IM_COL32(236,168,79,210),3);
          }
        }
      }
      if(s.target>=0){
        bool visible;
        auto p=camera.project(s.regions[s.target].center,&visible);
        if(visible){
          draw->AddCircle(vec(p),24,IM_COL32(240,166,73,220),32,2);
          draw->AddText({
            float(p.x)+28,float(p.y)-7
          },IM_COL32(240,181,104,255),(std::to_string(int(s.regions[s.target].capture*100))+"% contested").c_str());
        }
      }
      if(construction>=0&&!ImGui::GetIO().WantCaptureMouse){
        auto point=camera.unproject(pointer);
        int r=point?sim.regionAt(*point):-1;
        bool valid=r>=0&&sim.buildReason(r,Facility(construction)).empty();
        auto color=valid?IM_COL32(116,232,188,220):IM_COL32(238,108,97,220);
        draw->AddCircle(vec(pointer),16,color,24,2);
        draw->AddText({
          float(pointer.x)+22,float(pointer.y)-6
        },color,facilityNames[construction]);
      }
      if(leftStarted&&ImGui::IsMouseDragging(ImGuiMouseButton_Left,8)&&construction<0){
        ImVec2 a{
          float(std::min(leftStart.x,pointer.x)),float(std::min(leftStart.y,pointer.y))
        },b{
          float(std::max(leftStart.x,pointer.x)),float(std::max(leftStart.y,pointer.y))
        };
        draw->AddRectFilled(a,b,IM_COL32(89,207,176,35));
        draw->AddRect(a,b,IM_COL32(124,225,202,210));
      }
    }
    draw->PopClipRect();
  }
  void game(float w,float h){
    hud(w,h);
    sidePanel(w,h);
    if(panel==Research)researchTree(w,h);
    regionPopup();
    unitPopup();
    if(sim.state.victory||sim.state.defeat){
      ImGui::SetNextWindowPos({
        w/2-180,70
      },ImGuiCond_Appearing);
      ImGui::Begin("Campaign result",nullptr,ImGuiWindowFlags_AlwaysAutoResize);
      wrapped(sim.state.victory?"Victory: rival territory secured":"Campaign ended in mutual destruction");
      ImGui::End();
    }
    if(ImGui::BeginPopupModal("Command pause",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
      if(button("Resume command"))ImGui::CloseCurrentPopup();
      if(button("Save and return to menu")){
        if(save()){
          screen=Menu;
          sim.state=Campaign{
          };
          panel=None;
          ImGui::CloseCurrentPopup();
        }
      }
      ImGui::EndPopup();
    }
  }
  void frame(float w,float h,double dt){
    camera.x=0;
    camera.y=52;
    camera.width=w;
    camera.height=h-98;
    if(screen==Menu){
      camera.longitude=std::remainder(camera.longitude+dt*1.2,360.0);
      mainMenu(w,h);
    }
    else{
      controls();
      mapLabels();
      if(screen==Select)selection(w,h);
      else game(w,h);
    }
    if(screen==Game&&!paused&&!sim.state.victory&&!sim.state.defeat){
      accumulator+=std::min(dt,0.25)*speed;
      int ticks=0;
      while(accumulator>=0.25&&ticks++<32){
        if(autoResearch&&sim.state.research.empty())for(size_t i=0;i<sim.techs.size();++i)if(sim.techs[i].field==sim.state.specialization&&sim.available(int(i))){
          sim.queueResearch(int(i));
          break;
        }
        sim.tick();
        accumulator-=0.25;
        if(sim.state.day-sim.state.lastAutosave>=365){
          sim.state.lastAutosave=sim.state.day;
          save(true);
        }
      }
    }
  }
};
static void screenshot(const std::filesystem::path& path,int w,int h){
  std::vector<unsigned char> pixels(size_t(w)*h*3);
  gl::PixelStorei(gl::PACK_ALIGNMENT,1);
  gl::ReadPixels(0,0,w,h,gl::RGB,gl::UNSIGNED_BYTE,pixels.data());
  std::ofstream f(path,std::ios::binary);
  f<<"P6\n"<<w<<" "<<h<<"\n255\n";
  for(int y=h-1;y>=0;--y)f.write(reinterpret_cast<const char*>(pixels.data()+size_t(y)*w*3),w*3);
}
int main(int argc,char** argv){
  bool smoke=argc>1&&std::string(argv[1])=="--smoke";
  std::filesystem::path shots=argc>2?argv[2]:"smoke";
  try{
    HeadlessContext headless;
    glfwSetErrorCallback([](int,const char* text){std::cerr<<"GLFW: "<<text<<"\n";});
    if(smoke)glfwInitHint(GLFW_PLATFORM,GLFW_PLATFORM_NULL);
    if(!glfwInit())throw std::runtime_error("Could not initialise a desktop window system");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    if(smoke){
      glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
      glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    }
    auto* window=glfwCreateWindow(1440,900,"ESCALATION - National Command",nullptr,nullptr);
    if(!window)throw std::runtime_error("OpenGL 3.3 context creation failed");
    glfwSetWindowSizeLimits(window,1080,720,GLFW_DONT_CARE,GLFW_DONT_CARE);
    if(smoke){
      headless.create(1440,900);
      gl::load(HeadlessContext::getProc);
    }
    else{
      glfwMakeContextCurrent(window);
      glfwSwapInterval(1);
      gl::load([](const char* name)->void*{return reinterpret_cast<void*>(glfwGetProcAddress(name));});
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename=nullptr;
    ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetIO().FontGlobalScale=1.15f;
    style();
    if(!(smoke?ImGui_ImplGlfw_InitForOther(window,true):ImGui_ImplGlfw_InitForOpenGL(window,true))||!ImGui_ImplOpenGL3_Init("#version 330 core"))throw std::runtime_error("UI backend initialization failed");
    World world;
    auto data=dataDirectory(argv[0]);
    world.load(data+"/world.geojson",data+"/histories.json");
    App app(std::move(world),smoke?shots:userData());
    app.renderer.initialise(app.world,data);
    std::cout<<"OpenGL "<<app.renderer.version<<" | "<<app.world.countries.size()<<" countries | "<<app.renderer.landVertices<<" cached vertices\n";
    if(smoke)std::filesystem::create_directories(shots);
    auto last=std::chrono::steady_clock::now();
    int frame=0;
    while(!glfwWindowShouldClose(window)){
      glfwPollEvents();
      auto now=std::chrono::steady_clock::now();
      double dt=std::chrono::duration<double>(now-last).count();
      last=now;
      int w,h,fw,fh;
      glfwGetWindowSize(window,&w,&h);
      glfwGetFramebufferSize(window,&fw,&fh);
      if(w<=0||h<=0||fw<=0||fh<=0)continue;
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      if(smoke){
        if(frame==2){app.screen=App::Select;app.selectedNation=app.world.find("FRA");app.nationOpen=true;app.popup={1000,200};}
        if(frame==4)app.start();
        if(frame==6){
          app.camera.globe=false;
          app.camera.zoom=8;
        }
        if(frame==8){app.panel=App::Research;app.treeFocus=true;}if(frame==10)app.panel=App::Build;if(frame==12){app.panel=App::None;app.selectedRegion=app.sim.state.capital;app.regionOpen=true;app.popup={1000,180};}if(frame==14){app.regionOpen=false;app.panel=App::Forces;}
        if(frame==14){
          app.sim.declareWar();
          for(int i=24;i<48;++i)for(int n:app.sim.state.regions[i].neighbours)if(n<24){
            app.sim.state.money=10000;
            app.sim.state.stock[2]=500;
            app.sim.connect(app.sim.state.capital,n);
            app.sim.attack(i);
          }
        }
      }
      if(smoke&&frame==16){app.panel=App::None;app.selectedUnit=0;app.selectedUnits={0};app.unitOpen=true;app.popup={1000,180};app.center(app.sim.state.units[0].position);app.camera.zoom=64;}
      if(smoke&&frame==18){app.unitOpen=false;app.panel=App::Economy;app.centerNation(app.selectedNation);}
      app.frame(float(w),float(h),smoke?0.016:dt);
      ImGui::Render();
      app.renderer.render(app.world,app.sim.state,app.camera,app.selectedNation,app.selectedRegion,fw,fh,float(fw)/w,float(fh)/h);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      if(smoke&&frame%2==1){
        screenshot(shots/(std::to_string(frame)+".ppm"),fw,fh);
        if(gl::GetError()!=0)throw std::runtime_error("OpenGL error during smoke render");
      }
      if(!smoke)glfwSwapBuffers(window);
      if(smoke&&++frame>=20)break;
    }
    if(!smoke&&app.sim.state.player>=0)app.save(true);
    app.renderer.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
  }
  catch(const std::exception& e){
    std::cerr<<"ESCALATION: "<<e.what()<<"\n";
    glfwTerminate();
    return 1;
  }
}
