// Included inside App; fixed command surfaces.
  float hudHeight() const{return 104.f*uiScale;}
  float commandHeight() const{return 58.f*uiScale;}
  float managementWidth() const{return 320.f*uiScale;}
  float inspectorWidth(float w) const{
    float left=panel!=None&&panel!=Research?managementWidth():0.f;
    return selectedUnit>=0?std::min(640.f*uiScale,std::max(280.f,w-left)):340.f*uiScale;
  }
  float inspectorHeight(float h) const{
    float available=std::max(80.f,h-hudHeight()-commandHeight());
    return selectedUnit>=0?std::min(244.f*uiScale,available):available;
  }
  float effectiveUiScale(float w,float h) const{
    auto framebuffer=ImGui::GetIO().DisplayFramebufferScale;
    float dpi=std::max(1.f,std::min(framebuffer.x,framebuffer.y));
    return std::clamp(std::min(w/1440.f,h/900.f)*std::sqrt(dpi),.85f,1.4f);
  }
  std::string campaignDate() const{
    static const char* months[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
    static const int lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
    int day=sim.state.day%365,month=0;
    while(month<11&&day>=lengths[month])day-=lengths[month++];
    return std::to_string(day+1)+" "+months[month]+" "+std::to_string(sim.year());
  }
  void allocationBar(float width,bool topbar=false){
    const ImU32 blue=IM_COL32(68,147,232,255),orange=IM_COL32(235,151,62,255);
    float budget=float(sim.state.researchBudget*100);
    ImGui::PushID(topbar?"Top allocation":"Panel allocation");
    ImGui::TextColored(ImVec4(.27f,.58f,.91f,1),"RESEARCH %.0f%%",budget);
    ImGui::SameLine();
    float constructionWidth=ImGui::CalcTextSize("BUILD 100%",nullptr).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(8.f,width-ImGui::GetCursorPosX()+ImGui::GetWindowContentRegionMin().x-constructionWidth));
    ImGui::TextColored(ImVec4(.92f,.59f,.24f,1),"BUILD %.0f%%",100-budget);
    ImVec2 p=ImGui::GetCursorScreenPos();float height=18.f*uiScale;
    ImGui::InvisibleButton("Allocation slider",{width,height});
    if(ImGui::IsItemActive()&&ImGui::IsMouseDown(ImGuiMouseButton_Left)){
      sim.state.researchBudget=std::clamp(double((ImGui::GetIO().MousePos.x-p.x)/width),0.,1.);sim.refreshRates();budget=float(sim.state.researchBudget*100);
    }
    if(ImGui::IsItemFocused()){
      float delta=ImGui::IsKeyPressed(ImGuiKey_RightArrow)?1.f:ImGui::IsKeyPressed(ImGuiKey_LeftArrow)?-1.f:0.f;
      if(delta){sim.state.researchBudget=std::clamp(double((budget+delta)/100),0.,1.);sim.refreshRates();budget=float(sim.state.researchBudget*100);}
    }
    auto* draw=ImGui::GetWindowDrawList();float split=p.x+width*budget/100;
    draw->AddRectFilled(p,{p.x+width,p.y+height},orange,3.f);
    if(budget>0)draw->AddRectFilled(p,{split,p.y+height},blue,3.f);
    draw->AddLine({split,p.y},{split,p.y+height},IM_COL32(230,242,248,255),3.f);
    draw->AddCircleFilled({split,p.y+height*.5f},5.f*uiScale,IM_COL32(235,243,248,255));
    if(ImGui::IsItemFocused())draw->AddRect({p.x-2,p.y-2},{p.x+width+2,p.y+height+2},IM_COL32(235,243,248,255),4.f);
    tooltip("Drag the divider: blue funds research, orange funds building and unit production. Left / right arrows adjust the focused slider. Estimates use 1x speed.");
    ImGui::PopID();
  }
  void hud(float w,float h){
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(16,8));
    fixedWindow("Main HUD",0,0,w,hudHeight());
    bool compact=w/uiScale<1350;
    ImGui::TextColored(accent,"ESCALATION");ImGui::SameLine();
    if(screen==Game){
      flag(sim.state.player,{24,18});ImGui::SameLine();
      ImGui::Text("%s  |  %s",(compact?world.countries[sim.state.player].code:world.countries[sim.state.player].name).c_str(),sim.state.war?"WAR":"PEACE");
      ImGui::SameLine();ImGui::TextColored(muted,"%s",campaignDate().c_str());
      ImGui::SameLine();if(button(compact?(paused?"Resume":"Pause"):(paused?"Resume [Space]":"Pause [Space]")))paused=!paused;
      ImGui::SameLine();ImGui::SetNextItemWidth(65*uiScale);const char* values[]={"1x","2x","4x","8x"};int v=speed==1?0:speed==2?1:speed==4?2:3;if(ImGui::Combo("##speed",&v,values,4))speed=1<<v;
      ImGui::SameLine();if(button(compact?"Options":"Options [Esc]"))openPause();
      allocationBar(ImGui::GetContentRegionAvail().x,true);
    }else ImGui::TextColored(accent,"SELECT A NATION ON THE MAP");
    ImGui::End();
    fixedWindow("Command HUD",0,h-commandHeight(),w,commandHeight());
    if(screen==Game){
      if(button("Build [B]"))toggle(Build);ImGui::SameLine();if(button("Research [R]"))toggle(Research);ImGui::SameLine();if(button("Forces [U]"))toggle(Forces);ImGui::SameLine();if(button("Log"))toggle(Log);ImGui::SameLine();
    }else {if(button("Back")){screen=Menu;nationOpen=false;selectedNation=-1;}ImGui::SameLine();}
    std::string status=construction>=0?std::string("Place ")+facilityNames[construction]+" | Shift repeats | Right click cancels":commandMode?"Click a destination":message.empty()?"WASD / arrows pan | Drag middle / right | Wheel zooms":message;
    float available=ImGui::GetContentRegionAvail().x;
    std::string visible=status;while(!visible.empty()&&ImGui::CalcTextSize(visible.c_str()).x>available)visible.pop_back();
    if(visible.size()<status.size()&&visible.size()>3)visible.replace(visible.size()-3,3,"...");
    ImGui::TextColored(muted,"%s",visible.c_str());tooltip(status);ImGui::End();ImGui::PopStyleVar();
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
