// Included inside App; full management pages and anchored inspection cards.
  void allocation(){ImGui::TextColored(muted,"National allocation is set in the top header.");}
  bool productiveSite(const Site& site) const{return site.type==Facility::ArmyBase||site.type==Facility::Factory||site.type==Facility::Silo;}
  void production(int site){
    for(int type=0;type<9;++type){if(!groundUnit(UnitType(type)))continue;ImGui::PushID(type);auto reason=sim.unitReason(UnitType(type),site);if(button(unitNames[type],reason.empty())){sim.queueUnit(UnitType(type),site);notice("Unit production queued");}tooltip(reason.empty()?"Production time: "+durationText(sim.unitBuildETA(UnitType(type),site)):reason);ImGui::PopID();}
  }
  void buildPanel(){
    heading("PRODUCTION");allocation();ImGui::TextColored(muted,"Queue units and installations. Completed items appear in the ready rail.");
    if(ImGui::BeginTabBar("Production sections")){
      if(ImGui::BeginTabItem("Catalog")){
        ImGui::BeginChild("Production catalog",{0,0});heading("INSTALLATIONS");int columns=std::max(1,int(ImGui::GetContentRegionAvail().x/(280.f*uiScale)));
        if(ImGui::BeginTable("Installation catalog",columns,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_PadOuterX)){
          const int types[]={10,9,12,6,4,7,14};for(int id:types){ImGui::TableNextColumn();ImGui::PushID(id);auto p=ImGui::GetCursorScreenPos();command_icons::icon(ImGui::GetWindowDrawList(),{p.x+18.f*uiScale,p.y+18.f*uiScale},30.f*uiScale,20+id,IM_COL32(220,237,255,255));ImGui::Dummy({44.f*uiScale,40.f*uiScale});ImGui::SameLine();ImGui::TextWrapped("%s",facilityNames[id]);auto reason=sim.buildingQueueReason(Facility(id));ImGui::TextColored(muted,"%s fabrication",durationText(sim.buildingETA(Facility(id))).c_str());if(button("Queue installation",reason.empty())){sim.queueBuilding(Facility(id));notice("Installation fabrication queued");}tooltip(reason);ImGui::Separator();ImGui::PopID();}ImGui::EndTable();}
        heading("UNIT MANUFACTURING");bool sites=false;for(size_t i=0;i<sim.state.sites.size();++i){auto& site=sim.state.sites[i];if(sim.state.regions[site.region].owner!=sim.state.player||!productiveSite(site))continue;sites=true;ImGui::PushID(int(i)+100);if(ImGui::CollapsingHeader((std::string(facilityNames[int(site.type)])+" · sector "+std::to_string(site.region+1)).c_str()))production(int(i));ImGui::PopID();}if(!sites)wrapped("Place an Army Base, Arms Plant or Missile Site to manufacture ground formations.");ImGui::EndChild();ImGui::EndTabItem();}
      if(ImGui::BeginTabItem("Queue")){
        ImGui::BeginChild("Production queue",{0,0});heading("INSTALLATIONS");
        for(size_t i=0;i<sim.state.buildingQueue.size();++i){auto& job=sim.state.buildingQueue[i];ImGui::PushID(int(i)+1000);wrapped(facilityNames[int(job.type)]);ImGui::ProgressBar(float(job.progress/std::max(.01,job.total)),{std::min(440.f*uiScale,ImGui::GetContentRegionAvail().x),0});ImGui::TextColored(muted,"%s remaining",durationText(sim.buildingETA(i)).c_str());ImGui::SameLine();if(button("Cancel")){sim.cancelBuilding(i);ImGui::PopID();break;}ImGui::Separator();ImGui::PopID();}if(sim.state.buildingQueue.empty())wrapped("No installations queued.");
        heading("UNITS");bool pending=false;for(size_t i=0;i<sim.state.units.size();++i){const auto& u=sim.state.units[i];if(u.owner!=sim.state.player||u.remaining<=0||u.health<=0)continue;pending=true;ImGui::PushID(int(i)+2000);wrapped(unitNames[int(u.type)]);ImGui::ProgressBar(float(1-u.remaining/std::max(.01,u.total)),{std::min(440.f*uiScale,ImGui::GetContentRegionAvail().x),0});ImGui::TextColored(muted,"%s remaining",durationText(sim.unitETA(i)).c_str());ImGui::SameLine();if(button("Cancel")){sim.cancelUnit(int(i));ImGui::PopID();break;}ImGui::Separator();ImGui::PopID();}if(!pending)wrapped("No units in production.");ImGui::EndChild();ImGui::EndTabItem();}ImGui::EndTabBar();}
  }
  void unitCard(int id,bool preview){if(id<0||id>=int(sim.state.units.size()))return;auto& u=sim.state.units[id];ImGui::TextWrapped("%s",unitNames[int(u.type)]);ImGui::TextColored(muted,"%s · %.0f%%",u.owner==sim.state.player?"Friendly":"Enemy",u.health);if(!preview)unitDockCard(id);}
  void siteCard(int id,bool preview){if(id<0||id>=int(sim.state.sites.size()))return;auto& site=sim.state.sites[id];auto& region=sim.state.regions[site.region];auto p=ImGui::GetCursorScreenPos();command_icons::icon(ImGui::GetWindowDrawList(),{p.x+17.f*uiScale,p.y+17.f*uiScale},29.f*uiScale,20+int(site.type),IM_COL32(226,239,255,255));ImGui::Dummy({42.f*uiScale,35.f*uiScale});ImGui::SameLine();ImGui::TextWrapped("%s",facilityNames[int(site.type)]);ImGui::Separator();ImGui::Text("Sector %d",site.region+1);ImGui::TextColored(site.enabled?accent:muted,"%s",site.enabled?"Operational":"Disabled");if(!preview&&region.owner==sim.state.player){ImGui::Checkbox("Enabled",&site.enabled);if(productiveSite(site)&&button("Open production"))panel=Build;if(button("Focus")){center(site.position);camera.zoom=std::max(camera.zoom,40.0);}}}
  void regionCard(int id){
    if(id<0||id>=int(sim.state.regions.size()))return;auto& region=sim.state.regions[id];
    flag(region.owner);ImGui::SameLine();ImGui::Text("Sector %d",id+1);
    int main=sim.mainSettlement(id);if(main>=0){heading(sim.state.settlements[main].name.c_str());wrapped(sim.state.settlements[main].population>0?"MAIN CITY / REGIONAL OBJECTIVE":"MAIN STRATEGIC OBJECTIVE");wrapped("Capture every settlement to secure this region. The main city requires at least 150 seconds of occupation at 1x.");}
    for(size_t i=0;i<sim.state.settlements.size();++i){auto& city=sim.state.settlements[i];if(city.region!=id)continue;heading(city.name.c_str());ImGui::TextColored(city.owner==sim.state.player?accent:muted,"%s / %s",city.owner==sim.state.player?"Controlled":"Rival",int(i)==main?"main city":"town");if(city.capture>0){ImGui::ProgressBar(float(city.capture),{-1,0},"Occupation");ImGui::Text("At least %s remaining",durationText((1-city.capture)*(int(i)==main?150:90)).c_str());}else ImGui::TextColored(muted,"Occupation: at least %s",int(i)==main?"150s":"90s");}
    if(region.owner==sim.state.player){if(button("Construction"))toggle(Build);}
    else if(button(sim.state.war?"War in progress":"Declare war",!sim.state.war&&!sim.state.victory&&!sim.state.defeat))sim.declareWar();
    if(button("Review nuclear release",sim.state.war&&region.owner==sim.state.enemy&&sim.state.stock[6]>=1))ImGui::OpenPopup("Strategic release");
    ImGui::SetNextWindowSize({std::min(480.f,ImGui::GetIO().DisplaySize.x-40),0},ImGuiCond_Appearing);
    if(ImGui::BeginPopupModal("Strategic release",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
      wrapped("Nuclear release triggers retaliatory deterrence, mutual destruction and ends the campaign.");
      if(button("Confirm strategic release")){if(!sim.launchNuclear(id))notice("Delivery unavailable: check missile site and technology");ImGui::CloseCurrentPopup();}ImGui::SameLine();if(button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();}
  }

  void unitDockCard(int id){
    if(id<0||id>=int(sim.state.units.size()))return;auto& u=sim.state.units[id];auto p=ImGui::GetCursorScreenPos();command_icons::badge(ImGui::GetWindowDrawList(),{p.x+19.f*uiScale,p.y+16.f*uiScale},26.f*uiScale,int(u.type),IM_COL32(101,186,255,255));ImGui::Dummy({47.f*uiScale,34.f*uiScale});ImGui::SameLine();ImGui::TextWrapped("%s",unitNames[int(u.type)]);ImGui::Separator();
    if(ImGui::BeginTable("Inspection statistics",2,ImGuiTableFlags_SizingStretchProp)) {ImGui::TableSetupColumn("Label",ImGuiTableColumnFlags_WidthStretch,1);ImGui::TableSetupColumn("Value",ImGuiTableColumnFlags_WidthStretch,1.5f);ImGui::TableNextColumn();ImGui::TextUnformatted("Strength");ImGui::TableNextColumn();std::string strength=std::to_string(int(std::round(u.health)))+"%";ImGui::ProgressBar(float(u.health/100),{-1,0},strength.c_str());ImGui::TableNextColumn();ImGui::TextUnformatted("Range");ImGui::TableNextColumn();ImGui::Text("%.0f km",sim.unitRangeKm(u));ImGui::TableNextColumn();ImGui::TextUnformatted("Arrival");ImGui::TableNextColumn();ImGui::TextUnformatted(u.moving?durationText(arrivalETA(size_t(id))).c_str():"Holding position");ImGui::EndTable();}ImGui::Separator();
    if(u.owner==sim.state.player&&u.remaining<=0){float width=(ImGui::GetContentRegionAvail().x-16.f*uiScale)/3.f;if(ImGui::Button("Move",{width,36.f*uiScale}))commandMode=true;ImGui::SameLine();if(ImGui::Button("Stop",{width,36.f*uiScale}))for(int selected:selectedUnits)sim.holdUnit(selected);ImGui::SameLine();if(ImGui::Button("Focus",{width,36.f*uiScale})){center(u.position);camera.zoom=std::max(camera.zoom,40.0);}}
    else ImGui::TextColored(muted,"%s",u.owner==sim.state.player?"Manufacturing":"Enemy formation");
  }
  void capabilityGlyph(ImDrawList* d,ImVec2 c,const std::string& role,float size,ImU32 color){
    float r=size*.5f;auto p=[&](float x,float y){return ImVec2{c.x+x*r,c.y+y*r};};
    if(role=="movement"){
      const ImVec2 points[]={p(-.65f,-1),p(.05f,-1),p(.05f,.25f),p(.85f,.4f),p(.85f,.9f),p(-.65f,.9f)};d->AddConvexPolyFilled(points,6,color);
    }else if(role=="shield"){
      const ImVec2 points[]={p(0,-1),p(.85f,-.55f),p(.65f,.5f),p(0,1),p(-.65f,.5f),p(-.85f,-.55f)};d->AddPolyline(points,6,color,ImDrawFlags_Closed,1.5f);
    }else if(role=="range"){
      d->AddCircle(c,r,color,20,1.3f);d->AddLine(p(-1.3f,0),p(1.3f,0),color,1);d->AddLine(p(0,-1.3f),p(0,1.3f),color,1);
    }else if(role=="medical"){
      d->AddRectFilled(p(-.3f,-1),p(.3f,1),color);d->AddRectFilled(p(-1,-.3f),p(1,.3f),color);
    }else if(role=="weapon"){
      d->AddRectFilled(p(-.35f,-.4f),p(.35f,.9f),color,1);d->AddTriangleFilled(p(-.35f,-.4f),p(.35f,-.4f),p(0,-1),color);
    }else command_icons::icon(d,c,size,role=="research"?26:role=="factory"?24:35,color);
  }
  void capabilitiesCard(){
    if(titleFont)ImGui::PushFont(titleFont);ImGui::TextUnformatted("CAPABILITIES");if(titleFont)ImGui::PopFont();ImGui::Separator();std::vector<Capability> rows;if(selectedUnit>=0)rows=sim.unitCapabilities(size_t(selectedUnit));else if(selectedSite>=0)rows=sim.siteCapabilities(size_t(selectedSite));
    if(rows.empty()){wrapped("Select a unit or installation to inspect its effects.");return;}
    if(ImGui::BeginTable("Capability rows",2,ImGuiTableFlags_SizingStretchProp)){ImGui::TableSetupColumn("Capability",ImGuiTableColumnFlags_WidthFixed,185.f*uiScale);ImGui::TableSetupColumn("Effect",ImGuiTableColumnFlags_WidthStretch,1.4f);for(const auto& row:rows){ImGui::TableNextColumn();auto p=ImGui::GetCursorScreenPos();capabilityGlyph(ImGui::GetWindowDrawList(),{p.x+8.f*uiScale,p.y+9.f*uiScale},row.icon,14.f*uiScale,IM_COL32(213,232,251,255));ImGui::Dummy({21.f*uiScale,18.f*uiScale});ImGui::SameLine();ImGui::TextWrapped("%s",row.name.c_str());ImGui::TableNextColumn();ImGui::PushStyleColor(ImGuiCol_Text,row.active?ImVec4(.7f,.85f,1,1):muted);ImGui::TextWrapped("%s",row.value.c_str());ImGui::PopStyleColor();}ImGui::EndTable();}
  }
  void inspector(float w,float h){
    if(panel!=None||(selectedUnit<0&&selectedSite<0&&selectedRegion<0))return;float s=uiScale,total=inspectorWidth(w),height=inspectorHeight(h),gap=6.f*s,left=total*.49f,right=total-left-gap;float x=w-total-12.f*s,y=h-height-12.f*s;
    // Both anchored cards remain stable while hovering; inspection follows active selection.
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.018f,.04f,.075f,.97f));ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(.32f,.49f,.69f,1));
    bool compact=w/s<900.f;if(!compact){fixedWindow("Capabilities",x,y,left,height);capabilitiesCard();ImGui::End();}fixedWindow("Active inspection",compact?x:x+left+gap,y,compact?total:right,height);
    if(selectedUnit>=0)unitDockCard(selectedUnit);else if(selectedSite>=0)siteCard(selectedSite,false);else if(selectedRegion>=0)regionCard(selectedRegion);else{ImGui::TextUnformatted("ACTIVE INSPECTION");ImGui::Separator();wrapped("Select a unit or building on the map.");ImGui::TextColored(muted,"Click to select · Shift adds to formation");}
    if(compact&&ImGui::CollapsingHeader("Capabilities"))capabilitiesCard();ImGui::End();ImGui::PopStyleColor(2);
  }
  void forcesPanel(){
    heading("FORCES");if(button(sim.state.war?"War in progress":"Declare war",!sim.state.war&&!sim.state.victory&&!sim.state.defeat))sim.declareWar();ImGui::SameLine();ImGui::Checkbox("Unit / radar ranges",&showRanges);
    heading("DEPLOYED FORMATIONS");int columns=std::max(1,int(ImGui::GetContentRegionAvail().x/(260.f*uiScale)));if(ImGui::BeginTable("Force catalog",columns,ImGuiTableFlags_SizingStretchSame)){for(size_t i=0;i<sim.state.units.size();++i){auto& u=sim.state.units[i];if(u.owner!=sim.state.player||u.health<=0||!u.deployed)continue;ImGui::TableNextColumn();ImGui::PushID(int(i));if(ImGui::Selectable(unitNames[int(u.type)],int(i)==selectedUnit)){selectedUnit=int(i);selectedSite=-1;selectedUnits={int(i)};unitOpen=true;regionOpen=false;panel=None;center(u.position);}ImGui::TextColored(muted,"%.0f%% · %s",u.health,u.moving?"moving":"holding");ImGui::Separator();ImGui::PopID();}ImGui::EndTable();}
    heading("STRATEGIC WEAPONS");ImGui::Text("Warheads: %.0f",sim.state.stock[6]);if(button("Queue warhead",sim.state.levels[4]>=4&&sim.state.levels[6]>=4)&&!sim.produceWarhead())notice("Requires an operational factory and laboratory; stock limit 20");if(sim.state.warheadsQueued>0)ImGui::Text("%d warheads queued",sim.state.warheadsQueued);
  }
  void sidePanel(float w,float h){
    if(panel==None||panel==Research)return;float x=pageSafeLeft();
    ImGui::GetBackgroundDrawList()->AddRectFilled({0,hudHeight()},{w,h},IM_COL32(5,10,19,255));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.018f,.04f,.075f,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.f);ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
    ImGui::SetNextWindowPos({x,hudHeight()});ImGui::SetNextWindowSize({w-x,h-hudHeight()});
    ImGui::Begin("Management page",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoBringToFrontOnFocus|ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::PopStyleVar(2);
    if(button("Return to map"))panel=None;ImGui::BeginChild("Management page content",{0,0});switch(panel){case Build:buildPanel();break;case Forces:forcesPanel();break;case Log:heading("COMMAND LOG");for(auto& event:sim.state.events)wrapped(event);break;default:break;}ImGui::EndChild();ImGui::End();ImGui::PopStyleColor();
  }
