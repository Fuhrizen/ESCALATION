// Included inside App; approved compact command header and permanent ready rail.
  float hudHeight() const{return 44.f*uiScale;}
  float commandHeight() const{return 0.f;}
  void managementPageHeader(float w){
    const float s=uiScale;
    ImGui::BeginChild("National workspace navigation",{0,56*s},ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetCursorPos({12*s,12*s});
    auto page=[&](const char* name,Panel target){
      if(panel==target)ImGui::PushStyleColor(ImGuiCol_Button,{.04f,.30f,.40f,1});
      bool chosen=ImGui::Button(name,{154*s,32*s});
      if(panel==target)ImGui::PopStyleColor();
      if(chosen&&panel!=target)toggle(target);
    };
    page("Research [R]",Research);ImGui::SameLine();page("Construction [B]",Build);
    ImGui::SetCursorPos({std::max(338*s,w-282*s),12*s});
    if(ImGui::Button(paused?"Resume [Space]":"Pause [Space]",{124*s,32*s})){paused=!paused;audio.play(SoundCue::Click);}
    ImGui::SameLine();if(ImGui::Button("Exit to map",{134*s,32*s}))toggle(panel);
    ImGui::EndChild();
  }
  float pageSafeLeft() const{return readyRailWidth();}
  float managementWidth() const{return pageSafeLeft();}
  float inspectorWidth(float w) const{return std::min(360.f*uiScale,w-224.f*uiScale);}
  float inspectorHeight(float h) const{return std::min(410.f*uiScale,h-hudHeight()-24.f*uiScale);}
  float navigationReserveHeight() const{return 430.f*uiScale;}
  int readyItemKinds() const {int count=0;for(int id=0;id<16;++id)if(sim.state.buildingInventory[id]>0)++count;for(int id=0;id<int(UnitType::Count);++id)if(sim.readyUnitCount(UnitType(id))>0)++count;return count;}
  float readyRailWidth(float h) const {int rows=std::max(1,int((h-hudHeight()-navigationReserveHeight()-16.f*uiScale)/(64.f*uiScale)));return 64.f*uiScale*std::max(1,(readyItemKinds()+rows-1)/rows);}
  float readyRailWidth() const{return readyRailWidth(ImGui::GetIO().DisplaySize.y);}
  bool mapOverlayHit(Point p,float w,float h) const{
    if(p.y<hudHeight())return true;
    if(screen!=Game)return false;
    if(panel!=None)return true;
    if(p.x<readyRailWidth(h))return true;
    if(p.x>w-390.f*uiScale&&p.y>h-182.f*uiScale)return true;
    if(selectedUnit>=0||selectedSite>=0||selectedRegion>=0||selectedSettlement>=0||selectedWorldCity>=0){float width=inspectorWidth(w),height=inspectorHeight(h);if(p.x<width+12.f*uiScale&&p.y>h-height-24.f*uiScale)return true;}
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
    float s=uiScale,cell=64.f*s;int rows=std::max(1,int((h-hudHeight()-navigationReserveHeight()-16.f*s)/cell));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.018f,.04f,.075f,.08f));
    ImGui::SetNextWindowPos({0,hudHeight()});ImGui::SetNextWindowSize({readyRailWidth(h),h-hudHeight()});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
    ImGui::Begin("Ready rail",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();
    int index=0;auto item=[&](int kind,int count,const char* name,bool building,int id){int col=index/rows,row=index%rows;++index;ImGui::PushID(kind+100);ImGui::SetCursorPos({4.f*s+col*64.f*s,12.f*s+row*cell});auto p=ImGui::GetCursorScreenPos();bool active=building?construction==id:readyUnitPlacement==id;bool activate=ImGui::InvisibleButton("Deploy ready item",{56.f*s,56.f*s},ImGuiButtonFlags_EnableNav);bool hover=ImGui::IsItemHovered()||ImGui::IsItemFocused();bool pressed=ImGui::IsItemActive();auto*d=ImGui::GetWindowDrawList();ImVec2 hex[6];for(int j=0;j<6;++j){float a=3.14159265f*j/3.f;hex[j]={p.x+28.f*s+27.f*s*std::cos(a),p.y+28.f*s+27.f*s*std::sin(a)};}d->AddConvexPolyFilled(hex,6,pressed?IM_COL32(24,91,118,255):active?IM_COL32(12,65,88,255):hover?IM_COL32(10,44,62,245):IM_COL32(4,22,34,225));d->AddPolyline(hex,6,active||hover?IM_COL32(72,217,239,255):IM_COL32(59,135,157,210),ImDrawFlags_Closed,active?3.f:1.4f);command_icons::icon(d,{p.x+28.f*s,p.y+29.f*s},34.f*s,kind,IM_COL32(226,239,255,255));auto qty=std::to_string(count);auto sz=ImGui::CalcTextSize(qty.c_str());float badge=std::max(11.f*s,sz.x*.5f+3.f*s);ImVec2 counter{p.x+56.f*s-badge,p.y+std::max(5.f*s,badge-8.f*s)};d->AddCircleFilled(counter,badge,IM_COL32(44,143,249,255));d->AddText({counter.x-sz.x*.5f,counter.y-sz.y*.5f},IM_COL32(255,255,255,255),qty.c_str());if(hover){ImGui::BeginTooltip();ImGui::Text("%s · %d ready",name,count);ImGui::TextUnformatted(building?(id==int(Facility::Shipyard)?"Click to place on controlled coast":"Click to place on controlled land"):(navalUnit(UnitType(id))?"Click to deploy in friendly coastal waters near a shipyard":"Click to deploy on controlled land"));ImGui::EndTooltip();}if(activate){measuring=false;measurePoints=0;construction=building?id:-1;readyUnitPlacement=building?-1:id;panel=None;commandMode=false;notice(std::string(!building&&navalUnit(UnitType(id))?"Choose friendly coastal waters near a shipyard to deploy ":building&&id==int(Facility::Shipyard)?"Choose controlled coast to place ":building?"Choose controlled land to place ":"Choose controlled land to deploy ")+name);}ImGui::PopID();};
    for(int id=0;id<int(UnitType::Count);++id){int count=sim.readyUnitCount(UnitType(id));if(count)item(id,count,unitNames[id],false,id);}for(int id=0;id<16;++id)if(sim.state.buildingInventory[id]>0)item(20+id,sim.state.buildingInventory[id],facilityNames[id],true,id);
    ImGui::End();ImGui::PopStyleColor();ImGui::PopStyleVar();
  }
  void navigation(float h){
    float s=uiScale,w=ImGui::GetIO().DisplaySize.x;auto*d=ImGui::GetForegroundDrawList();
    ImGui::SetNextWindowPos({w-390*s,h-180*s});ImGui::SetNextWindowSize({390*s,180*s});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
    ImGui::Begin("Command navigation",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
    const char* names[]={"Research","Production","Forces","Log"};const Panel pages[]={Research,Build,Forces,Log};const int icons[]={26,24,0,30};
    for(int i=0;i<4;++i){ImGui::PushID(i);ImGui::SetCursorPos({i*49.f*s,120*s});auto p=ImGui::GetCursorScreenPos();bool activate=ImGui::InvisibleButton(names[i],{46*s,46*s},ImGuiButtonFlags_EnableNav);bool hot=ImGui::IsItemHovered()||ImGui::IsItemFocused(),active=panel==pages[i],pressed=ImGui::IsItemActive();ImVec2 c{p.x+23*s,p.y+23*s};ImVec2 q[6];for(int j=0;j<6;++j){float a=float(j)*3.14159265f/3.f;q[j]={c.x+23*s*std::cos(a),c.y+23*s*std::sin(a)};}d->AddConvexPolyFilled(q,6,pressed?IM_COL32(23,85,110,255):hot?IM_COL32(12,52,72,255):IM_COL32(5,27,42,232));d->AddPolyline(q,6,active||hot?IM_COL32(71,224,244,255):IM_COL32(46,119,143,210),ImDrawFlags_Closed,1.5f*s);command_icons::icon(d,c,26*s,icons[i],IM_COL32(77,205,227,255));tooltip(names[i]);if(activate)toggle(pages[i]);ImGui::PopID();}
    ImGui::SetCursorPos({12*s,76*s});
    if(ImGui::Button(measuring?"Exit measure":"Measure km",{145*s,30*s})){
      measuring=!measuring;measurePoints=0;panel=None;attackMode=0;commandMode=false;construction=readyUnitPlacement=-1;
      notice(measuring?"Click two map points to measure their great-circle distance":"Measurement closed");
    }
    tooltip("Compare two points against weapon ranges. Distance follows the Earth's surface.");
    ImVec2 c{w-83*s,h-86*s};d->AddCircleFilled(c,74*s,IM_COL32(2,14,27,238),64);d->AddCircle(c,73*s,IM_COL32(60,153,182,220),64,2*s);d->AddCircle(c,53*s,IM_COL32(12,102,145,240),64,3*s);
    for(int j=0;j<32;++j){float a=3.14159265f+(j/31.f)*3.14159265f;ImVec2 a1{c.x+60*s*std::cos(a),c.y+60*s*std::sin(a)},a2{c.x+68*s*std::cos(a),c.y+68*s*std::sin(a)};d->AddLine(a1,a2,j<8+int(std::log2(speed))*8?IM_COL32(13,159,216,255):IM_COL32(25,65,85,210),3*s);}
    ImGui::SetCursorScreenPos({c.x-42*s,c.y-40*s});bool play=ImGui::InvisibleButton("Pause / play",{84*s,80*s},ImGuiButtonFlags_EnableNav);if(ImGui::IsItemHovered()||ImGui::IsItemFocused())d->AddCircle(c,48*s,IM_COL32(110,220,245,240),48,2*s);if(play){paused=!paused;audio.play(SoundCue::Click);}tooltip(paused?"Resume simulation [Space]":"Pause simulation [Space]");
    if(paused)d->AddTriangleFilled({c.x-12*s,c.y-24*s},{c.x-12*s,c.y+24*s},{c.x+25*s,c.y},IM_COL32(120,228,255,255));else{d->AddRectFilled({c.x-16*s,c.y-22*s},{c.x-5*s,c.y+22*s},IM_COL32(120,228,255,255));d->AddRectFilled({c.x+5*s,c.y-22*s},{c.x+16*s,c.y+22*s},IM_COL32(120,228,255,255));}
    ImGui::SetCursorScreenPos({c.x-105*s,c.y-62*s});ImGui::BeginDisabled(speed>=8);if(ImGui::Button("+",{25*s,25*s}))speed=std::min(8,speed*2);ImGui::EndDisabled();tooltip("Increase simulation speed (maximum 8x)");ImGui::SetCursorScreenPos({c.x-105*s,c.y-32*s});ImGui::BeginDisabled(speed<=1);if(ImGui::Button("-",{25*s,25*s}))speed=std::max(1,speed/2);ImGui::EndDisabled();tooltip("Decrease simulation speed (minimum 1x)");ImGui::SetCursorScreenPos({c.x-111*s,c.y+7*s});ImGui::Text("%dx",speed);
    ImGui::End();ImGui::PopStyleVar(2);
    if(measuring&&panel==None)measurementOverlay();
  }
  void measurementOverlay(){
    if(!measurePoints)return;
    Point end=measureEnd;
    if(measurePoints==1){auto geo=camera.unproject(pointer);if(geo)end=*geo;}
    const double radians=3.141592653589793/180.;
    auto vector=[&](Point p){double lon=p.x*radians,lat=p.y*radians;return std::array<double,3>{std::cos(lat)*std::cos(lon),std::cos(lat)*std::sin(lon),std::sin(lat)};};
    auto a=vector(measureStart),b=vector(end);
    double angle=std::acos(std::clamp(a[0]*b[0]+a[1]*b[1]+a[2]*b[2],-1.,1.)),sine=std::sin(angle);
    auto* draw=ImGui::GetForegroundDrawList();Point previous{};bool previousVisible=false;
    for(int i=0;i<=96;++i){
      double t=i/96.;Point geo;
      if(std::abs(sine)>.00001){double wa=std::sin((1-t)*angle)/sine,wb=std::sin(t*angle)/sine;double x=wa*a[0]+wb*b[0],y=wa*a[1]+wb*b[1],z=wa*a[2]+wb*b[2];geo={std::atan2(y,x)/radians,std::atan2(z,std::hypot(x,y))/radians};}
      else geo={measureStart.x+std::remainder(end.x-measureStart.x,360.)*t,measureStart.y+(end.y-measureStart.y)*t};
      bool visible=false;auto point=camera.project(geo,&visible);
      if(visible&&previousVisible&&std::hypot(point.x-previous.x,point.y-previous.y)<camera.width*.5)draw->AddLine({float(previous.x),float(previous.y)},{float(point.x),float(point.y)},IM_COL32(255,217,105,245),2*uiScale);
      if((i==0||i==96)&&visible){draw->AddCircle({float(point.x),float(point.y)},7*uiScale,IM_COL32(255,226,144,255),20,2*uiScale);}
      previous=point;previousVisible=visible;
    }
    char label[96];std::snprintf(label,sizeof(label),"SURFACE DISTANCE  %.1f km%s",distanceKm(measureStart,end),measurePoints==1?" / choose endpoint":"");
    ImVec2 at{readyRailWidth()+16*uiScale,hudHeight()+40*uiScale};auto size=ImGui::CalcTextSize(label);
    draw->AddRectFilled({at.x-8,at.y-6},{at.x+size.x+8,at.y+size.y+6},IM_COL32(5,22,35,230),4);
    draw->AddText(at,IM_COL32(255,226,144,255),label);
  }
  void hud(float w,float h){
    float s=uiScale;ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{10*s,8*s});ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({w,hudHeight()});
    ImGui::Begin("Main HUD",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
    if(button("Menu"))openPause();ImGui::SameLine();if(screen==Game){flag(sim.state.player,{23*s,16*s});ImGui::SameLine();ImGui::TextUnformatted(world.countries[sim.state.player].name.c_str());ImGui::SameLine(0,18*s);ImGui::TextUnformatted(campaignDate().c_str());
      float aw=std::min(320*s,w*.32f),start=ImGui::GetItemRectMax().x+24*s,space=w-aw-30*s-start;
      if(space>140*s){double level=sim.researchLevel();ImGui::SetCursorScreenPos({start,5*s});ImGui::Text("TECH LEVEL %.2f",level);ImGui::SetCursorScreenPos({start,29*s});ImGui::ProgressBar(float(level>=10?1:level-std::floor(level)),{std::min(210*s,space),5*s},"");}
      ImGui::SetCursorPos({w-aw-14*s,4*s});allocationBar(aw,true);
    }else{ImGui::SameLine();ImGui::TextUnformatted("ESCALATION / Select a nation");}
    ImGui::End();ImGui::PopStyleVar(2);if(screen==Game&&panel==None&&noticeRemaining>0){auto*d=ImGui::GetForegroundDrawList();ImVec2 p{readyRailWidth(h)+12*s,hudHeight()+8*s};float wrap=std::min(550*s,w-p.x-16*s);d->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{p.x+1,p.y+1},IM_COL32(0,0,0,240),message.c_str(),nullptr,wrap);d->AddText(ImGui::GetFont(),ImGui::GetFontSize(),p,IM_COL32(229,238,233,255),message.c_str(),nullptr,wrap);}if(screen==Game){if(panel!=Research&&panel!=Build)readyRail(h);auto*d=ImGui::GetBackgroundDrawList();d->AddRectFilledMultiColor({0,h-24*s},{w,h},IM_COL32(5,94,121,0),IM_COL32(5,94,121,0),IM_COL32(5,94,121,150),IM_COL32(5,94,121,150));auto date=campaignDate();auto size=ImGui::CalcTextSize(date.c_str());d->AddText({(w-size.x)*.5f,h-22*s},IM_COL32(183,224,238,255),date.c_str());}
  }
  void pauseMenu(float w,float h){
    if(!pauseOpen){if(ImGui::BeginPopupModal("Command options")){ImGui::CloseCurrentPopup();ImGui::EndPopup();}return;}
    ImGui::SetNextWindowPos({w*.5f-300, std::max(68.f,h*.5f-350)},ImGuiCond_Always);ImGui::SetNextWindowSize({600,std::min(700.f,h-136)},ImGuiCond_Always);
    ImGui::OpenPopup("Command options");
    if(ImGui::BeginPopupModal("Command options",nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize)){
      heading("COMMAND PAUSED");
      if(ImGui::Button("Resume command",{-1,36})){pauseOpen=false;paused=wasPaused;saveSettings();ImGui::CloseCurrentPopup();}
      heading("AUDIO");
      if(ImGui::Checkbox("Mute sound effects",&audioMuted))audio.setMuted(audioMuted);
      float effectsPercent=audioVolume*100.f;ImGui::BeginDisabled(audioMuted);
      if(ImGui::SliderFloat("Effects volume",&effectsPercent,0.f,100.f,"%.0f%%")){audioVolume=effectsPercent/100.f;audio.setVolume(audioVolume);}
      ImGui::EndDisabled();if(audioStarted&&!audio.available())ImGui::TextColored(muted,"Sound device unavailable.");
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
