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
  enum Screen {
    Menu,Select,Game
  };
  Screen screen=Menu;
  int selectedNation=-1,selectedRegion=-1,tab=0,tabRequest=-1,focus=5,branch=5,facility=4,routeA=0,routeB=1;
  bool paused=true,showRoutes=true,autoResearch=false;
  int speed=1;
  double accumulator=0;
  char search[96]{};
  std::string message;
  std::filesystem::path saves;
  explicit App(World w,std::filesystem::path path):world(std::move(w)),sim(world),saves(std::move(path)){
    selectedNation=world.find("FRA");
  }
  void notice(const std::string& s){
    message=s;
  }
  bool save(bool automatic=false){
    try{
      sim.save((saves/(automatic?"autosave.json":"campaign.json")).string());
      notice(automatic?"Campaign autosaved":"Campaign saved");
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
      renderer.campaign(world,sim.state);
      screen=Game;
      paused=true;
      selectedNation=sim.state.player;
      selectedRegion=sim.state.capital;
      center(world.countries[selectedNation].center);
      notice("Campaign loaded and paused");
    }
    catch(const std::exception& e){
      notice(e.what());
    }
  }
  void center(Point p){
    camera.longitude=std::remainder(p.x,360.0);
    camera.latitude=p.y;
    camera.zoom=camera.globe?1.9:3.3;
  }
  void start(){
    sim.start(selectedNation,Field(focus));
    renderer.campaign(world,sim.state);
    screen=Game;
    selectedRegion=sim.state.capital;
    selectedNation=sim.state.player;
    center(world.countries[sim.state.player].center);
    paused=true;
    accumulator=0;
    notice("Build reserves and choose a research project. Space resumes time.");
  }
  void controls(){
    auto& io=ImGui::GetIO();
    if(!io.WantTextInput){
      if(ImGui::IsKeyPressed(ImGuiKey_G)){
        camera.globe=!camera.globe;
        camera.zoom=camera.globe?1.4:2;
      }
      if(screen==Game&&ImGui::IsKeyPressed(ImGuiKey_Space))paused=!paused;
      if(screen==Game&&ImGui::IsKeyPressed(ImGuiKey_F5))save();
      if(ImGui::IsKeyPressed(ImGuiKey_F9))resume();
      if(ImGui::IsKeyPressed(ImGuiKey_H)&&selectedNation>=0)center(world.countries[selectedNation].center);
      if(screen==Game&&ImGui::IsKeyPressed(ImGuiKey_Escape)){
        paused=true;
        ImGui::OpenPopup("Command pause");
      }
    }
    bool over=io.MousePos.x>=camera.x&&io.MousePos.x<camera.x+camera.width&&io.MousePos.y>=camera.y&&io.MousePos.y<camera.y+camera.height;
    if(over&&!io.WantCaptureMouse){
      camera.zoom=std::clamp(camera.zoom*std::exp(io.MouseWheel*0.12),camera.globe?0.55:0.7,16.0);
      if(ImGui::IsMouseDragging(ImGuiMouseButton_Right,0)){
        camera.longitude=std::remainder(camera.longitude-io.MouseDelta.x*180/camera.width/camera.zoom,360.0);
        camera.latitude=std::clamp(camera.latitude+io.MouseDelta.y*180/camera.width/camera.zoom,-80.0,80.0);
      }
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        auto p=camera.unproject({io.MousePos.x,io.MousePos.y});
        if(p){
          int nation=world.pick(*p);
          if(screen==Select&&nation>=0&&world.countries[nation].code!="ATA")selectedNation=nation;
          if(screen==Game){
            selectedRegion=-1;
            for(size_t i=0;i<renderer.shapes.size();++i){
              for(auto& patch:renderer.shapes[i].patches){
                bool hit=true;
                double sign=0;
                Point q=*p;
                while(q.x-renderer.shapes[i].center.x>180)q.x-=360;
                while(q.x-renderer.shapes[i].center.x< -180)q.x+=360;
                for(size_t k=0;k<patch.size();++k){
                  auto a=patch[k],b=patch[(k+1)%patch.size()];
                  double cross=(b.x-a.x)*(q.y-a.y)-(b.y-a.y)*(q.x-a.x);
                  if(std::abs(cross)<1e-8)continue;
                  if(sign==0)sign=cross;
                  else if(sign*cross<0){
                    hit=false;
                    break;
                  }
                }
                if(hit){
                  selectedRegion=int(i);
                  break;
                }
              }
              if(selectedRegion>=0)break;
            }
          }
        }
      }
    }
  }
  void top(float width){
    fixedWindow("Top command",0,0,width,60);
    ImGui::TextColored(accent,"E S C A L A T I O N");
    ImGui::SameLine(220);
    if(button(camera.globe?"Globe [G]":"Map [G]")){
      camera.globe=!camera.globe;
      camera.zoom=camera.globe?1.4:2;
    }
    ImGui::SameLine();
    if(button("Home [H]"))center(world.countries[selectedNation].center);
    if(screen==Game){
      ImGui::SameLine();
      if(button(paused?"Resume [Space]":"Pause [Space]"))paused=!paused;
      ImGui::SameLine();
      ImGui::SetNextItemWidth(80);
      const char* speeds[]={"1x","2x","4x","8x"};
      int choice=speed==1?0:speed==2?1:speed==4?2:3;
      if(ImGui::Combo("##speed",&choice,speeds,4))speed=1<<choice;
      ImGui::SameLine();
      ImGui::Text("%d / Day %d  |  $%.0f  |  %.1f brigades",sim.year(),sim.state.day%365+1,sim.state.money,sim.state.brigades);
      ImGui::SameLine();
      if(button("Save [F5]"))save();
    }
    ImGui::End();
  }
  void footer(float w,float h){
    fixedWindow("Map status",0,h-76,w,76);
    ImGui::TextColored(muted,"Right drag: pan / rotate   Wheel: zoom   Left click: select   G: projection");
    if(!message.empty())ImGui::TextColored(accent,"%s",message.c_str());
    ImGui::End();
  }
  void mainMenu(float w,float h){
    camera.x=w*0.25f;
    camera.width=w*0.73f;
    camera.y=0;
    camera.height=h;
    camera.zoom=0.85;
    fixedWindow("Main menu",40,h*0.25f,355,420);
    ImGui::SetWindowFontScale(1.6f);
    ImGui::TextColored(accent,"ESCALATION");
    ImGui::SetWindowFontScale(1);
    ImGui::TextColored(muted,"A WORLD UNDER YOUR COMMAND");
    ImGui::Spacing();
    wrapped("Build an industrial nation. Guide a century of research. Prepare for a war whose cost you control.");
    ImGui::Spacing();
    if(ImGui::Button("Start game",{310,42})){
      screen=Select;
      camera.longitude=15;
      camera.latitude=25;
      camera.zoom=0.9;
    }
    bool hasSave=std::filesystem::exists(saves/"campaign.json")||std::filesystem::exists(saves/"autosave.json");
    if(button("Continue saved campaign",hasSave))resume();
    tooltip("Restores the newest manual save or autosave, paused.");
    if(button("Quit"))std::exit(0);
    heading("PROTOTYPE 0.1");
    wrapped("1920-2055 technology campaign. Modern Natural Earth borders. Fictional starting economy. One rival, static defence.");
    ImGui::End();
  }
  void selection(float w,float h){
    top(w);
    fixedWindow("Choose nation",w-390,60,390,h-136);
    heading("CHOOSE YOUR NATION");
    wrapped("Click a country on the globe or search this list.");
    ImGui::InputTextWithHint("##nation","Search nations",search,sizeof(search));
    std::string filter=search;
    std::transform(filter.begin(),filter.end(),filter.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(ImGui::BeginChild("Nation list",{0,145},ImGuiChildFlags_Borders)){
      for(size_t i=0;i<world.countries.size();++i){
        auto& c=world.countries[i];
        if(c.code=="ATA")continue;
        std::string name=c.name;
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char x){return char(std::tolower(x));});
        if(name.find(filter)==std::string::npos)continue;
        if(ImGui::Selectable(c.name.c_str(),int(i)==selectedNation)){
          selectedNation=int(i);
          center(c.center);
        }
      }
    }
    ImGui::EndChild();
    ImGui::BeginChild("Nation details",{0,-100});
    if(selectedNation>=0){
      auto& c=world.countries[selectedNation];
      heading(c.name.c_str());
      wrapped(c.history);
      ImGui::TextColored(muted,"Modern reference data (not 1920 values)");
      ImGui::Text("Population: %.1f million",c.population/1e6);
      ImGui::Text("GDP: $%.1f billion",c.gdp/1000);
      ImGui::Text("Land neighbours: %zu",c.neighbours.size());
      int opponent=world.opponent(selectedNation);
      if(opponent>=0)wrapped("Scenario rival: "+world.countries[opponent].name+(c.neighbours.empty()?" (maritime)":" (land neighbour)"));
      ImGui::Combo("Specialise",&focus,fieldNames.data(),8);
      tooltip("Your chosen field receives 35% more research progress. All fields remain accessible.");
    }
    ImGui::EndChild();
    if(button("Continue - take command",selectedNation>=0))start();
    if(button("Back to main menu"))screen=Menu;
    ImGui::End();
    footer(w,h);
  }
  void overview(){
    auto& s=sim.state;
    heading(world.countries[s.player].name.c_str());
    wrapped("Rival: "+world.countries[s.enemy].name);
    ImGui::Text("Treasury: $%.0f  (%+.1f / day)",s.money,s.income-s.upkeep);
    ImGui::Text("Stability: %.0f%%",s.stability);
    ImGui::Text("Power: %.1f / %.1f demand",s.power,s.powerDemand);
    ImGui::Text("Research: %.1f points / day",s.researchRate);
    heading("NATIONAL STOCKPILES");
    if(ImGui::BeginTable("Stockpiles",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_BordersInnerH)){
      ImGui::TableSetupColumn("Resource");
      ImGui::TableSetupColumn("Reserve");
      ImGui::TableSetupColumn("Daily change");
      ImGui::TableHeadersRow();
      for(int i=0;i<resourceCount;++i){
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(resourceNames[i]);
        ImGui::TableNextColumn();
        ImGui::Text("%.1f",s.stock[i]);
        ImGui::TableNextColumn();
        ImGui::TextColored(s.delta[i]>=0?accent:ImVec4(0.92f,0.48f,0.36f,1),"%+.2f",s.delta[i]);
      }
      ImGui::EndTable();
    }
    tooltip("Stocks are nationally pooled after each connected site's route and power limits. Combat consumption is shown in the next tick's totals.");
    heading("ORDERS");
    wrapped("1. Research Industrial electrification or National laboratories.\n2. Build facilities on a selected sector and connect them to your capital.\n3. Stockpile fuel, supplies and equipment; recruit brigades.\n4. Declare war, take a supplied frontier, then extend your transport network.");
  }
  void research(){
    auto& s=sim.state;
    heading("RESEARCH & DOCTRINE");
    ImGui::Text("Era: %d  |  %.1f RP / day",sim.year(),s.researchRate);
    float budget=float(s.researchBudget*100);
    if(ImGui::SliderFloat("Funding",&budget,0,100,"%.0f%%"))s.researchBudget=budget/100;
    tooltip("Funding changes laboratory output and treasury upkeep; 0.50 is balanced.");
    ImGui::Checkbox("Auto queue specialization",&autoResearch);
    tooltip("When idle, starts the first eligible technology in your chosen field.");
    ImGui::TextColored(muted,"Specialization: %s (+35%%)",fieldNames[int(s.specialization)]);
    heading("RESEARCH QUEUE");
    if(s.research.empty())wrapped("Queue an available technology below.");
    for(size_t i=0;i<s.research.size();){
      auto& j=s.research[i];
      ImGui::PushID(int(i));
      wrapped(sim.techs[j.tech].name);
      ImGui::ProgressBar(float(j.progress/sim.techs[j.tech].cost),{-70,0});
      ImGui::SameLine();
      bool remove=button("Cancel");
      tooltip("Cancelling discards progress on this project.");
      ImGui::PopID();
      if(remove)sim.removeResearch(i);
      else ++i;
    }
    heading("TECHNOLOGY TREE - 1920 TO 2050");
    ImGui::Combo("Field",&branch,fieldNames.data(),8);
    for(int tier=0;tier<14;++tier){
      int id=branch*14+tier;
      auto& t=sim.techs[id];
      ImGui::PushID(id);
      ImGui::TextColored(muted,"%d  %s",t.year,s.completed[id]?"COMPLETE":"");
      wrapped(t.name);
      std::string reason=sim.lockReason(id);
      if(button(s.completed[id]?"Researched":"Queue research",reason.empty()))sim.queueResearch(id);
      std::string detail=t.effect+"\nCost: "+std::to_string(int(t.cost))+" RP";
      for(int p:t.prerequisites)detail+="\nRequires: "+sim.techs[p].name;
      if(!reason.empty())detail+="\n"+reason;
      tooltip(detail);
      ImGui::Separator();
      ImGui::PopID();
    }
  }
  void industry(){
    auto& s=sim.state;
    heading("INDUSTRY & CONSTRUCTION");
    wrapped("Ore -> steel -> equipment + supplies\nCrude oil -> fuel -> production and operations");
    ImGui::Text("Grid: %.1f power / %.1f demand",s.power,s.powerDemand);
    ImGui::Combo("Build",&facility,facilityNames.data(),10);
    ImGui::Text("Cost: $%.0f + 30 steel",sim.buildCost(Facility(facility)));
    std::string reason=sim.buildReason(selectedRegion,Facility(facility));
    if(button("Build in selected sector",reason.empty()))sim.build(selectedRegion,Facility(facility));
    tooltip(reason.empty()?"35 days of connected construction; missile silos take 120 days.":reason);
    if(selectedRegion>=0){
      ImGui::SameLine();
      if(button("Connect to capital",s.regions[selectedRegion].owner==s.player&&selectedRegion!=s.capital)){
        if(!sim.connect(s.capital,selectedRegion))notice("Link already exists or requires $250 and 10 steel.");
      }
    }
    heading("FACILITIES");
    for(size_t i=0;i<s.sites.size();++i){
      auto& site=s.sites[i];
      if(s.regions[site.region].owner!=s.player)continue;
      ImGui::PushID(int(i));
      ImGui::Checkbox("##enable",&site.enabled);
      ImGui::SameLine();
      if(ImGui::Selectable(facilityNames[int(site.type)],selectedRegion==site.region))selectedRegion=site.region;
      ImGui::TextColored(muted,"Sector %d | %s | %.0f%% output",site.region+1,site.remaining>0?"building":"operational",site.efficiency*100);
      if(site.remaining>0)ImGui::Text("%.0f construction days left",site.remaining);
      tooltip("Requires an enabled supply route, power and recipe inputs. Multiple facilities in a sector share its inbound capacity.");
      ImGui::Separator();
      ImGui::PopID();
    }
  }
  void logistics(){
    auto& s=sim.state;
    heading("SUPPLY NETWORK");
    wrapped("Connect sites to the capital. Throughput is the best bottleneck capacity along an enabled route. Captured sectors need a new route before the front can advance.");
    ImGui::Checkbox("Show routes on map",&showRoutes);
    auto sectorCombo=[&](const char* label,int& value){
      std::string preview="Sector "+std::to_string(value+1);
      if(ImGui::BeginCombo(label,preview.c_str())){
        for(size_t i=0;i<s.regions.size();++i)if(s.regions[i].owner==s.player){
          std::string name="Sector "+std::to_string(i+1);
          if(ImGui::Selectable(name.c_str(),value==int(i)))value=int(i);
        }
        ImGui::EndCombo();
      }
    };
    sectorCombo("From",routeA);
    sectorCombo("To",routeB);
    if(button("Add route: $250 + 10 steel")){
      if(!sim.connect(routeA,routeB))notice("Choose different controlled sectors; avoid duplicate links and keep funds/steel available.");
    }
    heading("ROUTES");
    for(size_t i=0;i<s.routes.size();++i){
      auto& route=s.routes[i];
      if(s.regions[route.a].owner!=s.player||s.regions[route.b].owner!=s.player)continue;
      ImGui::PushID(int(i));
      ImGui::Checkbox("##enabled",&route.enabled);
      ImGui::SameLine();
      ImGui::Text("%d -> %d | %.0f capacity",route.a+1,route.b+1,route.capacity*(1+0.15*s.levels[7]));
      if(button("Upgrade +4: $600 / 20 steel",s.money>=600&&s.stock[2]>=20))sim.upgradeRoute(i);
      ImGui::Separator();
      ImGui::PopID();
    }
  }
  void conflict(){
    auto& s=sim.state;
    heading("OPERATIONS & DETERRENCE");
    ImGui::Text("Brigades: %.1f / 60",s.brigades);
    if(button("Equip brigade: $1000 / 20 equipment",s.money>=1000&&s.stock[5]>=20&&s.brigades<60))sim.recruit();
    if(button("Declare war",!s.war&&!s.victory&&!s.defeat))sim.declareWar();
    if(selectedRegion>=0&&s.regions[selectedRegion].owner==s.enemy){
      if(button("Order offensive on selected sector",s.war)){
        if(!sim.attack(selectedRegion))notice("Target must touch supplied controlled land. Disconnected landings require Fleet logistics and a supplied shipyard.");
      }
      tooltip("Static defence resists your brigades. Fuel, supplies, research and connected routes determine progress.");
    }
    if(s.target>=0){
      ImGui::Text("Sector %d offensive",s.target+1);
      ImGui::ProgressBar(float(s.regions[s.target].capture));
      if(button("Halt offensive"))s.target=-1;
    }
    heading("NUCLEAR DETERRENCE");
    wrapped("Warheads require Ballistic and Research developments from the 1950s. Delivery requires a completed, supplied missile silo. This POC models the rival's retaliatory deterrent as a fixed scripted second strike.");
    ImGui::Text("Warheads: %.0f",s.stock[6]);
    if(button("Stockpile strategic warhead",s.levels[4]>=4&&s.levels[6]>=4&&s.money>=25000&&s.stock[2]>=400&&s.stock[3]>=250))sim.produceWarhead();
    tooltip("Costs $25000, 400 steel and 250 fuel. Missile silos cost $18000 and require 120 days of supplied construction.");
    if(button("Review nuclear strike",s.war&&selectedRegion>=0&&s.regions[selectedRegion].owner==s.enemy&&s.stock[6]>=1))ImGui::OpenPopup("Strategic release");
    if(ImGui::BeginPopupModal("Strategic release",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
      wrapped("A nuclear strike will trigger a scripted second strike, devastate your industry and end this campaign in mutual destruction.");
      if(button("Confirm launch and accept destruction")){
        if(!sim.launchNuclear(selectedRegion))notice("Launch unavailable: complete and connect a missile silo.");
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if(button("Cancel"))ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
    }
  }
  void mapLabels(){
    if(screen!=Game)return;
    auto& s=sim.state;
    auto* draw=ImGui::GetBackgroundDrawList();
    draw->PushClipRect({camera.x,camera.y},{camera.x+camera.width,camera.y+camera.height},true);
    if(showRoutes)for(auto& route:s.routes){
      if(s.regions[route.a].owner!=s.player||s.regions[route.b].owner!=s.player)continue;
      bool a,b;
      auto pa=camera.project(s.regions[route.a].center,&a),pb=camera.project(s.regions[route.b].center,&b);
      if(a&&b&&std::abs(pa.x-pb.x)<camera.width*0.6)draw->AddLine(vec(pa),vec(pb),route.enabled?IM_COL32(69,166,151,140):IM_COL32(130,95,78,120),1.3f);
    }
    for(size_t i=0;i<s.regions.size();++i){
      bool visible;
      auto p=camera.project(s.regions[i].center,&visible);
      if(!visible)continue;
      bool owned=s.regions[i].owner==s.player;
      auto col=owned?IM_COL32(100,215,187,255):IM_COL32(228,112,88,255);
      draw->AddCircleFilled(vec(p),int(i)==selectedRegion?5:3,col);
      if(int(i)==selectedRegion)draw->AddCircle(vec(p),10,IM_COL32(200,250,230,255),20,1.4f);
      if(camera.zoom>2.5){
        std::string label=std::to_string(i+1);
        draw->AddText({float(p.x)+5,float(p.y)-6},col,label.c_str());
      }
    }
    for(int i=24;i<int(s.regions.size());++i)if(s.regions[i].owner==s.enemy)for(int n:s.regions[i].neighbours)if(s.regions[n].owner==s.player){
      bool a,b;
      auto p=camera.project(s.regions[i].center,&a),q=camera.project(s.regions[n].center,&b);
      if(a&&b){
        Point mid{
          (p.x+q.x)/2,(p.y+q.y)/2
        };
        double dx=q.x-p.x,dy=q.y-p.y,length=std::max(1.0,std::hypot(dx,dy));
        draw->AddLine({float(mid.x-dy/length*10),float(mid.y+dx/length*10)},{float(mid.x+dy/length*10),float(mid.y-dx/length*10)},IM_COL32(238,171,86,255),3);
      }
    }
    for(auto& site:s.sites){
      if(s.regions[site.region].owner!=s.player)continue;
      bool visible;
      auto p=camera.project(s.regions[site.region].center,&visible);
      if(!visible)continue;
      auto color=site.enabled?IM_COL32(214,229,199,255):IM_COL32(139,110,90,255);
      draw->AddRect({float(p.x)-5,float(p.y)-5},{float(p.x)+5,float(p.y)+5},color,1,0,1.4f);
      if(camera.zoom>3){
        const char* symbols[]={"M","O","S","F","A","P","R","D","N","X"};
        draw->AddText({float(p.x)+8,float(p.y)+5},color,symbols[int(site.type)]);
      }
      auto mouse=ImGui::GetIO().MousePos;
      if(std::hypot(mouse.x-p.x,mouse.y-p.y)<8&&!ImGui::GetIO().WantCaptureMouse){
        ImGui::BeginTooltip();
        ImGui::Text("%s - sector %d",facilityNames[int(site.type)],site.region+1);
        ImGui::Text("%.0f%% output | %.0f construction days",site.efficiency*100,site.remaining);
        ImGui::EndTooltip();
      }
    }
    draw->PopClipRect();
  }
  void game(float w,float h){
    top(w);
    fixedWindow("National command",w-410,60,410,h-136);
    if(sim.state.victory)ImGui::TextColored(accent,"VICTORY - rival territory controlled");
    if(sim.state.defeat)ImGui::TextColored({1,0.45f,0.35f,1},"CAMPAIGN ENDED");
    if(selectedRegion>=0){
      auto& r=sim.state.regions[selectedRegion];
      ImGui::Text("Sector %d | %s",selectedRegion+1,r.owner==sim.state.player?"Controlled":"Rival");
      if(selectedRegion==sim.state.capital)ImGui::Text("Capital hub | Ore %.1f | Oil %.1f",r.ore,r.oil);
      else ImGui::Text("Supply: %.1f | Ore %.1f | Oil %.1f",sim.connection(selectedRegion),r.ore,r.oil);
    }
    if(ImGui::BeginTabBar("Management")){
      const char* labels[]={"Nation","Research","Industry","Supply","War"};
      int requested=tabRequest;
      tabRequest=-1;
      for(int i=0;i<5;++i)if(ImGui::BeginTabItem(labels[i],nullptr,requested==i?ImGuiTabItemFlags_SetSelected:0)){
        tab=i;
        switch(i){
          case 0:overview();
          break;
          case 1:research();
          break;
          case 2:industry();
          break;
          case 3:logistics();
          break;
          case 4:conflict();
          break;
        }
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    ImGui::End();
    fixedWindow("Command log",16,h-246,std::min(520.0f,w-450),160);
    ImGui::TextColored(muted,"COMMAND LOG");
    for(size_t i=0;i<std::min(size_t(4),sim.state.events.size());++i)wrapped(sim.state.events[i]);
    ImGui::End();
    footer(w,h);
    if(ImGui::BeginPopupModal("Command pause",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
      wrapped("Campaign paused");
      if(button("Return to command"))ImGui::CloseCurrentPopup();
      if(button("Save campaign"))save();
      if(button("Save and return to main menu")){if(save()){screen=Menu;sim.state=Campaign{};ImGui::CloseCurrentPopup();}}ImGui::EndPopup();
    }
  }
  void frame(float w,float h,double dt){
    camera.x=0;
    camera.y=60;
    camera.width=w-(screen==Select?390:410);
    camera.height=h-136;
    if(screen==Menu){
      camera.longitude=std::remainder(camera.longitude+dt*2,360.0);
      mainMenu(w,h);
    }
    else if(screen==Select)selection(w,h);
    else game(w,h);
    controls();
    mapLabels();
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
    app.renderer.initialise(app.world);
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
        if(frame==2)app.screen=App::Select;
        if(frame==4)app.start();
        if(frame==6){
          app.camera.globe=false;
          app.camera.zoom=8;
        }
        if(frame>=8)app.tabRequest=std::min(4,(frame-8)/2+1);
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
      app.frame(float(w),float(h),smoke?0.016:dt);
      ImGui::Render();
      app.renderer.render(app.world,app.sim.state,app.camera,app.selectedNation,app.selectedRegion,fw,fh,float(fw)/w,float(fh)/h);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      if(smoke&&frame%2==1){
        screenshot(shots/(std::to_string(frame)+".ppm"),fw,fh);
        if(gl::GetError()!=0)throw std::runtime_error("OpenGL error during smoke render");
      }
      if(!smoke)glfwSwapBuffers(window);
      if(smoke&&++frame>=16)break;
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
