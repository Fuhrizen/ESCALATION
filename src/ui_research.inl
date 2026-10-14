// Included inside App. The complete DAG fixes positions; visibility gates every UI surface.
  int researchSelected=-1;
  float researchZoom=1.0f;
  ImVec2 researchPan{32,32};
  int researchField=-1;
  void researchIcon(ImDrawList* d,ImVec2 c,float size,int kind,ImU32 color){
    float u=size/32.0f;
    auto p=[&](float x,float y){return ImVec2{c.x+x*u,c.y+y*u};};
    auto rect=[&](float x,float y,float x2,float y2,float r=0){d->AddRectFilled(p(x,y),p(x2,y2),color,r*u);};
    auto line=[&](float x,float y,float x2,float y2,float width=2){d->AddLine(p(x,y),p(x2,y2),color,width*u);};
    auto disk=[&](float x,float y,float r){d->AddCircleFilled(p(x,y),r*u,color,16);};
    auto poly=[&](std::initializer_list<ImVec2> points){std::vector<ImVec2> q;for(auto v:points)q.push_back(p(v.x,v.y));d->AddConvexPolyFilled(q.data(),int(q.size()),color);};
    if(kind==0||kind==13){ // Three infantry silhouettes, or a patrol pair.
      for(int j=0;j<(kind==13?2:3);++j){float x=(j-1)*9;disk(x,-8,j==1?4:3.5f);rect(x-3,-3,x+3,7,2);line(x-2,5,x-3,13,2.5f);line(x+2,5,x+3,13,2.5f);line(x-3,-1,x-5,5);line(x+3,-1,x+5,5);}
    }else if(kind==1){ // Motor transport.
      rect(-15,-8,3,6,1);poly({{5,-6},{11,-6},{16,1},{16,7},{5,7}});disk(-9,9,4);disk(10,9,4);line(-15,8,16,8);d->AddRect(p(7,-4),p(11,0),IM_COL32(13,35,54,255),0,0,1.5f*u);
    }else if(kind==2){ // Tank silhouette.
      rect(-15,4,15,12,4);rect(-12,-1,12,6,2);rect(-5,-8,7,0,2);rect(3,-6,18,-3,1);for(int x=-10;x<=10;x+=5)d->AddCircle(p(float(x),8),2*u,IM_COL32(13,35,54,255),12,1*u);
    }else if(kind==3){ // Defence shield.
      ImVec2 q[]={p(-11,-13),p(11,-13),p(10,3),p(0,14),p(-10,3)};d->AddPolyline(q,5,color,ImDrawFlags_Closed,3*u);line(-6,-7,6,-7,2);
    }else if(kind==4){ // Sensor signal rings.
      disk(0,0,3);for(float r:{8.0f,14.0f}){d->PathArcTo(c,r*u,-1.15f,1.15f,16);d->PathStroke(color,0,2*u);d->PathArcTo(c,r*u,2.0f,4.28f,16);d->PathStroke(color,0,2*u);}
    }else if(kind==5){ // Factory sawtooth roof and chimney.
      poly({{-14,13},{-14,-1},{-5,-7},{-5,-1},{4,-7},{4,1},{14,1},{14,13}});rect(9,-13,14,1);for(int x=-9;x<12;x+=7)d->AddRectFilled(p(float(x),6),p(float(x+3),10),IM_COL32(13,35,54,255));
    }else if(kind==6){ // Laboratory flask.
      line(-5,-13,5,-13,3);line(-3,-12,-3,-4);line(3,-12,3,-4);poly({{-3,-4},{-12,12},{12,12},{3,-4}});d->AddLine(p(-6,4),p(6,4),IM_COL32(13,35,54,255),2*u);
    }else if(kind==7){ // Logistics crate.
      poly({{0,-13},{13,-7},{13,7},{0,14},{-13,7},{-13,-7}});d->AddLine(p(-13,-7),p(0,0),IM_COL32(13,35,54,255),1.5f*u);d->AddLine(p(13,-7),p(0,0),IM_COL32(13,35,54,255),1.5f*u);d->AddLine(p(0,0),p(0,14),IM_COL32(13,35,54,255),1.5f*u);d->AddLine(p(-7,-10),p(6,-3),IM_COL32(13,35,54,255),2*u);
    }else if(kind==8){ // Rifle cartridge.
      poly({{-5,3},{-5,-6},{0,-15},{5,-6},{5,3}});rect(-5,5,5,13,1);rect(-6,12,6,15,1);
    }else if(kind==9){ // Guided missile silhouette.
      poly({{-4,5},{-4,-6},{0,-16},{4,-6},{4,5}});poly({{-4,0},{-11,10},{-4,7}});poly({{4,0},{11,10},{4,7}});poly({{-3,9},{0,16},{3,9}});
    }else if(kind==10){ // Protective vest.
      poly({{-11,-13},{-5,-13},{-3,-6},{3,-6},{5,-13},{11,-13},{10,13},{-10,13}});d->AddLine(p(-10,4),p(10,4),IM_COL32(13,35,54,255),2*u);d->AddLine(p(-10,9),p(10,9),IM_COL32(13,35,54,255),2*u);
    }else if(kind==14){
      poly({{-15,3},{15,3},{9,12},{-9,12}});rect(-6,-6,6,3);line(0,-6,0,-15);line(-14,15,14,15,1);
    }else if(kind==11){ // Field medicine.
      rect(-12,-10,12,13,3);rect(-5,-14,5,-9,2);d->AddRectFilled(p(-3,-6),p(3,9),IM_COL32(13,35,54,255));d->AddRectFilled(p(-8,-1),p(8,4),IM_COL32(13,35,54,255));
    }else{ // Support gun on tripod.
      rect(-12,-6,10,0,1);rect(8,-5,17,-3);line(-2,0,-11,14,2.5f);line(-2,0,8,14,2.5f);line(-2,0,-2,12,2.5f);
    }
  }
  void researchTree(float w,float h){
    auto& s=sim.state;
    static char search[96]{};
    const float rail=pageSafeLeft(),top=hudHeight();
    const float nav=std::clamp(w*.17f,180.0f*uiScale,240.0f*uiScale);
    const float queueWidth=std::clamp(w*.155f,170.f*uiScale,230.f*uiScale);
    const ImU32 cyan=IM_COL32(43,188,246,255), pale=IM_COL32(217,234,247,255),soft=IM_COL32(134,176,204,255),green=IM_COL32(78,216,183,255);
    const int categories[]={0,4,1,2,5,6,7,8};
    const char* labels[]={"Ground forces","Missiles","Defence","Sensors","Industry","Science","Logistics","Navy"};
    const int icons[]={2,9,3,4,5,6,7,14};
    auto categoryName=[&](){for(int k=0;k<8;++k)if(categories[k]==focus)return labels[k];return "Special operations";};
    auto queueIndex=[&](int id){for(size_t q=0;q<s.research.size();++q)if(s.research[q].tech==id)return int(q);return -1;};
    auto visible=[&](int id){return id>=0&&id<int(sim.techs.size())&&sim.visibleResearch(id);};
    auto belongs=[&](int id){if(id<0||id>=int(sim.techs.size()))return false;if(int(sim.techs[id].field)==focus)return true;for(int p:sim.techs[id].prerequisites)if(int(sim.techs[p].field)==focus)return true;return false;};
    if(!visible(researchSelected)||!belongs(researchSelected))researchSelected=-1;
    bool fit=treeFocus||researchField!=focus,centerCurrent=false;
    researchField=focus;treeFocus=false;
    ImGui::SetNextWindowPos({rail,top});ImGui::SetNextWindowSize({w-rail,h-top});
    ImGui::PushStyleColor(ImGuiCol_WindowBg,{.008f,.025f,.044f,.95f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::Begin("##researchpage",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::BeginChild("##researchnav",{nav,std::max(210.0f,h-top-24.f*uiScale)},ImGuiChildFlags_None);
    auto* nd=ImGui::GetWindowDrawList();auto np=ImGui::GetWindowPos();
    nd->AddRectFilled(np,{np.x+nav,np.y+h-top},IM_COL32(4,15,28,215));
    nd->AddLine({np.x+nav-1,np.y},{np.x+nav-1,h},IM_COL32(32,88,109,180));
    ImGui::SetCursorPos({20,24});ImGui::TextColored({.43f,.68f,.84f,1},"RESEARCH");
    for(int k=0;k<8;++k){
      ImGui::SetCursorPos({8,58.0f+k*54+((focus==0||focus==3)&&k>0?26.0f:0.0f)});ImGui::PushID(k);
      auto a=ImGui::GetCursorScreenPos();bool active=focus==categories[k]||(k==0&&focus==3);
      ImGui::InvisibleButton("category",{nav-16,50});bool over=ImGui::IsItemHovered();
      if(active||over){nd->AddRectFilled(a,{a.x+nav-16,a.y+50},active?IM_COL32(18,56,88,255):IM_COL32(14,38,56,255),3);if(active){nd->AddRect(a,{a.x+nav-16,a.y+50},IM_COL32(34,147,211,255),3);nd->AddRectFilled(a,{a.x+3,a.y+50},cyan);}}
      researchIcon(nd,{a.x+27,a.y+25},28,icons[k],active?pale:soft);
      nd->AddText({a.x+52,a.y+16},active?pale:soft,labels[k]);
      if(ImGui::IsItemClicked()){focus=categories[k];researchSelected=-1;search[0]=0;fit=true;researchField=focus;}
      nd->AddLine({a.x+10,a.y+52},{a.x+nav-26,a.y+52},IM_COL32(24,52,72,255));ImGui::PopID();
    }
    if(focus==0||focus==3){ImGui::SetCursorPos({36,110});if(ImGui::Selectable("Special operations",focus==3,0,{nav-54,26})){focus=3;researchSelected=-1;search[0]=0;fit=true;researchField=focus;}}
    ImGui::EndChild();ImGui::SameLine(0,0);
    ImGui::BeginChild("##researchworkspace",{w-rail-nav-queueWidth,0},ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetCursorPos({26,23});if(titleFont)ImGui::PushFont(titleFont);ImGui::SetWindowFontScale(1.65f);ImGui::TextColored({.89f,.95f,.99f,1},"%s",categoryName());ImGui::SetWindowFontScale(1);if(titleFont)ImGui::PopFont();
    ImGui::SetCursorPos({27,67});ImGui::TextColored({.51f,.71f,.85f,1},"REVEALED TECHNOLOGIES");
    ImGui::SetCursorPos({27,102});if(button("Fit"))fit=true;ImGui::SameLine();if(button("Current")){for(const auto& job:s.research)if(visible(job.tech)&&belongs(job.tech)){researchSelected=job.tech;break;}fit=true;centerCurrent=true;}ImGui::SameLine();
    ImGui::SetNextItemWidth(210);ImGui::InputTextWithHint("##researchsearch","Search revealed technology",search,sizeof(search));ImGui::SameLine();ImGui::Checkbox("Auto queue",&autoResearch);
    ImGui::SetCursorPos({0,147});
    ImGui::BeginChild("##researchcanvas",{0,-39},ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    auto cp=ImGui::GetWindowPos();auto size=ImGui::GetWindowSize();auto* draw=ImGui::GetWindowDrawList();
    const float cardW=196,cardH=110,stepX=260,stepY=135;
    const int count=int(sim.techs.size());
    std::vector<int> depth(count,0),parent(count,-1),members;
    std::vector<std::vector<int>> children(count);
    for(int id=0;id<count;++id)if(belongs(id))members.push_back(id);
    // Longest prerequisite path establishes columns. A primary dependency establishes subtree lanes.
    for(int pass=0;pass<count;++pass){bool changed=false;for(int id:members)for(int p:sim.techs[id].prerequisites)if(belongs(p)&&depth[id]<depth[p]+1){depth[id]=depth[p]+1;changed=true;}if(!changed)break;}
    for(int id:members){for(int p:sim.techs[id].prerequisites)if(belongs(p)&&(parent[id]<0||depth[p]>depth[parent[id]]))parent[id]=p;if(parent[id]>=0)children[parent[id]].push_back(id);}
    std::vector<float> lane(count,0);float nextLane=0;
    std::function<float(int)> assign=[&](int id){if(children[id].empty())return lane[id]=nextLane++;float first=-1,last=-1;for(int child:children[id]){float y=assign(child);if(first<0)first=y;last=y;}return lane[id]=(first+last)*.5f;};
    for(int id:members)if(parent[id]<0){assign(id);nextLane+=.4f;}
    // Compact each depth in subtree order. Wide later forks do not force the
    // first three branches off screen, and positions never depend on visibility.
    std::map<int,std::vector<int>> columns;for(int id:members)columns[depth[id]].push_back(id);
    size_t widest=1;for(const auto& column:columns)widest=std::max(widest,column.second.size());
    for(auto& column:columns){auto& nodes=column.second;std::stable_sort(nodes.begin(),nodes.end(),[&](int a,int b){return lane[a]<lane[b];});for(size_t i=0;i<nodes.size();++i)lane[nodes[i]]=float(i)-float(nodes.size()-1)*.5f+float(widest-1)*.5f;}
    auto position=[&](int id){return ImVec2{depth[id]*stepX,lane[id]*stepY};};
    std::string query(search);std::transform(query.begin(),query.end(),query.begin(),[](unsigned char c){return char(std::tolower(c));});
    std::vector<int> shown;
    for(int id:members){if(!visible(id))continue;std::string name=sim.techs[id].name;std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});if(query.empty()||name.find(query)!=std::string::npos)shown.push_back(id);}
    auto displayed=[&](int id){return std::find(shown.begin(),shown.end(),id)!=shown.end();};
    if(fit&&!shown.empty()){
      float minX=1e9f,minY=1e9f,maxX=0,maxY=0;for(int id:shown){auto p=position(id);minX=std::min(minX,p.x);minY=std::min(minY,p.y);maxX=std::max(maxX,p.x+cardW);maxY=std::max(maxY,p.y+cardH);}
      float fitWidth=size.x-68;if(researchSelected>=0)fitWidth=std::max(460.0f,fitWidth-370.0f);
      researchZoom=std::clamp(std::min(fitWidth/(maxX-minX),(size.y-62)/(maxY-minY)),.82f,1.1f);
      researchPan={32-minX*researchZoom,(size.y-(maxY-minY)*researchZoom)*.5f-minY*researchZoom};
      // Keep the active selection accessible even when a mature tree requires panning.
      if(centerCurrent&&researchSelected>=0){auto p=position(researchSelected);researchPan.x=std::min(32.0f,size.x-cardW*researchZoom-370)-p.x*researchZoom;researchPan.y=(size.y-cardH*researchZoom)*.45f-p.y*researchZoom;}
    }
    bool hovered=ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if(hovered){auto& io=ImGui::GetIO();if(ImGui::IsMouseDragging(ImGuiMouseButton_Middle)||(ImGui::IsMouseDragging(ImGuiMouseButton_Left)&&!ImGui::IsAnyItemActive())){researchPan.x+=io.MouseDelta.x;researchPan.y+=io.MouseDelta.y;}if(io.MouseWheel!=0){float old=researchZoom;researchZoom=std::clamp(researchZoom*std::pow(1.12f,io.MouseWheel),.82f,1.6f);float factor=researchZoom/old;researchPan.x=(io.MousePos.x-cp.x)-(io.MousePos.x-cp.x-researchPan.x)*factor;researchPan.y=(io.MousePos.y-cp.y)-(io.MousePos.y-cp.y-researchPan.y)*factor;}}
    const float z=researchZoom,nw=cardW*z,nh=cardH*z;
    auto screenPos=[&](int id){auto p=position(id);return ImVec2{cp.x+researchPan.x+p.x*z,cp.y+researchPan.y+p.y*z};};
    draw->PushClipRect(cp,{cp.x+size.x,cp.y+size.y},true);
    const float grid=38*z;for(float x=std::fmod(researchPan.x,grid);x<size.x;x+=grid)draw->AddLine({cp.x+x,cp.y},{cp.x+x,cp.y+size.y},IM_COL32(21,49,68,30));for(float y=std::fmod(researchPan.y,grid);y<size.y;y+=grid)draw->AddLine({cp.x,cp.y+y},{cp.x+size.x,cp.y+y},IM_COL32(21,49,68,30));
    for(int id:shown)for(int p:sim.techs[id].prerequisites){if(!displayed(p))continue;auto a=screenPos(p),b=screenPos(id);a={a.x+nw,a.y+nh*.5f};b={b.x,b.y+nh*.5f};float mid=(a.x+b.x)*.5f;ImU32 col=s.completed[p]?IM_COL32(67,193,234,220):IM_COL32(103,149,175,180);draw->AddBezierCubic(a,{mid,a.y},{mid,b.y},b,col,2.2f);draw->AddLine({b.x-8,b.y-5},b,col,2);draw->AddLine({b.x-8,b.y+5},b,col,2);}
    auto text=[&](ImVec2 p,ImU32 color,const char* value,float wrap=0){draw->AddText(ImGui::GetFont(),std::max(15.0f,ImGui::GetFontSize()*z),p,color,value,nullptr,wrap);};
    for(int id:shown){auto a=screenPos(id);ImVec2 b{a.x+nw,a.y+nh};if(b.x<cp.x||a.x>cp.x+size.x||b.y<cp.y||a.y>cp.y+size.y)continue;const auto& tech=sim.techs[id];int qi=queueIndex(id);bool done=s.completed[id];ImU32 edge=done?green:qi>=0?IM_COL32(244,190,98,255):cyan;
      ImGui::SetCursorScreenPos(a);ImGui::PushID(id);ImGui::InvisibleButton("technology",{nw,nh});bool over=ImGui::IsItemHovered();if(ImGui::IsItemClicked())researchSelected=id;
      draw->AddRectFilled(a,b,over?IM_COL32(18,48,72,255):IM_COL32(7,28,40,228),1);draw->AddRectFilled({a.x+1,b.y-32*z},{b.x-1,b.y-1},done?IM_COL32(13,54,58,255):IM_COL32(14,44,66,255),4);draw->AddRect(a,b,edge,1,0,id==researchSelected?2.8f:1.2f);
      std::string iconName=tech.name;std::transform(iconName.begin(),iconName.end(),iconName.begin(),[](unsigned char c){return char(std::tolower(c));});
      int kind=focus==8?14:focus==4?9:focus==1?10:focus==2?4:focus==5?5:focus==6?6:focus==7?7:focus==3?13:8;
      if(iconName.find("foundation")!=std::string::npos&&focus==0)kind=0;
      if(iconName.find("mobility")!=std::string::npos||iconName.find("motorised")!=std::string::npos||iconName.find("transport")!=std::string::npos)kind=1;
      if(iconName.find("armour")!=std::string::npos||iconName.find("armoured")!=std::string::npos||iconName.find("mechanised")!=std::string::npos)kind=2;
      if(iconName.find("protection")!=std::string::npos||iconName.find("body armour")!=std::string::npos)kind=10;
      if(iconName.find("medicine")!=std::string::npos)kind=11;
      if(iconName.find("support weapons")!=std::string::npos||iconName.find("artillery")!=std::string::npos||iconName.find("guns")!=std::string::npos)kind=12;
      if(iconName.find("manpads")!=std::string::npos||iconName.find("guided missiles")!=std::string::npos||iconName.find("anti-tank")!=std::string::npos)kind=9;
      researchIcon(draw,{a.x+29*z,a.y+31*z},40*z,kind,pale);
      if(titleFont)draw->AddText(titleFont,std::max(15.0f,ImGui::GetFontSize()*z),{a.x+55*z,a.y+13*z},pale,tech.name.c_str(),nullptr,nw-67*z);else text({a.x+55*z,a.y+13*z},pale,tech.name.c_str(),nw-67*z);
      std::string summary=tech.effect;if(summary.size()>48)summary=summary.substr(0,45)+"...";

      const std::string status=done?"Complete":qi==0?"Researching":qi>0?"Queued #"+std::to_string(qi+1):"Available";
      if(done){draw->AddCircleFilled({a.x+17*z,b.y-16*z},8*z,green);draw->AddLine({a.x+13*z,b.y-16*z},{a.x+16*z,b.y-13*z},IM_COL32(8,38,43,255),2);draw->AddLine({a.x+16*z,b.y-13*z},{a.x+21*z,b.y-19*z},IM_COL32(8,38,43,255),2);}else researchIcon(draw,{a.x+17*z,b.y-16*z},15*z,6,edge);
      text({a.x+33*z,b.y-26*z},edge,status.c_str());
      if(qi==0){auto& job=s.research.front();float progress=std::clamp(float(job.progress/job.cost),0.0f,1.0f);draw->AddRectFilled({a.x+1,b.y-4},{a.x+(nw-2)*progress,b.y-1},edge);}
      ImGui::PopID();
    }
    if(shown.empty())draw->AddText({cp.x+30,cp.y+40},soft,"No revealed technology matches your search.");
    draw->PopClipRect();ImGui::EndChild();
    ImGui::SetCursorPos({25,ImGui::GetWindowHeight()-29});ImGui::TextColored({.43f,.63f,.77f,1},"Drag to pan  /  Scroll to zoom  /  Esc to return");
    ImGui::EndChild();ImGui::SameLine(0,0);
    ImGui::BeginChild("##nationalqueues",{queueWidth,0},ImGuiChildFlags_None);
    ImGui::TextColored({.68f,.81f,.85f,1},"SCIENCE / PRODUCTION");ImGui::Separator();if(button("Exit research"))panel=None;ImGui::Spacing();
    ImGui::BeginChild("##queuelist",{0,-192*uiScale},ImGuiChildFlags_None);
    for(size_t q=0;q<s.research.size();++q){auto& job=s.research[q];if(!visible(job.tech))continue;ImGui::PushID(int(q)+7000);auto p=ImGui::GetCursorScreenPos();researchIcon(ImGui::GetWindowDrawList(),{p.x+22*uiScale,p.y+24*uiScale},34*uiScale,6,IM_COL32(86,198,217,255));ImGui::Dummy({46*uiScale,46*uiScale});ImGui::SameLine();ImGui::BeginGroup();ImGui::TextWrapped("%s",sim.techs[job.tech].name.c_str());auto eta=sim.researchETA(job.tech);ImGui::TextColored(muted,"%s",std::isfinite(eta)?durationText(eta).c_str():"Paused");ImGui::EndGroup();ImGui::ProgressBar(float(job.progress/std::max(.01,job.cost)),{-1,5*uiScale},"");if(ImGui::IsItemClicked())researchSelected=job.tech;ImGui::PopID();ImGui::Spacing();}
    if(s.research.empty())ImGui::TextColored(muted,"No research queued");ImGui::Separator();
    for(size_t q=0;q<s.buildingQueue.size();++q){auto& job=s.buildingQueue[q];ImGui::TextWrapped("%s",facilityNames[int(job.type)]);ImGui::ProgressBar(float(job.progress/std::max(.01,job.total)),{-1,5*uiScale},"");ImGui::TextColored(muted,"%s",durationText(sim.buildingETA(q)).c_str());}
    for(size_t q=0;q<s.units.size();++q){auto& unit=s.units[q];if(unit.owner!=s.player||unit.remaining<=0)continue;ImGui::TextWrapped("%s",unitNames[int(unit.type)]);ImGui::TextColored(muted,"%s",durationText(sim.unitETA(q)).c_str());ImGui::Separator();}
    ImGui::EndChild();ImGui::EndChild();
    ImGui::End();ImGui::PopStyleVar();ImGui::PopStyleColor();
    // Compact, anchored details. No unrevealed prerequisite names can enter this panel.
    if(visible(researchSelected)&&belongs(researchSelected)){
      const auto& tech=sim.techs[researchSelected];int qi=queueIndex(researchSelected);
      float dw=std::min(352.0f*uiScale,w-rail-nav-queueWidth-32),dh=(s.completed[researchSelected]?180.0f:qi>=0?228.0f:276.0f)*uiScale;
      ImGui::SetNextWindowPos({w-queueWidth-dw-18,h-dh-49});ImGui::SetNextWindowSize({dw,dh});
      ImGui::PushStyleColor(ImGuiCol_WindowBg,{.025f,.07f,.11f,.99f});ImGui::PushStyleColor(ImGuiCol_Border,{.21f,.51f,.69f,1});
      ImGui::Begin("##researchdetails",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
      if(titleFont)ImGui::PushFont(titleFont);ImGui::TextWrapped("%s",tech.name.c_str());if(titleFont)ImGui::PopFont();ImGui::Spacing();wrapped(tech.effect);
      if(tech.sourceLevel>0){ImGui::TextColored(muted,"Technology level: %.1f / current %.2f",tech.sourceLevel,sim.researchLevel());ImGui::Text("Research points: %.0f",tech.sourcePoints);}else ImGui::TextColored(muted,"Reference era: %d",tech.year);
      for(int p:tech.prerequisites)if(visible(p))ImGui::TextWrapped("Requires: %s",sim.techs[p].name.c_str());
      if(s.completed[researchSelected])ImGui::TextColored(accent,"Capability complete");
      else if(qi>=0){auto eta=sim.researchETA(researchSelected);ImGui::TextWrapped("%s: %s",qi==0?"Remaining":"Ready in",std::isfinite(eta)?durationText(eta).c_str():"Paused");if(button("Remove from queue"))sim.removeResearch(size_t(qi));}
      else{ImGui::Text("Early research: %.1fx duration",sim.researchCost(researchSelected)/tech.cost);auto eta=sim.researchETA(researchSelected);ImGui::Text("Duration: %s",std::isfinite(eta)?durationText(eta).c_str():"Paused");ImGui::Spacing();ImGui::PushStyleColor(ImGuiCol_Button,{.02f,.43f,.72f,1});if(button("Queue research",sim.available(researchSelected))){if(sim.queueResearch(researchSelected))notice("Research queued: "+tech.name);}ImGui::PopStyleColor();}
      ImGui::End();ImGui::PopStyleColor(2);
    }
  }
