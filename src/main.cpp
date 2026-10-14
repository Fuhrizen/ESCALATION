#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "ui_icons.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "escalation/renderer.hpp"
#include "escalation/headless_context.hpp"
#include "escalation/json.hpp"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <functional>
#ifndef ESCALATION_DATA_DIR
#define ESCALATION_DATA_DIR "data"
#endif
using namespace escalation;
static ImFont* titleFont=nullptr;
static ImVec4 muted(0.70f,0.78f,0.83f,1),accent(0.25f,0.66f,1.0f,1);
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
  if(titleFont)ImGui::PushFont(titleFont);
  ImGui::PushTextWrapPos(0);
  ImGui::TextColored(accent,"%s",text);
  ImGui::PopTextWrapPos();
  if(titleFont)ImGui::PopFont();
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
  s.WindowRounding=4;
  s.FrameRounding=3;
  s.ScrollbarRounding=3;
  s.WindowPadding={16,14};
  s.FramePadding={8,8};
  s.ItemSpacing={8,8};
  s.Colors[ImGuiCol_WindowBg]={0.025f,0.06f,0.10f,1};
  s.Colors[ImGuiCol_PopupBg]={0.045f,0.065f,0.085f,0.99f};
  s.Colors[ImGuiCol_TitleBg]={0.045f,0.065f,0.085f,1};
  s.Colors[ImGuiCol_TitleBgActive]={0.075f,0.115f,0.145f,1};
  s.Colors[ImGuiCol_TitleBgCollapsed]={0.045f,0.065f,0.085f,1};
  s.Colors[ImGuiCol_FrameBg]={0.09f,0.14f,0.17f,1};
  s.Colors[ImGuiCol_Button]={0.055f,0.19f,0.32f,1};
  s.Colors[ImGuiCol_ButtonHovered]={0.08f,0.32f,0.53f,1};
  s.Colors[ImGuiCol_Header]={0.055f,0.22f,0.37f,1};
  s.Colors[ImGuiCol_CheckMark]=accent;
  s.Colors[ImGuiCol_PlotHistogram]=accent;
  s.Colors[ImGuiCol_Text]={0.93f,0.96f,0.97f,1};
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
static std::string durationText(double seconds){
  if(!std::isfinite(seconds))return "Paused";
  if(seconds<60)return std::to_string(int(std::ceil(std::max(0.0,seconds))))+"s";
  int total=int(std::ceil(seconds));return std::to_string(total/60)+"m "+std::to_string(total%60)+"s";
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
  int selectedNation=-1,selectedRegion=-1,selectedSite=-1,selectedSettlement=-1,selectedWorldCity=-1,selectedUnit=-1,construction=-1,readyUnitPlacement=-1,focus=5,speed=1;
  bool paused=true,regionOpen=false,unitOpen=false,nationOpen=false,commandMode=false,showRanges=true,autoResearch=false,leftStarted=false,rightDragged=false,treeFocus=false;
  int attackMode=0; // 1: unit point attack, 2: conventional silo, 3: confirmed nuclear silo
  double accumulator=0;
  Point pointer,leftStart,popup;
  int hoverUnit=-1,hoverSite=-1;
  bool pauseOpen=false,wasPaused=true,showCities=true,showRoads=true,showBorders=true,showRivers=true,showPressure=true,windowFocused=true;
  float panSpeed=1.8f,dragSpeed=1.0f,zoomSensitivity=1.0f,uiScale=1.0f;
  int panButton=-1;
  bool rightStarted=false;
  MapGesture leftGesture,rightGesture;
  std::vector<int> selectedUnits;
  std::array<std::vector<int>,10> controlGroups;
  bool autoUiScale=true,leftDoubleClick=false;
  std::string message;
  std::filesystem::path saves;
  explicit App(World w,std::filesystem::path p):world(std::move(w)),sim(world),saves(std::move(p)){
    loadSettings();
  }
  void loadSettings(){
    try{auto j=readJson((saves/"settings.json").string());
      auto value=[&](const char* key,float fallback){return j.contains(key)&&j.get(key).is<double>()?float(j.get(key).get<double>()):fallback;};
      panSpeed=std::clamp(value("panSpeed",panSpeed),.5f,5.f);dragSpeed=std::clamp(value("dragSpeed",dragSpeed),.5f,3.f);zoomSensitivity=std::clamp(value("zoomSensitivity",zoomSensitivity),.5f,3.f);uiScale=std::clamp(value("uiScale",uiScale),.85f,1.4f);
      auto boolean=[&](const char* key,bool fallback){return j.contains(key)&&j.get(key).is<bool>()?j.get(key).get<bool>():fallback;};
      autoUiScale=boolean("autoUiScale",!j.contains("uiScale"));showRanges=boolean("showRanges",true);showCities=boolean("showCities",true);showRoads=boolean("showRoads",true);showBorders=boolean("showBorders",true);showRivers=boolean("showRivers",true);showPressure=boolean("showPressure",true);camera.terrain=boolean("terrain",true);camera.globe=boolean("globe",true);
    }catch(const std::exception&){}
  }
  void saveSettings(){
    try{std::filesystem::create_directories(saves);Object j;
      j["panSpeed"]=num(panSpeed);j["dragSpeed"]=num(dragSpeed);j["zoomSensitivity"]=num(zoomSensitivity);j["uiScale"]=num(uiScale);j["autoUiScale"]=Json(autoUiScale);
      j["showRanges"]=Json(showRanges);j["showCities"]=Json(showCities);j["showRoads"]=Json(showRoads);j["showBorders"]=Json(showBorders);j["showRivers"]=Json(showRivers);j["showPressure"]=Json(showPressure);j["terrain"]=Json(camera.terrain);j["globe"]=Json(camera.globe);
      std::ofstream f(saves/"settings.json");f<<Json(j).serialize(true);
    }catch(const std::exception& e){notice(e.what());}
  }
  void cancelGestures(){leftStarted=rightStarted=rightDragged=false;panButton=-1;leftGesture.cancel();rightGesture.cancel();cameraMotion.reset();}
  void openPause(){wasPaused=paused;paused=true;pauseOpen=true;cancelGestures();}
  void notice(const std::string& text){
    message=text;
  }
  void clearSelection(){
    selectedRegion=selectedSite=selectedSettlement=selectedWorldCity=selectedUnit=construction=readyUnitPlacement=-1;
    attackMode=0;
    selectedUnits.clear();
    for(auto& group:controlGroups)group.clear();
    orderPreviews.clear();
    regionOpen=unitOpen=nationOpen=commandMode=leftStarted=rightStarted=rightDragged=false;
    cancelGestures();
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
    cancelGestures();construction=readyUnitPlacement=-1;commandMode=false;attackMode=0;
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
    if(attackMode){
      bool issued=false;std::string reason;
      if(attackMode==1){for(int id:selectedUnits){reason=sim.unitAttackReason(size_t(id),geo);issued=sim.orderUnitAttackAt(size_t(id),geo)||issued;}}
      else if(selectedSite>=0){reason=sim.missileStrikeReason(size_t(selectedSite),geo,attackMode==3);issued=sim.launchMissile(size_t(selectedSite),geo,attackMode==3);}
      notice(issued?"Strike launched":reason.empty()?"Select a ready launcher first":reason);
      if(issued)attackMode=0;
      return;
    }
    if(readyUnitPlacement>=0){
      auto type=UnitType(readyUnitPlacement);
      auto reason=sim.deploymentReason(type,geo);
      if(sim.deployUnit(type,geo)){
        notice(std::string("Deployed ")+unitNames[int(type)]);
        if(!ImGui::GetIO().KeyShift||sim.readyUnitCount(type)==0)readyUnitPlacement=-1;
      }else notice(reason);
      return;
    }
    if(construction>=0){
      int region=sim.regionAt(geo);
      if(sim.placeBuilding(Facility(construction),region,geo)){
        notice(std::string("Placed ")+facilityNames[construction]);
        if(!ImGui::GetIO().KeyShift||sim.state.buildingInventory[construction]==0)construction=-1;
      }
      else notice("Placement blocked: choose controlled land with a free building slot; shipyards need a coast.");
      return;
    }
    if(commandMode&&!selectedUnits.empty()){
      bool issued=false;
      for(int id:selectedUnits)issued=sim.orderUnitAt(size_t(id),geo,ImGui::GetIO().KeyShift)||issued;
      notice(issued?"Movement / attack order issued":"Order unavailable: check war status, land access and unit readiness");
      commandMode=false;
      return;
    }
    selectedUnit=-1;
    double nearest=17;
    for(size_t i=0;i<sim.state.units.size();++i){
      auto& u=sim.state.units[i];
      if(u.health<=0||!u.deployed||u.remaining>0)continue;
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
      auto found=std::find(selectedUnits.begin(),selectedUnits.end(),selectedUnit);
      if(ImGui::GetIO().KeyShift&&found!=selectedUnits.end()){selectedUnits.erase(found);selectedUnit=selectedUnits.empty()?-1:selectedUnits.front();}
      else if(sim.state.units[selectedUnit].owner==sim.state.player)selectedUnits.push_back(selectedUnit);
      if(selectedUnit>=0&&leftDoubleClick&&sim.state.units[selectedUnit].owner==sim.state.player){
        auto type=sim.state.units[selectedUnit].type;
        for(size_t i=0;i<sim.state.units.size();++i){const auto& u=sim.state.units[i];bool visible=false;auto p=camera.project(u.position,&visible);if(u.owner==sim.state.player&&u.deployed&&u.type==type&&u.health>0&&u.remaining<=0&&visible&&p.x>=camera.x&&p.x<camera.x+camera.width&&p.y>=camera.y&&p.y<camera.y+camera.height&&std::find(selectedUnits.begin(),selectedUnits.end(),int(i))==selectedUnits.end())selectedUnits.push_back(int(i));}
      }
      selectedSite=selectedSettlement=selectedWorldCity=-1;selectedRegion=-1;
      unitOpen=selectedUnit>=0;
      regionOpen=false;
      popup=pointer;
      return;
    }
    selectedUnits.clear();
    unitOpen=false;
    selectedRegion=sim.regionAt(geo);
    selectedSite=selectedSettlement=selectedWorldCity=-1;
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
    if(selectedSite<0&&showCities){
      selectedSettlement=cityPick(pointer);
      if(selectedSettlement>=0)selectedRegion=sim.state.settlements[selectedSettlement].region;
      else {selectedWorldCity=worldCityPick(pointer);if(selectedWorldCity>=0)selectedRegion=-1;}
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
  struct OrderPreview {size_t fingerprint=0;double eta=0;std::vector<Point> path;};
  std::map<size_t,OrderPreview> orderPreviews;
  const OrderPreview& orderPreview(size_t id){
    auto& cached=orderPreviews[id];const auto& unit=sim.state.units[id];
    size_t signature=std::hash<int>{}(sim.state.day);
    auto mix=[&](double value){signature^=std::hash<double>{}(value)+0x9e3779b9+(signature<<6)+(signature>>2);};
    mix(unit.position.x);mix(unit.position.y);mix(unit.destination.x);mix(unit.destination.y);mix(unit.health);mix(unit.remaining);mix(unit.moving);mix(unit.type==UnitType::Infantry?0:int(unit.type));
    for(auto point:unit.waypoints){mix(point.x);mix(point.y);}mix(unit.waypoints.size());
    for(auto point:unit.pendingDestinations){mix(point.x);mix(point.y);}mix(unit.pendingDestinations.size());
    if(cached.fingerprint!=signature){cached.fingerprint=signature;cached.eta=sim.unitArrivalETA(id);cached.path=sim.unitPlannedPath(id);}
    return cached;
  }
  double arrivalETA(size_t id){return orderPreview(id).eta;}
  void controls(double dt){
    selectedUnits.erase(std::remove_if(selectedUnits.begin(),selectedUnits.end(),[&](int id){return id<0||id>=int(sim.state.units.size())||sim.state.units[id].health<=0||!sim.state.units[id].deployed;}),selectedUnits.end());
    if(selectedUnit>=0&&(selectedUnit>=int(sim.state.units.size())||sim.state.units[selectedUnit].health<=0||!sim.state.units[selectedUnit].deployed)){
      selectedUnit=selectedUnits.empty()?-1:selectedUnits.front();unitOpen=selectedUnit>=0;notice("Selected formation lost");
    }
    auto& io=ImGui::GetIO();
    pointer={
      io.MousePos.x,io.MousePos.y
    };
    bool popupOpen=ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId);
    bool blocked=pauseOpen||popupOpen||!windowFocused;
    if(blocked)cancelGestures();
    bool keyboardAllowed=!io.WantTextInput&&!blocked;
    if(keyboardAllowed){
      if(ImGui::IsKeyPressed(ImGuiKey_G)){cameraMotion.reset();camera.globe=!camera.globe;}
      if(ImGui::IsKeyPressed(ImGuiKey_H))centerNation(selectedNation);
      if(screen==Game&&ImGui::IsKeyPressed(ImGuiKey_F)&&selectedUnit>=0&&selectedUnit<int(sim.state.units.size())){
        center(sim.state.units[selectedUnit].position);
        camera.zoom=std::max(camera.zoom,40.0);
      }
      if(screen==Game&&panel==None){
        const ImGuiKey groupKeys[]={ImGuiKey_0,ImGuiKey_1,ImGuiKey_2,ImGuiKey_3,ImGuiKey_4,ImGuiKey_5,ImGuiKey_6,ImGuiKey_7,ImGuiKey_8,ImGuiKey_9};
        for(int n=0;n<10;++n)if(ImGui::IsKeyPressed(groupKeys[n],false)){
          if(io.KeyCtrl){controlGroups[n]=selectedUnits;notice("Control group "+std::to_string(n)+" assigned");}
          else{
            if(!io.KeyShift)selectedUnits.clear();
            for(int id:controlGroups[n])if(id>=0&&id<int(sim.state.units.size())&&sim.state.units[id].owner==sim.state.player&&sim.state.units[id].deployed&&sim.state.units[id].health>0&&std::find(selectedUnits.begin(),selectedUnits.end(),id)==selectedUnits.end())selectedUnits.push_back(id);
            selectedUnit=selectedUnits.empty()?-1:selectedUnits.front();selectedSite=selectedRegion=selectedSettlement=selectedWorldCity=-1;unitOpen=selectedUnit>=0;
          }
        }
        if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_A,false)){
          selectedUnits.clear();for(size_t i=0;i<sim.state.units.size();++i){const auto& u=sim.state.units[i];bool visible=false;auto p=camera.project(u.position,&visible);if(u.owner==sim.state.player&&u.deployed&&u.health>0&&u.remaining<=0&&visible&&p.x>=camera.x&&p.x<camera.x+camera.width&&p.y>=camera.y&&p.y<camera.y+camera.height)selectedUnits.push_back(int(i));}
          selectedUnit=selectedUnits.empty()?-1:selectedUnits.front();selectedSite=selectedRegion=selectedSettlement=selectedWorldCity=-1;unitOpen=selectedUnit>=0;
        }
        if(ImGui::IsKeyPressed(ImGuiKey_X,false)){for(int id:selectedUnits)sim.holdUnit(size_t(id));notice("Selected formations stopped");}
      }
      if(screen==Game){
        if(ImGui::IsKeyPressed(ImGuiKey_Space))paused=!paused;
        if(ImGui::IsKeyPressed(ImGuiKey_B))toggle(Build);
        if(ImGui::IsKeyPressed(ImGuiKey_R))toggle(Research);
        if(ImGui::IsKeyPressed(ImGuiKey_U))toggle(Forces);
        if(ImGui::IsKeyPressed(ImGuiKey_F5))save();
        if(ImGui::IsKeyPressed(ImGuiKey_F9))resume();
      }
    }
    if(ImGui::IsKeyPressed(ImGuiKey_Escape)&&screen==Game&&windowFocused&&(pauseOpen||!popupOpen)){
      if(pauseOpen){pauseOpen=false;paused=wasPaused;saveSettings();}
      else if(construction>=0||readyUnitPlacement>=0||commandMode||attackMode){construction=readyUnitPlacement=-1;commandMode=false;attackMode=0;}
      else if(panel!=None){panel=None;cancelGestures();}
      else openPause();
    }
    // Escape can open the pause modal during this very frame.
    blocked=pauseOpen||popupOpen||!windowFocused;
    bool mapBlocked=blocked||(screen==Game&&panel!=None);
    if(mapBlocked)cancelGestures();
    keyboardAllowed=!io.WantTextInput&&!mapBlocked;
    bool over=pointer.x>=camera.x&&pointer.x<camera.x+camera.width&&pointer.y>=camera.y&&pointer.y<camera.y+camera.height&&!(screen==Game&&mapOverlayHit(pointer,io.DisplaySize.x,io.DisplaySize.y));
    if(keyboardAllowed&&!io.KeyCtrl){
      double horizontal=(ImGui::IsKeyDown(ImGuiKey_D)||ImGui::IsKeyDown(ImGuiKey_RightArrow))-(ImGui::IsKeyDown(ImGuiKey_A)||ImGui::IsKeyDown(ImGuiKey_LeftArrow));
      double vertical=(ImGui::IsKeyDown(ImGuiKey_W)||ImGui::IsKeyDown(ImGuiKey_UpArrow))-(ImGui::IsKeyDown(ImGuiKey_S)||ImGui::IsKeyDown(ImGuiKey_DownArrow));
      double length=std::hypot(horizontal,vertical);
      double distance=(camera.globe?114.6:360.0)*panSpeed*std::min(dt,0.1)/camera.zoom;
      if(length>0)cameraMotion.directPan(camera,{horizontal/length*distance,vertical/length*distance});
    }
    else cameraMotion.pending={};
    leftGesture.observe(pointer);rightGesture.observe(pointer);
    if(rightGesture.dragged(5))rightDragged=true;
    bool mapInput=over&&!io.WantCaptureMouse&&!mapBlocked;
    if(mapInput){
      if(io.MouseWheel!=0)cameraMotion.wheel(camera,pointer,io.MouseWheel*zoomSensitivity);
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Right)){rightDragged=false;rightStarted=true;rightGesture.begin(pointer);panButton=ImGuiMouseButton_Right;}
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle))panButton=ImGuiMouseButton_Middle;
      if(rightStarted&&ImGui::IsMouseReleased(ImGuiMouseButton_Right)&&!rightDragged){
        if(construction>=0||readyUnitPlacement>=0||attackMode){
          construction=readyUnitPlacement=-1;attackMode=0;
          notice("Construction placement cancelled");
        }
        else if(screen==Game&&!selectedUnits.empty()){
          auto geo=camera.unproject(pointer);
          if(geo){
            bool issued=false;
            int targetRegion=sim.regionAt(*geo);
            bool hostileLand=targetRegion>=0&&sim.state.regions[targetRegion].owner==sim.state.enemy;
            for(int id:selectedUnits){
              bool strike=hostileLand&&!io.KeyShift&&sim.unitAttackReason(size_t(id),*geo).empty();
              issued=(strike?sim.orderUnitAttackAt(size_t(id),*geo):sim.orderUnitAt(size_t(id),*geo,io.KeyShift))||issued;
            }
            notice(issued?(io.KeyShift?"Orders queued":"Orders issued"):"Order blocked: check access, readiness and war status");
          }
        }
      }
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        leftStarted=true;leftDoubleClick=ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
        leftStart=pointer;leftGesture.begin(pointer);
      }
    }
    // Continue a map-owned pan across inspector/management docks.
    if(!mapBlocked&&panButton>=0&&ImGui::IsMouseDown(panButton)&&(panButton==ImGuiMouseButton_Middle||rightDragged)){
      double scale=camera.globe?114.6/camera.height:360.0/camera.width;
      cameraMotion.drag(camera,{-io.MouseDelta.x*scale/camera.zoom*dragSpeed,io.MouseDelta.y*scale/camera.zoom*dragSpeed});
    }
    if(leftStarted&&ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
      leftStarted=false;
      bool dragged=leftGesture.dragged(8);
      if(dragged&&screen==Game&&construction<0&&readyUnitPlacement<0&&attackMode==0&&mapInput){
        if(!io.KeyShift)selectedUnits.clear();
        for(size_t i=0;i<sim.state.units.size();++i){
          auto& u=sim.state.units[i];
          if(u.owner!=sim.state.player||!u.deployed||u.health<=0||u.remaining>0)continue;
          bool visible;
          auto p=camera.project(displayedUnit(size_t(&u-sim.state.units.data())),&visible);
          if(visible&&p.x>=std::min(leftStart.x,pointer.x)&&p.x<=std::max(leftStart.x,pointer.x)&&p.y>=std::min(leftStart.y,pointer.y)&&p.y<=std::max(leftStart.y,pointer.y)){auto found=std::find(selectedUnits.begin(),selectedUnits.end(),int(i));if(found==selectedUnits.end())selectedUnits.push_back(int(i));else if(io.KeyShift)selectedUnits.erase(found);}
        }
        selectedUnit=selectedUnits.empty()?-1:selectedUnits.front();selectedSite=selectedSettlement=selectedWorldCity=-1;selectedRegion=-1;
        if(!selectedUnits.empty()){
          selectedUnit=selectedUnits.front();
          unitOpen=true;
          regionOpen=false;
          popup=pointer;
          notice("Formation selected; right click to order");
        }
      }
      else if(!dragged&&mapInput){
        auto geo=camera.unproject(pointer);
        if(geo)selectPoint(*geo);
      }
    }
    if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))leftGesture.cancel();
    if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)){rightStarted=false;rightGesture.cancel();}
    if(panButton>=0&&!ImGui::IsMouseDown(panButton))panButton=-1;
    cameraMotion.advance(camera,dt);
    hoverUnit=hoverSite=-1;
    if(screen==Game&&mapInput){
      double nearest=20;
      for(size_t i=0;i<sim.state.units.size();++i){auto& u=sim.state.units[i];if(u.health<=0||!u.deployed||u.remaining>0)continue;bool visible;auto p=camera.project(displayedUnit(i),&visible);double d=std::hypot(p.x-pointer.x,p.y-pointer.y);if(visible&&d<nearest){nearest=d;hoverUnit=int(i);}}
      for(size_t i=0;i<sim.state.sites.size();++i){auto& site=sim.state.sites[i];if(site.destroyed)continue;bool visible;auto p=camera.project(site.position,&visible);double d=std::hypot(p.x-pointer.x,p.y-pointer.y);if(visible&&d<nearest){nearest=d;hoverSite=int(i);hoverUnit=-1;}}
    }
  }
