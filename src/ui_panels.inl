// Included inside App; stable management and selection docks.
  void allocation(){
    ImGui::TextColored(muted,"National allocation is set in the top bar.");
  }
  void buildPanel(){
    heading("CONSTRUCTION");allocation();
    int ready=0;for(int count:sim.state.buildingInventory)ready+=count;
    ImGui::TextColored(accent,"%zu queued  /  %d ready",sim.state.buildingQueue.size(),ready);
    if(ImGui::BeginTabBar("Construction tabs")){
      if(ImGui::BeginTabItem("Catalog")){
        wrapped("Fabricate first; deploy from Ready.");ImGui::BeginChild("Catalog scroll",{0,0});
        const int types[]={10,11,8,9,12,6,4,7,14};
        for(int id:types){ImGui::PushID(id);auto p=ImGui::GetCursorScreenPos();command_icons::icon(ImGui::GetWindowDrawList(),{p.x+12,p.y+12},21,20+id,IM_COL32(99,216,207,255));ImGui::Dummy({28,24});ImGui::SameLine();wrapped(facilityNames[id]);
          auto reason=sim.buildingQueueReason(Facility(id));
          if(button("Queue",reason.empty())){sim.queueBuilding(Facility(id));notice("Building fabrication queued");}tooltip(reason.empty()?"Fabrication: "+durationText(sim.buildingETA(Facility(id))):reason);
          ImGui::SameLine();ImGui::TextColored(muted,"%s",durationText(sim.buildingETA(Facility(id))).c_str());ImGui::Separator();ImGui::PopID();}
        ImGui::EndChild();ImGui::EndTabItem();}
      if(ImGui::BeginTabItem("Queue")){
        ImGui::BeginChild("Queue scroll",{0,0});
        for(size_t i=0;i<sim.state.buildingQueue.size();++i){auto& job=sim.state.buildingQueue[i];ImGui::PushID(int(i)+100);wrapped(facilityNames[int(job.type)]);ImGui::ProgressBar(float(job.progress/std::max(.01,job.total)));ImGui::TextColored(muted,"%s remaining",durationText(sim.buildingETA(i)).c_str());if(button("Cancel")){sim.cancelBuilding(i);ImGui::PopID();break;}ImGui::Separator();ImGui::PopID();}
        if(sim.state.buildingQueue.empty())wrapped("No fabrication in progress.");ImGui::EndChild();ImGui::EndTabItem();}
      if(ImGui::BeginTabItem("Ready")){
        ImGui::BeginChild("Ready scroll",{0,0});
        for(int id=0;id<16;++id)if(sim.state.buildingInventory[id]>0){ImGui::PushID(id);heading(facilityNames[id]);ImGui::Text("%d ready",sim.state.buildingInventory[id]);if(ImGui::Button("Place",{-1,36})){construction=id;notice("Choose controlled land with a free slot");}ImGui::PopID();}
        if(!ready)wrapped("Completed fabrication appears here. Queue a building in Catalog.");ImGui::EndChild();ImGui::EndTabItem();}
      ImGui::EndTabBar();
    }
  }
  void production(int site){
    for(int type=0;type<9;++type){ImGui::PushID(type);auto reason=sim.unitReason(UnitType(type),site);
      if(button(unitNames[type],reason.empty())){sim.queueUnit(UnitType(type),site);notice("Unit production queued");}
      tooltip(reason.empty()?"Production time: "+durationText(sim.unitBuildETA(UnitType(type),site)):reason);ImGui::PopID();}
  }
  void unitCard(int id,bool preview){
    if(id<0||id>=int(sim.state.units.size()))return;auto& unit=sim.state.units[id];
    if(preview){
      auto p=ImGui::GetCursorScreenPos();command_icons::badge(ImGui::GetWindowDrawList(),{p.x+12,p.y+12},20,int(unit.type),IM_COL32(99,216,207,255));ImGui::Dummy({28,24});ImGui::SameLine();wrapped(unitNames[int(unit.type)]);
      ImGui::TextColored(muted,"%s / %.0f%%",unit.owner==sim.state.player?"Friendly":"Enemy",unit.health);
      ImGui::Text("Range %.0f km",sim.unitRangeKm(unit));return;
    }
    auto portrait=ImGui::GetCursorScreenPos();command_icons::badge(ImGui::GetWindowDrawList(),{portrait.x+20,portrait.y+20},27,int(unit.type),IM_COL32(99,216,207,255));ImGui::Dummy({46,40});ImGui::SameLine();wrapped(unitNames[int(unit.type)]);
    flag(unit.owner);ImGui::SameLine();ImGui::TextColored(muted,"%s",unit.owner==sim.state.player?"Friendly formation":"Enemy formation");
    // Keep commands above the detailed statistics so they remain reachable.
    if(unit.owner==sim.state.player){
      ImGui::Text("Formation: %zu selected",selectedUnits.size());
      if(unit.remaining>0){if(button("Cancel production"))sim.cancelUnit(id);}
      else{if(ImGui::Button("Move / attack",{-1,36}))commandMode=true;
        if(button("Hold"))for(int selected:selectedUnits)sim.holdUnit(selected);
        ImGui::SameLine();if(button("Focus [F]")){center(unit.position);camera.zoom=std::max(camera.zoom,40.0);}}
    }
    ImGui::Separator();
    std::string strength="Strength "+std::to_string(int(std::round(unit.health)))+"%";
    ImGui::ProgressBar(float(unit.health/100),{-1,0},strength.c_str());
    ImGui::Text("Speed (1x) %.2f km/s",sim.unitSpeedKmPerSecond(unit));ImGui::Text("Weapon range %.0f km",sim.unitRangeKm(unit));
    bool engaged=false;
    if(sim.state.war&&unit.remaining<=0)for(const auto& enemy:sim.state.units)if(enemy.owner!=unit.owner&&enemy.health>0&&enemy.remaining<=0&&canEngage(unit.type,enemy.type)&&distanceKm(unit.position,enemy.position)<=sim.unitRangeKm(unit)){engaged=true;break;}
    ImGui::TextColored(engaged?ImVec4(.96f,.75f,.4f,1):muted,"%s",unit.remaining>0?"In production":engaged?"Engaging nearby enemy":unit.moving?"Moving to destination":"Holding position");

    if(unit.remaining>0){ImGui::ProgressBar(float(1-unit.remaining/std::max(.01,unit.total)));ImGui::Text("%s remaining",durationText(sim.unitETA(size_t(id))).c_str());}
    else if(unit.owner==sim.state.player)wrapped("Right-click to order. Shift-click toggles formation membership.");
  }
  void siteCard(int id,bool preview){
    if(id<0||id>=int(sim.state.sites.size()))return;auto& site=sim.state.sites[id];auto& region=sim.state.regions[site.region];
    if(preview){
      auto p=ImGui::GetCursorScreenPos();command_icons::badge(ImGui::GetWindowDrawList(),{p.x+12,p.y+12},20,20+int(site.type),IM_COL32(99,216,207,255));ImGui::Dummy({28,24});ImGui::SameLine();wrapped(facilityNames[int(site.type)]);
      ImGui::TextColored(muted,"%s / %s",region.owner==sim.state.player?"Friendly":"Enemy",site.enabled?"Operational":"Disabled");return;
    }
    auto portrait=ImGui::GetCursorScreenPos();command_icons::badge(ImGui::GetWindowDrawList(),{portrait.x+24,portrait.y+24},30,20+int(site.type),IM_COL32(99,216,207,255));ImGui::Dummy({54,54});
    flag(region.owner);ImGui::SameLine();wrapped(facilityNames[int(site.type)]);
    ImGui::TextColored(muted,"%s",site.enabled?"Operational":"Disabled");
    ImGui::Text("Sector %d",site.region+1);
    if(site.type==Facility::Radar)wrapped("Radar coverage preview; detection is not simulated.");
    else if(site.type==Facility::Laboratory)wrapped("Raises national research capacity.");
    else if(site.type==Facility::IndustrialPark)wrapped("Raises production from this region; keep the region controlled.");
    else if(site.type==Facility::Factory)wrapped("Adds manufacturing capacity and produces advanced formations.");
    else if(site.type==Facility::ArmyBase)wrapped("Produces infantry, armour and artillery.");
    else if(site.type==Facility::Airbase)wrapped("Produces combat aircraft after air research.");
    else if(site.type==Facility::Shipyard)wrapped("Coastal naval production and landing support.");
    else if(site.type==Facility::Silo)wrapped("Strategic missile production and delivery.");
    if(preview)return;
    if(region.owner==sim.state.player){ImGui::Checkbox("Enabled",&site.enabled);
      if(site.type==Facility::ArmyBase||site.type==Facility::Factory||site.type==Facility::Airbase||site.type==Facility::Shipyard||site.type==Facility::Silo){heading("UNIT PRODUCTION");production(id);}}
  }
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
    if(id<0||id>=int(sim.state.units.size()))return;
    auto& unit=sim.state.units[id];
    bool friendly=unit.owner==sim.state.player;
    if(ImGui::BeginTable("Unit command columns",3,ImGuiTableFlags_SizingStretchProp)){
      ImGui::TableSetupColumn("Identity",ImGuiTableColumnFlags_WidthStretch,1.1f);
      ImGui::TableSetupColumn("Statistics",ImGuiTableColumnFlags_WidthStretch,1.1f);
      ImGui::TableSetupColumn("Orders",ImGuiTableColumnFlags_WidthStretch,1.f);
      ImGui::TableNextColumn();
      auto p=ImGui::GetCursorScreenPos();command_icons::badge(ImGui::GetWindowDrawList(),{p.x+18,p.y+18},29,int(unit.type),IM_COL32(99,216,207,255));
      ImGui::Dummy({40,38});ImGui::SameLine();ImGui::TextWrapped("%s",unitNames[int(unit.type)]);
      flag(unit.owner);ImGui::SameLine();ImGui::TextColored(muted,"%s",friendly?"Friendly":"Enemy");
      ImGui::TextColored(muted,"%zu selected",selectedUnits.size());
      ImGui::TextColored(muted,"%zu orders queued",unit.pendingDestinations.size());
      if(hoverUnit>=0&&hoverUnit!=id&&hoverUnit<int(sim.state.units.size())){
        const auto& other=sim.state.units[hoverUnit];ImGui::TextColored(muted,"Hover: %s / %.0f%%",unitNames[int(other.type)],other.health);
      }
      ImGui::TableNextColumn();
      std::string strength="Strength "+std::to_string(int(std::round(unit.health)))+"%";
      ImGui::ProgressBar(float(unit.health/100),{-1,0},strength.c_str());
      ImGui::Text("Speed %.2f km/s",sim.unitSpeedKmPerSecond(unit));
      ImGui::Text("Range %.0f km",sim.unitRangeKm(unit));
      bool engaged=false;
      if(sim.state.war&&unit.remaining<=0)for(const auto& enemy:sim.state.units)if(enemy.owner!=unit.owner&&enemy.health>0&&enemy.remaining<=0&&canEngage(unit.type,enemy.type)&&distanceKm(unit.position,enemy.position)<=sim.unitRangeKm(unit)){engaged=true;break;}
      ImGui::TextColored(engaged?ImVec4(.96f,.75f,.4f,1):muted,"%s",unit.remaining>0?"In production":engaged?"Engaging enemy":unit.moving?"Moving":"Holding position");
      if(unit.moving)ImGui::Text("Arrival %s",durationText(arrivalETA(size_t(id))).c_str());
      ImGui::TableNextColumn();
      if(friendly){
        if(unit.remaining>0){ImGui::TextWrapped("%s remaining",durationText(sim.unitETA(size_t(id))).c_str());if(ImGui::Button("Cancel production",{-1,0}))sim.cancelUnit(id);}
        else{
          if(ImGui::Button("Move / attack",{-1,0}))commandMode=true;
          if(ImGui::Button("Stop [X]",{-1,0}))for(int selected:selectedUnits)sim.holdUnit(selected);
          if(ImGui::Button("Focus [F]",{-1,0})){center(unit.position);camera.zoom=std::max(camera.zoom,40.0);}
        }
      }else ImGui::TextWrapped("Enemy formation. Select friendly units to issue orders.");
      ImGui::EndTable();
    }
  }
  void inspector(float w,float h){
    if(panel==Research)return;
    float width=inspectorWidth(w),height=inspectorHeight(h);
    fixedWindow("Inspector",w-width,selectedUnit>=0?h-commandHeight()-height:hudHeight(),width,height);
    if(selectedUnit>=0){
      ImGui::TextColored(accent,"UNIT COMMAND");ImGui::Separator();unitDockCard(selectedUnit);
      ImGui::Separator();
      ImGui::TextColored(muted,"Shift-right: queue  |  Ctrl+1: assign  |  1: recall");
      tooltip("Shift + right-click queues destinations. Ctrl + number assigns a control group; number recalls it. Double-click selects visible units of the same type. Ctrl+A selects all visible friendly units. X stops selected units.");
      ImGui::TextColored(muted,"Double-click: type  |  Ctrl+A: visible  |  X: stop");
      ImGui::End();return;
    }
    heading("COMMAND INSPECTOR");
    bool differentUnit=hoverUnit>=0&&hoverUnit!=selectedUnit;
    bool differentSite=hoverSite>=0&&hoverSite!=selectedSite;
    if(differentUnit||differentSite){
      heading("HOVER PREVIEW");
      ImGui::BeginChild("Compact hover",{0,100*uiScale},false,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
      if(differentUnit)unitCard(hoverUnit,true);else siteCard(hoverSite,true);
      ImGui::EndChild();ImGui::Separator();
    }
    heading("SELECTED");
    if(selectedSite>=0)siteCard(selectedSite,false);
    else if(selectedRegion>=0)regionCard(selectedRegion);
    else {wrapped("Select a unit or building on the map.");ImGui::TextColored(muted,"Left click selects");ImGui::TextColored(muted,"Shift click adds / removes");ImGui::TextColored(muted,"Drag a box for a formation");ImGui::TextColored(muted,"Empty click clears selection");}
    ImGui::End();
  }
  void forcesPanel(){
    heading("ARMED FORCES");allocation();
    if(button(sim.state.war?"War in progress":"Declare war",!sim.state.war&&!sim.state.victory&&!sim.state.defeat))sim.declareWar();
    ImGui::Checkbox("Unit / radar ranges",&showRanges);
    for(size_t i=0;i<sim.state.units.size();++i){auto& unit=sim.state.units[i];if(unit.owner!=sim.state.player||unit.health<=0)continue;ImGui::PushID(int(i));
      if(ImGui::Selectable(unitNames[int(unit.type)],int(i)==selectedUnit)){selectedUnit=int(i);selectedSite=-1;selectedUnits={int(i)};unitOpen=true;regionOpen=false;}
      ImGui::TextColored(muted,"%.0f%%  /  %s",unit.health,unit.remaining>0?"production":unit.moving?"moving":"ready");if(unit.remaining>0)ImGui::TextColored(muted,"%s remaining",durationText(sim.unitETA(i)).c_str());ImGui::PopID();}
    heading("STRATEGIC WEAPONS");ImGui::Text("Warheads: %.0f",sim.state.stock[6]);
    if(button("Queue warhead",sim.state.levels[4]>=4&&sim.state.levels[6]>=4)&&!sim.produceWarhead())notice("Requires an operational factory and laboratory; stock limit 20");
    if(sim.state.warheadsQueued>0)ImGui::Text("%d warheads queued",sim.state.warheadsQueued);
  }
  void sidePanel(float,float h){
    if(panel==None||panel==Research)return;
    fixedWindow("Management",0,hudHeight(),managementWidth(),h-hudHeight()-commandHeight());
    if(button("Close panel"))panel=None;
    switch(panel){case Build:buildPanel();break;case Forces:forcesPanel();break;case Log:heading("COMMAND LOG");for(auto& event:sim.state.events)wrapped(event);break;default:break;}ImGui::End();
  }
