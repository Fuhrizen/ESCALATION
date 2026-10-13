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
  CameraMotion cameraMotion;
  std::vector<Point> previousUnitPositions;
  enum Screen{
    Menu,Select,Game
  };
  enum Panel{
    None,Build,Research,Forces,Log
  };
  Screen screen=Menu;
  Panel panel=None;
  int selectedNation=-1,selectedRegion=-1,selectedSite=-1,selectedUnit=-1,construction=-1,focus=5,speed=1;
  bool paused=true,regionOpen=false,unitOpen=false,nationOpen=false,commandMode=false,showRanges=true,autoResearch=false,leftStarted=false,rightDragged=false,treeFocus=false;
  double accumulator=0;
  Point pointer,leftStart,popup;
  int panButton=-1;
  bool rightStarted=false;
  std::vector<int> selectedUnits;
  std::string message;
  std::filesystem::path saves;
  explicit App(World w,std::filesystem::path p):world(std::move(w)),sim(world),saves(std::move(p)){
  }
  void notice(const std::string& text){
    message=text;
  }
  void clearSelection(){
    selectedRegion=selectedSite=selectedUnit=construction=-1;
    selectedUnits.clear();
    regionOpen=unitOpen=nationOpen=commandMode=leftStarted=rightStarted=rightDragged=false;
    panButton=-1;
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
      previousUnitPositions.clear();
      accumulator=0;
      renderer.campaign(world,sim.state);
      selectedNation=sim.state.player;
      screen=Game;
      panel=None;
      paused=true;
      clearSelection();
      centerNation(selectedNation);
      notice("Campaign restored; paused");
    }
    catch(const std::exception& e){
      notice(e.what());
    }
  }
  void center(Point point){
    cameraMotion.reset();
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
    clearSelection();
    paused=true;
    accumulator=0;
    previousUnitPositions.clear();
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
      else notice("Cannot build here: choose controlled land, an unlocked building and available production capacity; shipyards need a coast.");
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
      auto p=camera.project(displayedUnit(size_t(&u-sim.state.units.data())),&visible);
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
  Point displayedUnit(size_t id) const {
    const auto& position=sim.state.units[id].position;
    if(paused||id>=previousUnitPositions.size())return position;
    auto old=previousUnitPositions[id];
    double t=std::clamp(accumulator/0.25,0.0,1.0);
    return {std::remainder(old.x+std::remainder(position.x-old.x,360.0)*t,360.0),old.y+(position.y-old.y)*t};
  }
  void controls(double dt){
    auto& io=ImGui::GetIO();
    pointer={
      io.MousePos.x,io.MousePos.y
    };
    if(!io.WantTextInput&&!io.WantCaptureKeyboard){
      if(ImGui::IsKeyPressed(ImGuiKey_G)){cameraMotion.reset();camera.globe=!camera.globe;}
      if(ImGui::IsKeyPressed(ImGuiKey_H))centerNation(selectedNation);
      if(screen==Game&&ImGui::IsKeyPressed(ImGuiKey_F)&&selectedUnit>=0&&selectedUnit<int(sim.state.units.size())){
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
    if(!io.WantTextInput&&!io.WantCaptureKeyboard){
      double horizontal=(ImGui::IsKeyDown(ImGuiKey_D)||ImGui::IsKeyDown(ImGuiKey_RightArrow))-(ImGui::IsKeyDown(ImGuiKey_A)||ImGui::IsKeyDown(ImGuiKey_LeftArrow));
      double vertical=(ImGui::IsKeyDown(ImGuiKey_W)||ImGui::IsKeyDown(ImGuiKey_UpArrow))-(ImGui::IsKeyDown(ImGuiKey_S)||ImGui::IsKeyDown(ImGuiKey_DownArrow));
      double length=std::hypot(horizontal,vertical);
      double distance=(camera.globe?114.6:360.0)*0.65*std::min(dt,0.1)/camera.zoom;
      if(length>0)cameraMotion.pan({horizontal/length*distance,vertical/length*distance});
    }
    else cameraMotion.pending={};
    if(over&&!io.WantCaptureMouse){
      if(io.MouseWheel!=0)cameraMotion.wheel(camera,pointer,io.MouseWheel);
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Right)){rightDragged=false;rightStarted=true;panButton=ImGuiMouseButton_Right;}
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle))panButton=ImGuiMouseButton_Middle;
      if(panButton>=0&&ImGui::IsMouseDragging(panButton,panButton==ImGuiMouseButton_Right?5:0)){
        if(panButton==ImGuiMouseButton_Right)rightDragged=true;
        double scale=camera.globe?114.6/camera.height:360.0/camera.width;
        cameraMotion.pan({-io.MouseDelta.x*scale/camera.zoom,io.MouseDelta.y*scale/camera.zoom});
      }
      if(rightStarted&&ImGui::IsMouseReleased(ImGuiMouseButton_Right)&&!rightDragged){
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
      if(drag>8&&screen==Game&&construction<0&&over&&!io.WantCaptureMouse){
        selectedUnits.clear();
        for(size_t i=0;i<sim.state.units.size();++i){
          auto& u=sim.state.units[i];
          if(u.owner!=sim.state.player||u.health<=0||u.remaining>0)continue;
          bool visible;
          auto p=camera.project(displayedUnit(size_t(&u-sim.state.units.data())),&visible);
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
    if(ImGui::IsMouseReleased(ImGuiMouseButton_Right))rightStarted=false;
    if(panButton>=0&&!ImGui::IsMouseDown(panButton))panButton=-1;
    cameraMotion.advance(camera,dt);
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
      ImGui::Text("Production %.1f/day | Research %.1f RP/day | %d",sim.state.productionRate,sim.state.researchRate,sim.year());
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
    if(button(camera.globe?"Globe [G]":"Map [G]")){cameraMotion.reset();camera.globe=!camera.globe;}
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
    std::string status=construction>=0?std::string("Place ")+facilityNames[construction]+" on land | Shift repeats | Right click cancels":commandMode?"Click a destination for your selected formation":message.empty()?"WASD / arrows pan | Middle drag pans | Right click orders | Wheel zooms":message;
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
    wrapped("Build military facilities. Allocate research. Move formations to capture cities and secure regions.");
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
    wrapped("Global command simulation. Technology branches, military production, city capture and responsive defensive forces.");
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
  void allocation(){
    auto& s=sim.state;
    float budget=float(s.researchBudget*100);
    wrapped("Research allocation");
    ImGui::SetNextItemWidth(-1);
    if(ImGui::SliderFloat("##researchAllocation",&budget,0,100,"%.0f%%")){s.researchBudget=budget/100;sim.refreshRates();}
    ImGui::TextColored(muted,"Research: %.1f RP/day",s.researchRate);
    ImGui::TextColored(muted,"Production: %.1f work/day",s.productionRate);
    tooltip("National output is shared between research and production. Construction, units and warheads share production capacity.");
  }
  void buildPanel(){
    heading("CONSTRUCTION");
    allocation();
    wrapped("Choose a military facility, then place it on controlled land. Facilities support production, research and strategic delivery.");
    const int types[]={
      10,11,8,9,12,6,4,7
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
      ImGui::TextColored(muted,"%.0f production work",sim.buildCost(Facility(id)));
      if(id==6)ImGui::TextColored(accent,"Produces research points");
      if(id==8)wrapped("Requires a coastal location and naval research");
      ImGui::PopID();
    }
    heading("CONSTRUCTION QUEUE");
    for(size_t i=0;i<sim.state.sites.size();++i){
      auto& site=sim.state.sites[i];
      if(site.destroyed||site.remaining<=0||sim.state.regions[site.region].owner!=sim.state.player)continue;
      ImGui::PushID(int(i)+100);
      ImGui::Text("%s | %.0f work left",facilityNames[int(site.type)],site.remaining);
      ImGui::ProgressBar(float(std::clamp(1-site.remaining/sim.buildCost(site.type),0.0,1.0)));
      if(button("Inspect")){selectedSite=int(i);selectedRegion=site.region;regionOpen=true;popup={350,90};center(site.position);}
      ImGui::SameLine();
      if(button("Cancel"))sim.cancelSite(i);
      ImGui::PopID();
    }
  }

  void production(int site){
    for(int type=0;type<9;++type){
      auto reason=sim.unitReason(UnitType(type),site);
      ImGui::PushID(type);
      std::string name=std::string(unitNames[type])+" | "+std::to_string(int(sim.unitCost(UnitType(type))))+" work";
      if(button(name.c_str(),reason.empty())){
        sim.queueUnit(UnitType(type),site);
        notice("Production queued");
      }
      tooltip(reason.empty()?"Estimated "+std::to_string(int(std::ceil(sim.unitCost(UnitType(type))/std::max(0.01,sim.state.productionRate))))+" days with current allocation (shared by active jobs)":reason);
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
    wrapped("Capture every settlement to secure this region. Move ground formations directly into the area.");
    int held=0,total=0;
    for(const auto& city:sim.state.settlements)if(city.region==selectedRegion){++total;held+=city.owner==sim.state.player;}
    ImGui::Text("Settlements controlled: %d / %d",held,total);
    for(const auto& city:sim.state.settlements)if(city.region==selectedRegion){
      ImGui::TextColored(city.owner==sim.state.player?accent:ImVec4(0.9f,0.55f,0.45f,1),"%s | %.0f residents",city.name.c_str(),city.population);
      if(city.capture>0)ImGui::ProgressBar(float(city.capture),{-1,0},"Contested");
    }
    if(r.owner==sim.state.player){
      if(button("Place a new building"))toggle(Build);
    }
    else if(!sim.state.war&&button("Declare war",!sim.state.victory&&!sim.state.defeat))sim.declareWar();
    if(selectedSite>=0){
      auto& site=sim.state.sites[selectedSite];
      heading(facilityNames[int(site.type)]);
      if(site.remaining>0){
        ImGui::Text("%.0f production work remaining",site.remaining);
        ImGui::ProgressBar(float(1-site.remaining/sim.buildCost(site.type)));
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
    },"Regional control");
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
      ImGui::Text("%.0f production work remaining",unit.remaining);
      ImGui::ProgressBar(float(1-unit.remaining/unit.total));
      if(button("Cancel production",unit.owner==sim.state.player))sim.cancelUnit(selectedUnit);
    }
    else if(unit.owner==sim.state.player){
      ImGui::Text("Selected formation: %zu units",selectedUnits.size());
      ImGui::TextColored(muted,"Road movement bonus: +%.0f%%",100*sim.roadMovementBonus(unit.position));
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
    else wrapped("Enemy formation. Defensive AI responds to threatened settlements and nearby attacks.");
    ImGui::End();
  }
  void forcesPanel(){
    heading("ARMED FORCES");
    allocation();
    wrapped("Select a formation, then right-click its destination. Ground units capture settlements automatically.");
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
    if(button("Queue warhead | 1200 work",sim.state.levels[4]>=4&&sim.state.levels[6]>=4)){
      if(!sim.produceWarhead())notice("Warhead production requires an operational factory and laboratory, with room in the 20-warhead limit");
    }
    tooltip("Requires nuclear deterrence and Computing centres research, an operational factory and laboratory. Delivery requires intercontinental missiles and an operational missile site.");
    if(sim.state.warheadsQueued>0)ImGui::Text("%d warheads queued | %.0f work left",sim.state.warheadsQueued,sim.state.warheadWork);
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
  void researchTree(float w,float h){
    bool open=true;
    ImGui::SetNextWindowSize({
      std::min(w-48,1400.0f),h-136
    },ImGuiCond_Appearing);
    ImGui::SetNextWindowPos({
      24,66
    },ImGuiCond_Appearing);
    ImGui::Begin("TECHNOLOGY COMMAND",&open,ImGuiWindowFlags_NoCollapse);
    auto& s=sim.state;
    ImGui::Text("Research %.1f RP/day | Production %.1f work/day | Focus: %s",s.researchRate,s.productionRate,fieldNames[int(s.specialization)]);
    allocation();
    ImGui::SameLine();
    ImGui::Checkbox("Auto queue doctrine",&autoResearch);
    wrapped("Research is never locked by calendar year. Advanced projects cost more; increasing research allocation slows military construction and production.");
    if(!s.research.empty()){
      auto job=s.research.front();
      double rate=s.researchRate*(sim.techs[job.tech].field==s.specialization?1.35:1);
      ImGui::Text("Active: %s | %zu queued",sim.techs[job.tech].name.c_str(),s.research.size());
      if(rate>0)ImGui::Text("~%.0f days remaining (includes doctrine bonus)",std::ceil(std::max(0.0,job.cost-job.progress)/rate));
      else ImGui::TextColored(muted,"Paused: research has no active allocation or laboratory");
      ImGui::ProgressBar(float(job.progress/job.cost),{
        300,0
      });
      ImGui::SameLine();
      if(button("Cancel active project"))sim.removeResearch(0);
    }
    else ImGui::TextColored(muted,"Click an available node to queue it. Drag scrollbars to browse the full century.");
    if(s.research.size()>1){
      ImGui::BeginChild("Research queue",{0,74},ImGuiChildFlags_Borders);
      for(size_t q=1;q<s.research.size();++q){
        ImGui::PushID(int(q));
        ImGui::Text("%zu. %s",q+1,sim.techs[s.research[q].tech].name.c_str());
        ImGui::SameLine();
        if(button("Remove")){sim.removeResearch(q);ImGui::PopID();break;}
        ImGui::PopID();
      }
      ImGui::EndChild();
    }
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
      std::string state=done?"RESEARCHED":queued?"QUEUED":std::to_string(int(sim.researchCost(int(id))))+" RP";
      draw->AddText({
        a.x+9,b.y-17
      },border,state.c_str());
      if(ImGui::IsItemClicked()){
        if(sim.queueResearch(int(id)))notice("Research queued: "+tech.name);
        else notice(sim.lockReason(int(id)));
      }
      std::string detail=tech.effect+"\nEffective cost: "+std::to_string(int(sim.researchCost(int(id))))+" RP";
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
    const char* title=panel==Build?"Construction":panel==Forces?"Forces":"Command log";
    ImGui::Begin(title,&open,ImGuiWindowFlags_NoCollapse);
    switch(panel){
      case Build:buildPanel();
      break;
      case Forces:forcesPanel();
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
  void pressureOverlay(ImDrawList* draw){
    if(!sim.state.war||camera.zoom<3)return;
    // Screen-space sampling follows individual units rather than region edges.
    // Bilinear colour fades paint pressure; marching squares traces opposition.
    const float cell=24;
    int columns=int(std::ceil(camera.width/cell))+1,rows=int(std::ceil(camera.height/cell))+1;
    struct Sample { Point screen; double value=0; bool land=false; };
    std::vector<Sample> samples(size_t(columns)*rows);
    auto color=[](double value){
      int alpha=int(std::clamp(std::abs(value)*54.0,0.0,54.0));
      return value>=0?IM_COL32(66,218,176,alpha):IM_COL32(233,104,86,alpha);
    };
    for(int y=0;y<rows;++y)for(int x=0;x<columns;++x){
      auto& sample=samples[size_t(y)*columns+x];
      sample.screen={camera.x+x*cell,camera.y+y*cell};
      auto geo=camera.unproject(sample.screen);
      if(!geo)continue;
      sample.value=sim.frontlinePressure(*geo);
      if(std::abs(sample.value)>0.001){
        sample.land=sim.regionAt(*geo)>=0;
        if(!sample.land)sample.value=0;
      }
    }
    for(int y=0;y<rows-1;++y)for(int x=0;x<columns-1;++x){
      const Sample* corners[]={&samples[size_t(y)*columns+x],&samples[size_t(y)*columns+x+1],&samples[size_t(y+1)*columns+x+1],&samples[size_t(y+1)*columns+x]};
      if(!corners[0]->land||!corners[1]->land||!corners[2]->land||!corners[3]->land)continue;
      draw->AddRectFilledMultiColor(vec(corners[0]->screen),vec(corners[2]->screen),color(corners[0]->value),color(corners[1]->value),color(corners[2]->value),color(corners[3]->value));
      std::vector<Point> crossings;
      for(int edge=0;edge<4;++edge){
        auto& a=*corners[edge];auto& b=*corners[(edge+1)%4];
        if(!a.land||!b.land||a.value*b.value>=0||std::max(std::abs(a.value),std::abs(b.value))<0.005)continue;
        double t=a.value/(a.value-b.value);
        crossings.push_back({a.screen.x+(b.screen.x-a.screen.x)*t,a.screen.y+(b.screen.y-a.screen.y)*t});
      }
      for(size_t n=1;n<crossings.size();n+=2)draw->AddLine(vec(crossings[n-1]),vec(crossings[n]),IM_COL32(246,193,108,225),2);
    }
  }
  void roadsOverlay(ImDrawList* draw){
    if(camera.zoom<6)return;
    double aspect=camera.width/camera.height;
    double radius=camera.globe?std::asin(std::min(1.0,std::hypot(aspect,1.0)/camera.zoom))*57.2957795:180.0*std::hypot(1.0,1.0/aspect)/camera.zoom;
    if(camera.globe&&std::abs(camera.latitude)+radius>=89)radius=180;
    for(int id:world.nearbyRoads({camera.longitude,camera.latitude},radius+0.3)){
      const auto& road=world.roads[size_t(id)];
      for(size_t n=1;n<road.points.size();++n){
      // Subdivide long source segments so orthographic roads follow the globe.
      Point a=road.points[n-1],b=road.points[n];
      double dx=std::remainder(b.x-a.x,360.0),dy=b.y-a.y;
      int steps=std::clamp(int(std::ceil(std::hypot(dx,dy)/0.25)),1,80);
      bool previousVisible=false;
      Point previous=camera.project(a,&previousVisible);
      for(int part=1;part<=steps;++part){
        double t=double(part)/steps;
        bool visible=false;
        Point current=camera.project({std::remainder(a.x+dx*t,360.0),a.y+dy*t},&visible);
        if(visible&&previousVisible&&std::abs(current.x-previous.x)<camera.width/2)draw->AddLine(vec(previous),vec(current),IM_COL32(196,175,132,130),camera.zoom>30?1.6f:1.0f);
        previous=current;previousVisible=visible;
      }
      }
    }
  }
  void mapLabels(){
    if(screen==Menu)return;
    auto* draw=ImGui::GetBackgroundDrawList();
    draw->PushClipRect({
      camera.x,camera.y
    },{
      camera.x+camera.width,camera.y+camera.height
    },true);
    roadsOverlay(draw);
    if(screen==Game)pressureOverlay(draw);
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
      std::vector<Point> cityLabels;
      for(const auto& city:s.settlements){
        bool visible=false;
        auto p=camera.project(city.position,&visible);
        if(!visible)continue;
        auto color=city.owner==s.player?IM_COL32(148,232,199,255):IM_COL32(241,160,129,255);
        float radius=city.population>500000?4.0f:3.0f;
        draw->AddCircleFilled(vec(p),radius+2,IM_COL32(9,17,22,245));
        draw->AddCircleFilled(vec(p),radius,color);
        if(city.capture>0){
          draw->PathArcTo(vec(p),radius+5,-1.5708f,-1.5708f+float(city.capture)*6.28318f,24);
          draw->PathStroke(IM_COL32(248,202,98,245),0,2);
        }
        bool clear=camera.zoom>8&&(city.population>0||city.region==selectedRegion||camera.zoom>60);
        for(auto previous:cityLabels)if(std::hypot(previous.x-p.x,previous.y-p.y)<100)clear=false;
        if(clear){
          draw->AddText({float(p.x)+8,float(p.y)-7},color,city.name.c_str());
          cityLabels.push_back(p);
        }
      }
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
          float progress=float(1-site.remaining/sim.buildCost(site.type));
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
        auto p=camera.project(displayedUnit(i),&visible);
        if(!visible)continue;
        bool selected=std::find(selectedUnits.begin(),selectedUnits.end(),int(i))!=selectedUnits.end();
        auto color=unit.owner==s.player?IM_COL32(135,229,211,255):IM_COL32(243,140,117,255);
        if(unit.remaining>0)color=IM_COL32(222,184,95,210);
        if(selected&&unit.moving){
          Point previous=p;
          bool previousVisible=true;
          for(auto waypoint:unit.waypoints){
            bool visible=false;auto target=camera.project(waypoint,&visible);
            if(visible&&previousVisible&&std::abs(target.x-previous.x)<camera.width/2)draw->AddLine(vec(previous),vec(target),IM_COL32(128,222,200,150),1.5f);
            previous=target;previousVisible=visible;
          }
        }
        unitSymbol(draw,p,unit.type,color,selected);
        draw->AddRectFilled({
          float(p.x)-10,float(p.y)+10
        },{
          float(p.x)-10+20*float(unit.health/100),float(p.y)+12
        },color);
        bool hovered=!ImGui::GetIO().WantCaptureMouse&&std::hypot(pointer.x-p.x,pointer.y-p.y)<14;
        if(selected||hovered){
          std::string text=unit.remaining>0?std::to_string(int(std::ceil(unit.remaining)))+" work":unitNames[int(unit.type)];
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
      wrapped(sim.state.victory?"Victory: rival territory secured":sim.state.nuclearUsed?"Campaign ended in mutual destruction":"Defeat: all home regions have been captured");
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
          clearSelection();
          previousUnitPositions.clear();
          accumulator=0;
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
      if(screen==Game)sim.refreshRates();
      controls(dt);
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
        previousUnitPositions.clear();
        for(auto& unit:sim.state.units)previousUnitPositions.push_back(unit.position);
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
          for(size_t n=0;n<app.sim.techs.size();++n)if(app.sim.available(int(n))){app.sim.queueResearch(int(n));break;}
          for(size_t n=0;n<app.sim.state.sites.size();++n)if(app.sim.state.sites[n].type==Facility::ArmyBase){app.sim.queueUnit(UnitType::Infantry,int(n));break;}
          if(!app.sim.state.units.empty()){
            for(const auto& city:app.sim.state.settlements)if(city.owner==app.sim.state.enemy){
              app.sim.orderUnitAt(0,city.position);
              break;
            }
          }
          app.sim.tick();
        }
      }
      if(smoke&&frame==16){app.panel=App::None;app.selectedUnit=0;app.selectedUnits={0};app.unitOpen=true;app.popup={1000,180};app.center(app.sim.state.units[0].position);app.camera.zoom=64;}
      if(smoke&&frame==18){
        app.unitOpen=false;app.panel=App::None;
        // A compact frontline scene exercises cities, source road data, pressure
        // paint and contested-settlement progress independently of AI outcomes.
        for(auto& city:app.sim.state.settlements)if(city.owner==app.sim.state.enemy){
          app.center(city.position);app.camera.zoom=40;city.capture=0.42;
          int friendly=-1,enemy=-1;
          for(size_t n=0;n<app.sim.state.units.size();++n){
            auto& unit=app.sim.state.units[n];
            if(unit.remaining>0||unit.type!=UnitType::Infantry)continue;
            if(unit.owner==app.sim.state.player&&friendly<0)friendly=int(n);
            if(unit.owner==app.sim.state.enemy&&enemy<0)enemy=int(n);
          }
          if(friendly>=0){auto& unit=app.sim.state.units[friendly];unit.position={city.position.x-0.28,city.position.y};unit.moving=false;}
          if(enemy>=0){auto& unit=app.sim.state.units[enemy];unit.position={city.position.x+0.28,city.position.y};unit.moving=false;}
          break;
        }
      }
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