#include "ui_hud.inl"
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
    ImGui::SetNextWindowPos({camera.x+camera.width-340,60},ImGuiCond_Always);
    ImGui::SetNextWindowSize({340,height},ImGuiCond_Always);
    ImGui::Begin(name,&open,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
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
    ImGui::Combo("Doctrine",&focus,fieldNames.data(),int(fieldNames.size()));
    if(ImGui::Button("Take command",{
      -1,34
    }))start();
    if(button("Zoom to country"))centerNation(selectedNation);
    ImGui::End();
  }
#include "ui_panels.inl"
#include "ui_research.inl"
#include "ui_map.inl"
  void game(float w,float h){
    hud(w,h);
    sidePanel(w,h);
    if(panel==Research)researchTree(w,h);
    inspector(w,h);
    navigation(h);
    if(sim.state.victory||sim.state.defeat){
      ImGui::SetNextWindowPos({
        w/2-180,70
      },ImGuiCond_Appearing);
      ImGui::Begin("Campaign result",nullptr,ImGuiWindowFlags_AlwaysAutoResize);
      wrapped(sim.state.victory?"Victory: rival territory secured":sim.state.nuclearUsed?"Campaign ended in mutual destruction":"Defeat: all home regions have been captured");
      ImGui::End();
    }
    pauseMenu(w,h);
  }
  void frame(float w,float h,double dt){
    if(autoUiScale)uiScale=effectiveUiScale(w,h);
    ImGui::GetIO().FontGlobalScale=uiScale;
    static const ImGuiStyle baseStyle=ImGui::GetStyle();ImGui::GetStyle()=baseStyle;ImGui::GetStyle().ScaleAllSizes(uiScale);
    camera.x=0;
    camera.y=screen==Game?hudHeight():60;
    camera.borders=showBorders;
    camera.width=w;
    camera.height=h-camera.y-(screen==Game?commandHeight():58);
    ImGui::GetIO().FontGlobalScale=uiScale;
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
    const int initialWidth=smoke&&argc>3?std::clamp(std::stoi(argv[3]),1080,3840):1440;
    const int initialHeight=smoke&&argc>4?std::clamp(std::stoi(argv[4]),720,2160):900;
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
    auto* window=glfwCreateWindow(initialWidth,initialHeight,"ESCALATION - National Command",nullptr,nullptr);
    if(!window)throw std::runtime_error("OpenGL 3.3 context creation failed");
    if(!smoke)glfwSetWindowSizeLimits(window,1080,720,GLFW_DONT_CARE,GLFW_DONT_CARE);
    if(smoke){
      headless.create(initialWidth,initialHeight);
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
    ImGui::GetIO().FontGlobalScale=1.0f;
    static const ImWchar labelGlyphs[]={0x20,0x024F,0x1E00,0x1EFF,0x2000,0x206F,0};
    auto fontPath=dataDirectory(argv[0])+"/fonts/DejaVuSans.ttf";
    if(std::filesystem::exists(fontPath))ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.c_str(),17.0f,nullptr,labelGlyphs);
    auto titlePath=dataDirectory(argv[0])+"/fonts/DejaVuSans-Bold.ttf";
    if(std::filesystem::exists(titlePath))titleFont=ImGui::GetIO().Fonts->AddFontFromFileTTF(titlePath.c_str(),18.0f,nullptr,labelGlyphs);
    style();
    if(!(smoke?ImGui_ImplGlfw_InitForOther(window,true):ImGui_ImplGlfw_InitForOpenGL(window,true))||!ImGui_ImplOpenGL3_Init("#version 330 core"))throw std::runtime_error("UI backend initialization failed");
    World world;
    auto data=dataDirectory(argv[0]);
    world.load(data+"/world.geojson",data+"/histories.json");
    App app(std::move(world),smoke?shots:userData());
    app.renderer.initialise(app.world,data);
    if(smoke&&argc>5){app.autoUiScale=false;app.uiScale=std::clamp(std::stof(argv[5]),.85f,1.4f);}
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
        if(frame==4){app.start();app.sim.state.buildingInventory[int(Facility::ArmyBase)]=1;app.sim.state.buildingInventory[int(Facility::Radar)]=2;Unit ready=app.sim.state.units[0];ready.deployed=false;ready.moving=false;ready.path.clear();ready.waypoints.clear();ready.pendingDestinations.clear();app.sim.state.units.push_back(ready);app.sim.state.units.push_back(ready);}
        if(frame==6){
          app.camera.globe=false;
          app.camera.zoom=8;
        }
        if(frame==8){app.focus=0;app.panel=App::Research;app.researchSelected=-1;app.treeFocus=true;for(int id:{0,1,2,3,5})app.sim.state.completed[id]=true;}if(frame==10)app.panel=App::Build;if(frame==12){app.panel=App::None;app.selectedRegion=app.sim.state.capital;app.regionOpen=true;app.popup={1000,180};}if(frame==14){app.regionOpen=false;app.panel=App::Forces;}
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
      if(smoke&&frame==20){app.panel=App::None;app.selectedUnit=0;app.selectedUnits={0};app.center(app.sim.state.units[0].position);app.camera.zoom=64;ImGui::GetIO().AddMousePosEvent(float(app.camera.project(app.sim.state.units[0].position).x),float(app.camera.project(app.sim.state.units[0].position).y));}
      if(smoke&&frame==22){app.openPause();}
      if(smoke&&frame==24){app.pauseOpen=false;app.panel=App::Build;app.sim.queueBuilding(Facility::ArmyBase);app.sim.state.buildingInventory[int(Facility::Radar)]=1;}
      if(smoke&&frame==26){app.panel=App::None;app.sim.state.buildingInventory[int(Facility::ArmyBase)]=1;app.construction=int(Facility::ArmyBase);}
      if(smoke&&frame==28){
        app.clearSelection();app.panel=App::None;app.construction=-1;app.camera.globe=false;
        for(size_t i=0;i<app.world.cities.size();++i)if(app.world.cities[i].name=="Paris"&&app.world.cities[i].countryCode=="FRA"){
          app.selectedWorldCity=int(i);app.center(app.world.cities[i].position);app.camera.zoom=144;
          for(size_t j=0;j<app.sim.state.settlements.size();++j)if(app.sim.state.settlements[j].name=="Paris"){app.selectedSettlement=int(j);app.selectedWorldCity=-1;break;}break;
        }
      }
      if(smoke&&frame==30){
        app.clearSelection();app.panel=App::None;
        for(int id=112;id<122;++id)app.sim.state.completed[id]=true;
        app.sim.state.buildingInventory[int(Facility::Shipyard)]=1;
        Point land{},water{};int region=-1;
        for(const auto& polygon:app.world.countries[app.sim.state.player].polygons){if(region>=0)break;if(polygon.rings.empty())continue;for(Point v:polygon.rings.front()){
          if(region>=0)break;
          for(int a=0;a<32&&region<0;++a){double angle=a*6.28318530718/32;Point candidate{v.x+.015*std::cos(angle),v.y+.015*std::sin(angle)};int r=app.sim.regionAt(candidate);if(r<0||!app.sim.placementReason(Facility::Shipyard,r,candidate).empty())continue;
            for(int b=0;b<32;++b){double theta=b*6.28318530718/32;Point sea{candidate.x+.06*std::cos(theta),candidate.y+.06*std::sin(theta)};if(app.world.pick(sea)<0&&app.world.canonical(nearestCoastNation(app.world,sea))==app.sim.state.player){region=r;land=candidate;water=sea;break;}}
          }
        }}
        if(region>=0&&app.sim.placeBuilding(Facility::Shipyard,region,land)){
          int yard=int(app.sim.state.sites.size())-1;
          if(app.sim.queueUnit(UnitType::Destroyer,yard)){int ship=int(app.sim.state.units.size())-1;app.sim.state.units[ship].remaining=0;
            if(app.sim.deployUnit(UnitType::Destroyer,water)){app.selectedUnit=ship;app.selectedUnits={ship};app.center(water);app.camera.zoom=22;}
          }
          app.sim.queueUnit(UnitType::Submarine,yard);
        }
      }
      if(smoke&&frame==32){app.clearSelection();app.panel=App::Research;app.focus=8;app.treeFocus=true;}
      if(smoke&&frame==34){
        app.clearSelection();app.panel=App::None;app.camera.globe=false;app.sim.state.completed[58]=true;app.sim.state.war=true;
        int source=app.sim.state.capital;Site silo;silo.type=Facility::Silo;silo.region=source;silo.position=app.sim.state.regions[source].center;app.sim.state.sites.push_back(silo);
        app.selectedSite=int(app.sim.state.sites.size())-1;
        for(const auto& city:app.sim.state.settlements)if(city.owner==app.sim.state.enemy){
          if(app.sim.launchMissile(size_t(app.selectedSite),city.position)){
            app.center({(silo.position.x+city.position.x)*.5,(silo.position.y+city.position.y)*.5});app.camera.zoom=12;
            double flight=app.sim.state.engagements.back().total;for(int tick=0;tick<int(flight*.35/.25);++tick)app.sim.tick();break;
          }
        }
      }
      if(smoke&&frame==36){
        for(int tick=0;tick<500;++tick){bool hit=false;Point impact{};for(const auto& shot:app.sim.state.engagements)if(shot.ballistic&&shot.sourceSite==app.selectedSite&&shot.impactSeconds>0){hit=true;impact=shot.target;break;}if(hit){app.center(impact);app.camera.zoom=45;break;}app.sim.tick();}
      }
      app.windowFocused=smoke||glfwGetWindowAttrib(window,GLFW_FOCUSED);
      app.frame(float(w),float(h),smoke?0.016:dt);
      ImGui::Render();
      if(app.screen==App::Game&&app.panel!=App::None){gl::Viewport(0,0,fw,fh);gl::ClearColor(.025f,.06f,.10f,1);gl::Clear(0x4000|0x0100);}
      else app.renderer.render(app.world,app.sim.state,app.camera,app.selectedNation,app.selectedRegion,fw,fh,float(fw)/w,float(fh)/h);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      if(smoke&&frame%2==1){
        screenshot(shots/(std::to_string(frame)+".ppm"),fw,fh);
        if(gl::GetError()!=0)throw std::runtime_error("OpenGL error during smoke render");
      }
      if(!smoke)glfwSwapBuffers(window);
      if(smoke&&++frame>=38)break;
    }
    if(!smoke&&app.sim.state.player>=0)app.save(true);
    app.saveSettings();
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
