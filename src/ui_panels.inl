// Included inside App; full management pages and anchored inspection cards.
  void allocation(){ImGui::TextColored(muted,"National allocation is set in the top header.");}
  bool productiveSite(const Site& site) const{return site.type==Facility::ArmyBase||site.type==Facility::Factory||site.type==Facility::Silo||site.type==Facility::Shipyard;}
  void production(int site){
    for(int type=0;type<int(UnitType::Count);++type){if(!activeUnit(UnitType(type)))continue;ImGui::PushID(type);auto reason=sim.unitReason(UnitType(type),site);if(button(unitNames[type],reason.empty())){sim.queueUnit(UnitType(type),site);notice("Unit production queued");}tooltip(reason.empty()?"Production time: "+durationText(sim.unitBuildETA(UnitType(type),site)):reason);ImGui::PopID();}
  }
  void buildPanel(){
    heading("PRODUCTION");allocation();ImGui::TextColored(muted,"Queue units and installations. Completed items appear in the ready rail.");
    if(ImGui::BeginTabBar("Production sections")){
      if(ImGui::BeginTabItem("Catalog")){
        ImGui::BeginChild("Production catalog",{0,0});heading("INSTALLATIONS");int columns=std::max(1,int(ImGui::GetContentRegionAvail().x/(280.f*uiScale)));
        if(ImGui::BeginTable("Installation catalog",columns,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_PadOuterX)){
          const int types[]={10,8,9,12,6,4,7,14};for(int id:types){ImGui::TableNextColumn();ImGui::PushID(id);auto p=ImGui::GetCursorScreenPos();command_icons::icon(ImGui::GetWindowDrawList(),{p.x+18.f*uiScale,p.y+18.f*uiScale},30.f*uiScale,20+id,IM_COL32(220,237,255,255));ImGui::Dummy({44.f*uiScale,40.f*uiScale});ImGui::SameLine();ImGui::TextWrapped("%s",facilityNames[id]);auto reason=sim.buildingQueueReason(Facility(id));ImGui::TextColored(muted,"%s fabrication",durationText(sim.buildingETA(Facility(id))).c_str());if(button("Queue installation",reason.empty())){sim.queueBuilding(Facility(id));notice("Installation fabrication queued");}tooltip(reason);ImGui::Separator();ImGui::PopID();}ImGui::EndTable();}
        heading("UNIT MANUFACTURING");bool sites=false;for(size_t i=0;i<sim.state.sites.size();++i){auto& site=sim.state.sites[i];if(sim.state.regions[site.region].owner!=sim.state.player||!productiveSite(site))continue;sites=true;ImGui::PushID(int(i)+100);if(ImGui::CollapsingHeader((std::string(facilityNames[int(site.type)])+" · sector "+std::to_string(site.region+1)).c_str()))production(int(i));ImGui::PopID();}if(!sites)wrapped("Place an Army Base, Arms Plant, Missile Site or Shipyard to manufacture formations.");ImGui::EndChild();ImGui::EndTabItem();}
      if(ImGui::BeginTabItem("Queue")){
        ImGui::BeginChild("Production queue",{0,0});heading("INSTALLATIONS");
        for(size_t i=0;i<sim.state.buildingQueue.size();++i){auto& job=sim.state.buildingQueue[i];ImGui::PushID(int(i)+1000);wrapped(facilityNames[int(job.type)]);ImGui::ProgressBar(float(job.progress/std::max(.01,job.total)),{std::min(440.f*uiScale,ImGui::GetContentRegionAvail().x),0});ImGui::TextColored(muted,"%s remaining",durationText(sim.buildingETA(i)).c_str());ImGui::SameLine();if(button("Cancel")){sim.cancelBuilding(i);ImGui::PopID();break;}ImGui::Separator();ImGui::PopID();}if(sim.state.buildingQueue.empty())wrapped("No installations queued.");
        heading("UNITS");bool pending=false;for(size_t i=0;i<sim.state.units.size();++i){const auto& u=sim.state.units[i];if(u.owner!=sim.state.player||u.remaining<=0||u.health<=0)continue;pending=true;ImGui::PushID(int(i)+2000);wrapped(unitNames[int(u.type)]);ImGui::ProgressBar(float(1-u.remaining/std::max(.01,u.total)),{std::min(440.f*uiScale,ImGui::GetContentRegionAvail().x),0});ImGui::TextColored(muted,"%s remaining",durationText(sim.unitETA(i)).c_str());ImGui::SameLine();if(button("Cancel")){sim.cancelUnit(int(i));ImGui::PopID();break;}ImGui::Separator();ImGui::PopID();}if(!pending)wrapped("No units in production.");ImGui::EndChild();ImGui::EndTabItem();}ImGui::EndTabBar();}
  }
  void unitCard(int id,bool preview){if(id<0||id>=int(sim.state.units.size()))return;auto& u=sim.state.units[id];ImGui::TextWrapped("%s",unitNames[int(u.type)]);ImGui::TextColored(muted,"%s · %.0f%%",u.owner==sim.state.player?"Friendly":"Enemy",u.health);if(!preview)unitDockCard(id);}
  void siteCard(int id,bool preview){if(id<0||id>=int(sim.state.sites.size()))return;auto& site=sim.state.sites[id];auto& region=sim.state.regions[site.region];auto p=ImGui::GetCursorScreenPos();command_icons::icon(ImGui::GetWindowDrawList(),{p.x+17.f*uiScale,p.y+17.f*uiScale},29.f*uiScale,20+int(site.type),IM_COL32(226,239,255,255));ImGui::Dummy({42.f*uiScale,35.f*uiScale});ImGui::SameLine();ImGui::TextWrapped("%s",facilityNames[int(site.type)]);ImGui::Separator();ImGui::Text("Sector %d",site.region+1);ImGui::TextColored(site.enabled?accent:muted,"%s",site.enabled?"Operational":"Disabled");if(!preview&&region.owner==sim.state.player){ImGui::Checkbox("Enabled",&site.enabled);if(productiveSite(site)&&button("Open production"))panel=Build;if(button("Focus")){center(site.position);camera.zoom=std::max(camera.zoom,40.0);}}}
  static std::string populationText(double population){
    if(population<=0)return "Population unavailable";
    std::string value=std::to_string(static_cast<long long>(std::llround(population)));
    for(int at=int(value.size())-3;at>0;at-=3)value.insert(size_t(at),",");
    return value;
  }
  void cityCard(int id){
    if(id<0||id>=int(sim.state.settlements.size()))return;const auto& city=sim.state.settlements[id];
    heading(city.name.c_str());ImGui::TextWrapped("Reference population: %s",populationText(city.population).c_str());
    if(city.owner>=0&&city.owner<int(world.countries.size())){flag(city.owner);ImGui::SameLine();ImGui::TextWrapped("Controlled by %s",world.countries[city.owner].name.c_str());}
    bool main=sim.mainSettlement(city.region)==id;ImGui::Separator();wrapped(main?"MAIN CITY / REGIONAL OBJECTIVE":"CITY / REGIONAL OBJECTIVE");ImGui::Text("Sector %d",city.region+1);
    ImGui::ProgressBar(float(city.capture),{-1,0},city.capture>0?"Occupation in progress":"No occupation");ImGui::TextWrapped("Minimum occupation: %s at 1x",durationText(main?150:90).c_str());
    if(city.capture>0)ImGui::TextWrapped("At least %s remaining",durationText((1-city.capture)*(main?150:90)).c_str());
    if(button("Focus city")){center(city.position);camera.zoom=std::max(camera.zoom,55.0);}
    if(button("Inspect sector")){selectedSettlement=-1;selectedRegion=city.region;}
  }
  void worldCityCard(int id){
    if(id<0||id>=int(world.cities.size()))return;const auto& city=world.cities[id];heading(city.name.c_str());ImGui::TextWrapped("Reference population: %s",populationText(city.population).c_str());
    int nation=world.find(city.countryCode);if(nation>=0){flag(nation);ImGui::SameLine();ImGui::TextWrapped("%s",world.countries[nation].name.c_str());}
    wrapped("Outside the current campaign's occupation objectives.");if(button("Focus city")){center(city.position);camera.zoom=std::max(camera.zoom,55.0);}
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
      wrapped("A nuclear strike damages formations and infrastructure of either side, and lowers national stability when it impacts. Confirm release?");
      if(button("Confirm strategic release")){if(!sim.launchNuclear(id))notice("Delivery unavailable: check missile site and technology");ImGui::CloseCurrentPopup();}ImGui::SameLine();if(button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();}
  }

  void weaponIcon(ImDrawList* d,ImVec2 center,float size,const std::string& id){
    FlagUV uv;
    if(renderer.assets.weaponIcon(id,uv))d->AddImage(ImTextureID(renderer.assets.weaponIcons),{center.x-size*.5f,center.y-size*.5f},{center.x+size*.5f,center.y+size*.5f},vec(uv.min),vec(uv.max));
    else command_icons::weapon(d,center,size,id,IM_COL32(171,218,228,255));
  }
  void unitDockCard(int id){
    if(id<0||id>=int(sim.state.units.size()))return;auto& u=sim.state.units[id];float scale=uiScale;
    ImGui::TextUnformatted(unitNames[int(u.type)]);ImGui::Separator();
    auto p=ImGui::GetCursorScreenPos();command_icons::badge(ImGui::GetWindowDrawList(),{p.x+39*scale,p.y+39*scale},42*scale,int(u.type),IM_COL32(100,207,244,255));ImGui::Dummy({88*scale,78*scale});ImGui::SameLine();
    ImGui::BeginGroup();int n=0;for(const auto& weapon:sim.unitWeapons(u)){ImGui::PushID(weapon.id.c_str());auto q=ImGui::GetCursorScreenPos();ImGui::InvisibleButton("Weapon",{53*scale,53*scale});auto*d=ImGui::GetWindowDrawList();d->AddRectFilled(q,{q.x+53*scale,q.y+53*scale},IM_COL32(12,39,52,220));d->AddRect(q,{q.x+53*scale,q.y+53*scale},IM_COL32(51,115,134,230));weaponIcon(d,{q.x+26*scale,q.y+22*scale},38*scale,weapon.id);int slot=weaponSlot(u.type,weapon.id);int ammo=slot>=0&&slot<int(u.weaponAmmo.size())?u.weaponAmmo[slot]:weapon.magazine;auto count=std::to_string(ammo);d->AddText({q.x+35*scale,q.y+35*scale},IM_COL32(210,230,235,255),count.c_str());if(ImGui::IsItemHovered()){ImGui::BeginTooltip();ImGui::TextUnformatted(weapon.name.c_str());ImGui::Text("%.0f–%.0f km / %.1f damage",weapon.minRangeKm,weapon.maxRangeKm,weapon.damage);ImGui::TextWrapped("%s",weaponDomains(weapon.targetMask).c_str());ImGui::TextWrapped("%s",weapon.behaviour.c_str());ImGui::EndTooltip();}ImGui::PopID();if(++n%3)ImGui::SameLine(0,4*scale);}ImGui::EndGroup();
    std::string strength=std::to_string(int(std::round(u.health)))+"% strength";ImGui::ProgressBar(float(u.health/100),{-1,6*scale},"");ImGui::Text("%s / %.0f km engagement",strength.c_str(),sim.unitRangeKm(u));
    ImGui::Checkbox("Show ranges",&showRanges);ImGui::TextWrapped("%s",u.moving?("Arrival: "+durationText(arrivalETA(size_t(id)))).c_str():"Holding position");
    if(u.owner==sim.state.player&&u.remaining<=0){bool pointStrike=false;for(const auto& mount:sim.unitWeapons(u))if(mount.id=="howitzer"||mount.id=="ground_missile"||mount.id=="cruise")pointStrike=true;float bw=(ImGui::GetContentRegionAvail().x-8*scale)*.5f;if(ImGui::Button("Move",{pointStrike?bw:ImGui::GetContentRegionAvail().x,26*scale})){commandMode=true;attackMode=0;}if(pointStrike){ImGui::SameLine();if(ImGui::Button("Attack point",{bw,26*scale})){attackMode=1;commandMode=false;notice("Choose an attack target on the map; Escape cancels");}}if(ImGui::Button("Stop",{bw,26*scale}))for(int selected:selectedUnits)sim.holdUnit(selected);ImGui::SameLine();if(ImGui::Button("Focus",{bw,26*scale})){center(u.position);camera.zoom=std::max(camera.zoom,40.0);}if(navalUnit(u.type)){if(button("Return to shipyard")&&!sim.returnToPort(size_t(id)))notice("No reachable operational friendly shipyard");if(u.portServiceSeconds>0)ImGui::ProgressBar(float(u.portServiceSeconds/30.0),{-1,0},"Repair / rearm");}}
    if(ImGui::CollapsingHeader("Capabilities / weapon details"))capabilitiesCard();
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
  static std::string weaponDomains(unsigned mask){std::string value;for(auto item:std::initializer_list<std::pair<unsigned,const char*>>{{1,"Ground"},{2,"Surface ships"},{4,"Submerged"},{8,"Air"}})if(mask&item.first){if(!value.empty())value+=" / ";value+=item.second;}return value;}
  void weaponCard(const Unit& unit){
    heading("WEAPON SYSTEMS");
    for(const auto& weapon:sim.unitWeapons(unit)){
      ImGui::PushID(weapon.id.c_str());auto iconPos=ImGui::GetCursorScreenPos();weaponIcon(ImGui::GetWindowDrawList(),{iconPos.x+14*uiScale,iconPos.y+14*uiScale},28*uiScale,weapon.id);ImGui::Dummy({32*uiScale,28*uiScale});ImGui::SameLine();ImGui::TextWrapped("%s",weapon.name.c_str());
      tooltip(weapon.behaviour+" · "+std::to_string(int(weapon.reloadSeconds))+"s magazine reload");
      ImGui::TextColored(accent,"%.1f–%.1f km",weapon.minRangeKm,weapon.maxRangeKm);ImGui::SameLine();ImGui::TextWrapped("%s%s",weaponDomains(weapon.targetMask).c_str(),weapon.armourOnly?" (armour)":"");
      int slot=weaponSlot(unit.type,weapon.id);
      int rounds=slot>=0&&slot<int(unit.weaponAmmo.size())?unit.weaponAmmo[size_t(slot)]:weapon.magazine;
      int reserve=slot>=0&&slot<int(unit.weaponReserve.size())?unit.weaponReserve[size_t(slot)]:weapon.reserveAmmo;
      ImGui::TextWrapped("%.1f dmg / %.1fs · %d rounds (+%d)",weapon.damage,weapon.cooldownSeconds,rounds,reserve);
      if(slot>=0&&slot<int(unit.weaponReloads.size())&&unit.weaponReloads[size_t(slot)]>0)ImGui::Text("Reloading: %.1fs",unit.weaponReloads[size_t(slot)]);
      ImGui::Separator();ImGui::PopID();
    }
  }
  void capabilitiesCard(){
    if(titleFont)ImGui::PushFont(titleFont);ImGui::TextUnformatted("CAPABILITIES");if(titleFont)ImGui::PopFont();ImGui::Separator();std::vector<Capability> rows;if(selectedUnit>=0)rows=sim.unitCapabilities(size_t(selectedUnit));else if(selectedSite>=0)rows=sim.siteCapabilities(size_t(selectedSite));
    if(selectedSettlement>=0||selectedWorldCity>=0){heading("CITY PROFILE");double population=selectedSettlement>=0?sim.state.settlements[selectedSettlement].population:world.cities[selectedWorldCity].population;ImGui::TextWrapped("Reference population: %s",populationText(population).c_str());wrapped("Population is the source gazetteer figure, not a live census.");if(selectedSettlement>=0){wrapped("Control changes through ground occupation. Every city in a sector must be secured to capture the sector.");}return;}
    if(rows.empty()){wrapped("Select a unit or installation to inspect its effects.");return;}
    if(ImGui::BeginTable("Capability rows",2,ImGuiTableFlags_SizingStretchProp)){ImGui::TableSetupColumn("Capability",ImGuiTableColumnFlags_WidthFixed,185.f*uiScale);ImGui::TableSetupColumn("Effect",ImGuiTableColumnFlags_WidthStretch,1.4f);for(const auto& row:rows){if(selectedUnit>=0){bool weaponRow=false;for(const auto& weapon:sim.unitWeapons(sim.state.units[selectedUnit]))if((row.name==weapon.name||row.name==weapon.name+" ammunition"))weaponRow=true;if(weaponRow)continue;}ImGui::TableNextColumn();auto p=ImGui::GetCursorScreenPos();capabilityGlyph(ImGui::GetWindowDrawList(),{p.x+8.f*uiScale,p.y+9.f*uiScale},row.icon,14.f*uiScale,IM_COL32(213,232,251,255));ImGui::Dummy({21.f*uiScale,18.f*uiScale});ImGui::SameLine();ImGui::TextWrapped("%s",row.name.c_str());ImGui::TableNextColumn();ImGui::PushStyleColor(ImGuiCol_Text,row.active?ImVec4(.7f,.85f,1,1):muted);ImGui::TextWrapped("%s",row.value.c_str());ImGui::PopStyleColor();}ImGui::EndTable();}
    if(selectedUnit>=0)weaponCard(sim.state.units[selectedUnit]);
  }
  void inspector(float w,float h){
    if(panel!=None||(selectedUnit<0&&selectedSite<0&&selectedSettlement<0&&selectedWorldCity<0&&selectedRegion<0))return;float scale=uiScale,width=inspectorWidth(w),height=inspectorHeight(h);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.008f,.025f,.05f,.90f));ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(.2f,.48f,.55f,1));
    fixedWindow("Active inspection",12*scale,h-height-24*scale,width,height);
    if(selectedUnit>=0)unitDockCard(selectedUnit);else if(selectedSite>=0){siteCard(selectedSite,false);if(sim.state.sites[selectedSite].type==Facility::Silo){ImGui::Separator();ImGui::Text("Missile range: %.0f km",sim.siteMissileRange(size_t(selectedSite)));if(button("Launch conventional")){attackMode=2;commandMode=false;notice("Choose a conventional missile target on the map; Escape cancels");}if(button("Launch nuclear"))ImGui::OpenPopup("Confirm nuclear targeting");if(ImGui::BeginPopupModal("Confirm nuclear targeting",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){wrapped("A nuclear strike damages friendly and hostile forces within its blast area and lowers national stability. Continue to choose a target?");if(button("Choose nuclear target")){attackMode=3;commandMode=false;notice("Nuclear targeting armed: choose a target on the map; Escape cancels");ImGui::CloseCurrentPopup();}ImGui::SameLine();if(button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();}wrapped("Select a target on the map. Launch checks range, readiness and war status.");}if(ImGui::CollapsingHeader("Capabilities"))capabilitiesCard();}else if(selectedSettlement>=0)cityCard(selectedSettlement);else if(selectedWorldCity>=0)worldCityCard(selectedWorldCity);else regionCard(selectedRegion);
    ImGui::End();ImGui::PopStyleColor(2);
  }
  void forcesPanel(){
    heading("FORCES");if(button(sim.state.war?"War in progress":"Declare war",!sim.state.war&&!sim.state.victory&&!sim.state.defeat))sim.declareWar();ImGui::SameLine();ImGui::Checkbox("Unit / radar ranges",&showRanges);
    heading("DEPLOYED FORMATIONS");int columns=std::max(1,int(ImGui::GetContentRegionAvail().x/(260.f*uiScale)));if(ImGui::BeginTable("Force catalog",columns,ImGuiTableFlags_SizingStretchSame)){for(size_t i=0;i<sim.state.units.size();++i){auto& u=sim.state.units[i];if(u.owner!=sim.state.player||u.health<=0||!u.deployed)continue;ImGui::TableNextColumn();ImGui::PushID(int(i));if(ImGui::Selectable(unitNames[int(u.type)],int(i)==selectedUnit)){selectedUnit=int(i);selectedSite=selectedSettlement=selectedWorldCity=-1;selectedUnits={int(i)};unitOpen=true;regionOpen=false;panel=None;center(u.position);}ImGui::TextColored(muted,"%.0f%% · %s",u.health,u.moving?"moving":"holding");ImGui::Separator();ImGui::PopID();}ImGui::EndTable();}
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
