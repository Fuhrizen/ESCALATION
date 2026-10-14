// Included inside App; fixed command surfaces.
  void hud(float w,float h){
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(16,8));
    fixedWindow("Main HUD",0,0,w,60);
    bool compact=w/uiScale<1350;
    ImGui::TextColored(accent,"ESCALATION");ImGui::SameLine();
    if(screen==Game){
      flag(sim.state.player,{24,18});ImGui::SameLine();
      ImGui::Text("%s  |  %s",(compact?world.countries[sim.state.player].code:world.countries[sim.state.player].name).c_str(),sim.state.war?"WAR":"PEACE");
      ImGui::SameLine();ImGui::TextColored(muted,"Day %d  /  %d",sim.state.day,sim.year());
      ImGui::SameLine();if(button(compact?(paused?"Resume":"Pause"):(paused?"Resume [Space]":"Pause [Space]")))paused=!paused;
      ImGui::SameLine();ImGui::SetNextItemWidth(65);const char* values[]={"1x","2x","4x","8x"};int v=speed==1?0:speed==2?1:speed==4?2:3;if(ImGui::Combo("##speed",&v,values,4))speed=1<<v;
      ImGui::SameLine();if(button(compact?"Options":"Options [Esc]"))openPause();
    }else ImGui::TextColored(accent,"SELECT A NATION ON THE MAP");
    ImGui::End();
    fixedWindow("Command HUD",0,h-58,w,58);
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
      ImGui::SliderFloat("UI scale",&uiScale,.85f,1.4f,"%.2fx");
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
