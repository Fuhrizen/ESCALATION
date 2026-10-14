// Included inside App; approved compact command header and permanent ready rail.
  float hudHeight() const{return (ImGui::GetIO().DisplaySize.x/uiScale<1120.f?104.f:56.f)*uiScale;}
  float commandHeight() const{return 0.f;}
  float managementWidth() const{return readyRailWidth();}
  float inspectorWidth(float w) const{return std::min((w/uiScale<900.f?420.f:780.f)*uiScale,w-readyRailWidth()-24.f*uiScale);}
  float inspectorHeight(float h) const{return std::min(260.f*uiScale,h-hudHeight()-20.f*uiScale);}
  float navigationReserveHeight() const{return 198.f*uiScale;}
  int readyItemKinds() const {int count=0;for(int id=0;id<16;++id)if(sim.state.buildingInventory[id]>0)++count;for(int id=0;id<9;++id)if(sim.readyUnitCount(UnitType(id))>0)++count;return count;}
  float readyRailWidth(float h) const {int rows=std::max(1,int((h-hudHeight()-navigationReserveHeight())/(64.f*uiScale)));return 64.f*uiScale*std::max(1,(readyItemKinds()+rows-1)/rows);}
  float readyRailWidth() const{return readyRailWidth(ImGui::GetIO().DisplaySize.y);}
  bool mapOverlayHit(Point p,float w,float h) const{
    if(p.y<hudHeight())return true;
    if(screen!=Game)return false;
    if(panel!=None)return true;
    if(p.x<readyRailWidth(h))return true;
    if(p.x<190.f*uiScale&&p.y>h-navigationReserveHeight())return true;
    if(selectedUnit>=0||selectedSite>=0||selectedRegion>=0){float width=inspectorWidth(w),height=inspectorHeight(h);if(p.x>w-width-12.f*uiScale&&p.y>h-height-12.f*uiScale)return true;}
    return false;
  }
  float effectiveUiScale(float w,float h) const{
    auto framebuffer=ImGui::GetIO().DisplayFramebufferScale;
    float dpi=std::max(1.f,std::min(framebuffer.x,framebuffer.y));
    return std::clamp(std::min(w/1440.f,h/900.f)*std::sqrt(dpi),.90f,1.4f);
  }
  std::string campaignDate() const{
    static const char* months[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
    static const int lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
    int day=sim.state.day%365,month=0;while(month<11&&day>=lengths[month])day-=lengths[month++];
    return std::to_string(day+1)+" "+months[month]+" "+std::to_string(sim.year());
  }
  void allocationBar(float width,bool topbar=false){
    ImGui::PushID(topbar?"Top allocation":"Panel allocation");
    float budget=float(sim.state.researchBudget*100);auto p=ImGui::GetCursorScreenPos();float height=34.f*uiScale;
    ImGui::InvisibleButton("Allocation slider",{width,height});
    if(ImGui::IsItemActive()&&ImGui::IsMouseDown(ImGuiMouseButton_Left)){sim.state.researchBudget=std::clamp(double((ImGui::GetIO().MousePos.x-p.x)/width),0.,1.);sim.refreshRates();budget=float(sim.state.researchBudget*100);}
    if(ImGui::IsItemFocused()){float delta=ImGui::IsKeyPressed(ImGuiKey_RightArrow)?1.f:ImGui::IsKeyPressed(ImGuiKey_LeftArrow)?-1.f:0.f;if(delta){sim.state.researchBudget=std::clamp(double((budget+delta)/100),0.,1.);sim.refreshRates();budget=float(sim.state.researchBudget*100);}}
    auto* d=ImGui::GetWindowDrawList();std::string a="Research "+std::to_string(int(budget))+"%",b="Build "+std::to_string(int(100-budget))+"%";
    d->AddText(p,IM_COL32(211,226,247,255),a.c_str());d->AddText({p.x+width-ImGui::CalcTextSize(b.c_str()).x,p.y},IM_COL32(255,184,115,255),b.c_str());
    ImVec2 q{p.x,p.y+24.f*uiScale};float split=q.x+width*budget/100.f;d->AddRectFilled(q,{q.x+width,q.y+8.f*uiScale},IM_COL32(245,143,57,255),4.f);if(budget>0)d->AddRectFilled(q,{split,q.y+8.f*uiScale},IM_COL32(47,142,246,255),4.f);d->AddCircleFilled({split,q.y+4.f*uiScale},5.f*uiScale,IM_COL32(222,237,255,255));
    tooltip("Drag to allocate national capacity between research and production. Arrow keys adjust by 1%.");ImGui::PopID();
  }
  void readyRail(float h){
    float s=uiScale,cell=64.f*s;int rows=std::max(1,int((h-hudHeight()-navigationReserveHeight())/cell));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{8.f*s,8.f*s});ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.018f,.04f,.075f,.08f));
    fixedWindow("Ready rail",0,hudHeight(),readyRailWidth(h),h-hudHeight());
    int index=0;auto item=[&](int kind,int count,const char* name,bool building,int id){int col=index/rows,row=index%rows;++index;ImGui::PushID(kind+100);ImGui::SetCursorPos({8.f*s+col*64.f*s,8.f*s+row*cell});auto p=ImGui::GetCursorScreenPos();bool active=building?construction==id:readyUnitPlacement==id;ImGui::InvisibleButton("Deploy ready item",{56.f*s,56.f*s});bool hover=ImGui::IsItemHovered();auto*d=ImGui::GetWindowDrawList();d->AddRectFilled(p,{p.x+56.f*s,p.y+56.f*s},active?IM_COL32(12,65,118,255):IM_COL32(4,22,44,255),4.f*s);d->AddRect(p,{p.x+56.f*s,p.y+56.f*s},active||hover?IM_COL32(72,167,255,255):IM_COL32(90,121,161,255),4.f*s,0,active?3.f:1.4f);command_icons::icon(d,{p.x+28.f*s,p.y+29.f*s},34.f*s,kind,IM_COL32(226,239,255,255));d->AddCircleFilled({p.x+49.f*s,p.y+5.f*s},11.f*s,IM_COL32(44,143,249,255));auto qty=std::to_string(count);auto sz=ImGui::CalcTextSize(qty.c_str());d->AddText({p.x+49.f*s-sz.x*.5f,p.y+5.f*s-sz.y*.5f},IM_COL32(255,255,255,255),qty.c_str());if(hover){ImGui::BeginTooltip();ImGui::Text("%s · %d ready",name,count);ImGui::TextUnformatted(building?"Click to place on controlled land":"Click to deploy on controlled land");ImGui::EndTooltip();}if(ImGui::IsItemClicked()){construction=building?id:-1;readyUnitPlacement=building?-1:id;panel=None;commandMode=false;notice(std::string("Choose controlled land to ")+(building?"place ":"deploy ")+name);}ImGui::PopID();};
    for(int id=0;id<9;++id){int count=sim.readyUnitCount(UnitType(id));if(count)item(id,count,unitNames[id],false,id);}for(int id=0;id<16;++id)if(sim.state.buildingInventory[id]>0)item(20+id,sim.state.buildingInventory[id],facilityNames[id],true,id);
    ImGui::End();ImGui::PopStyleColor();ImGui::PopStyleVar();
  }
  void navigation(float h){
    float s=uiScale;ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,0));
    ImGui::SetNextWindowPos({10.f*s,h-188.f*s});ImGui::SetNextWindowSize({180.f*s,178.f*s});ImGui::Begin("Command navigation",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    const char* names[]={"Research","Production","Forces","Log"};const Panel pages[]={Research,Build,Forces,Log};const int icons[]={26,24,0,30};
    for(int i=0;i<4;++i){ImGui::PushID(i);ImGui::SetCursorPos({float(i%2)*90.f*s,float(i/2)*90.f*s});auto p=ImGui::GetCursorScreenPos();ImGui::InvisibleButton(names[i],{84.f*s,84.f*s});bool hover=ImGui::IsItemHovered(),active=panel==pages[i];auto*d=ImGui::GetWindowDrawList();ImVec2 c{p.x+42.f*s,p.y+42.f*s};d->AddCircleFilled(c,41.f*s,IM_COL32(4,20,42,247),48);d->AddCircle(c,41.f*s,active||hover?IM_COL32(72,169,255,255):IM_COL32(102,137,188,255),48,active?3.f:2.f);if(i==2){for(int j=-1;j<=1;++j){float px=c.x+j*11.f*s,py=c.y-11.f*s+(j==0?-3.f*s:0);d->AddCircleFilled({px,py},5.f*s,IM_COL32(234,242,255,255));d->AddRectFilled({px-6.f*s,py+7.f*s},{px+6.f*s,py+17.f*s},IM_COL32(234,242,255,255),4.f*s);}}else if(i==3){d->AddRectFilled({c.x-11.f*s,c.y-24.f*s},{c.x+11.f*s,c.y+7.f*s},IM_COL32(234,242,255,255),2.f*s);for(int j=0;j<4;++j)d->AddLine({c.x-6.f*s,c.y-18.f*s+j*5.f*s},{c.x+6.f*s,c.y-18.f*s+j*5.f*s},IM_COL32(4,20,42,255),2.f*s);}else command_icons::icon(d,{c.x,c.y-7.f*s},30.f*s,icons[i],IM_COL32(234,242,255,255));auto size=ImGui::CalcTextSize(names[i]);d->AddText({c.x-size.x*.5f,c.y+17.f*s},IM_COL32(240,246,255,255),names[i]);if(ImGui::IsItemClicked())toggle(pages[i]);ImGui::PopID();}
    ImGui::End();ImGui::PopStyleColor();ImGui::PopStyleVar(2);
  }
  void hud(float w,float h){
    float s=uiScale;bool compact=w/s<1120.f;ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{16.f*s,10.f*s});ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{8.f*s,4.f*s});ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.018f,.04f,.075f,.99f));
    fixedWindow("Main HUD",0,0,w,hudHeight());
    if(titleFont)ImGui::PushFont(titleFont);ImGui::TextUnformatted("ESCALATION");if(titleFont)ImGui::PopFont();ImGui::SameLine(0,24.f*s);
    if(screen==Game){flag(sim.state.player,{26.f*s,18.f*s});ImGui::SameLine();ImGui::TextUnformatted(world.countries[sim.state.player].name.c_str());ImGui::SameLine(0,24.f*s);ImGui::TextUnformatted(campaignDate().c_str());
      if(compact)ImGui::SetCursorPos({16.f*s,52.f*s});else ImGui::SameLine(0,24.f*s);
      float controls=260.f*s;float allocationWidth=compact?std::max(170.f*s,w-controls-48.f*s):std::max(180.f*s,ImGui::GetContentRegionAvail().x-controls-24.f*s);allocationBar(allocationWidth,true);ImGui::SameLine(0,20.f*s);
      if(button(paused?"|| Paused":"> Running"))paused=!paused;ImGui::SameLine();ImGui::SetNextItemWidth(62.f*s);const char* values[]={"1x","2x","4x","8x"};int v=speed==1?0:speed==2?1:speed==4?2:3;if(ImGui::Combo("##speed",&v,values,4))speed=1<<v;ImGui::SameLine();if(button("Options"))openPause();
    }else{ImGui::SameLine();ImGui::TextColored(muted,"Select a nation on the map");}
    ImGui::End();ImGui::PopStyleColor();ImGui::PopStyleVar(2);if(screen==Game)readyRail(h);
  }
  void pauseMenu(float w,float h){
    if(!pauseOpen){if(ImGui::BeginPopupModal("Command options")){ImGui::CloseCurrentPopup();ImGui::EndPopup();}return;}
    ImGui::SetNextWindowPos({w*.5f-300, std::max(68.f,h*.5f-350)},ImGuiCond_Always);ImGui::SetNextWindowSize({600,std::min(700.f,h-136)},ImGuiCond_Always);
    ImGui::OpenPopup("Command options");
    if(ImGui::BeginPopupModal("Command options",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize)){
      heading("COMMAND PAUSED");
      if(ImGui::Button("Resume command",{-1,36})){pauseOpen=false;paused=wasPaused;saveSettings();ImGui::CloseCurrentPopup();}
      heading("CAMERA & DISPLAY");
      ImGui::SliderFloat("Keyboard pan speed",&panSpeed,.5f,5.f,"%.1fx");
      ImGui::SliderFloat("Drag speed",&dragSpeed,.5f,3.f,"%.1fx");
      ImGui::SliderFloat("Zoom sensitivity",&zoomSensitivity,.5f,3.f,"%.1fx");
      ImGui::Checkbox("Detect UI scale automatically",&autoUiScale);
      if(autoUiScale)ImGui::TextColored(muted,"Detected scale: %.2fx (window size / display density)",uiScale);
      ImGui::BeginDisabled(autoUiScale);
      ImGui::SliderFloat("Manual UI scale",&uiScale,.85f,1.4f,"%.2fx");
      ImGui::EndDisabled();
      ImGui::Checkbox("Globe projection",&camera.globe);ImGui::SameLine();ImGui::Checkbox("Terrain relief",&camera.terrain);
      ImGui::Checkbox("Cities",&showCities);ImGui::SameLine();ImGui::Checkbox("Roads",&showRoads);ImGui::SameLine();ImGui::Checkbox("Borders",&showBorders);
      ImGui::Checkbox("Rivers",&showRivers);ImGui::SameLine();ImGui::Checkbox("Frontline pressure",&showPressure);
      ImGui::Checkbox("Unit / radar ranges",&showRanges);
      wrapped("WASD or arrows pan. Middle / right drag tracks the map directly. Scroll zooms toward the cursor. F focuses your selected formation; H fits your nation.");
      heading("CAMPAIGN");
      if(ImGui::Button("Save campaign [F5]",{-1,36}))save();
      if(ImGui::Button("Save and return to menu",{-1,36})&&save()){screen=Menu;panel=None;clearSelection();pauseOpen=false;sim.state=Campaign{};saveSettings();ImGui::CloseCurrentPopup();}
      ImGui::EndPopup();
    }
  }
