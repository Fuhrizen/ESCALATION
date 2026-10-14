// Included inside App. Geographic overlays and original vector command assets.
  static float reveal(double zoom,double from,double to){
    float x=float(std::clamp((std::log(std::max(.01,zoom))-std::log(from))/(std::log(to)-std::log(from)),0.0,1.0));
    return x*x*(3-2*x);
  }
  void unitSymbol(ImDrawList* draw,Point point,UnitType type,ImU32 color,bool selected){
    command_icons::badge(draw,vec(point),16,int(type),color,selected);
  }
  void label(ImDrawList* d,Point p,const std::string& text,ImU32 color){
    auto size=ImGui::CalcTextSize(text.c_str());
    ImVec2 a{float(p.x)+9,float(p.y)-size.y*.5f};
    d->AddRectFilled({a.x-4,a.y-2},{a.x+size.x+4,a.y+size.y+2},IM_COL32(8,18,28,205),3);
    d->AddText(a,color,text.c_str());
  }
  void geographicalRing(ImDrawList* draw,Point centre,double kilometres,ImU32 color,bool dashed=false){
    if(kilometres<=0)return;
    constexpr double radians=.017453292519943295;
    double lat=centre.y*radians,lon=centre.x*radians,angle=kilometres/6371.0088;
    Point previous{};bool previousVisible=false;
    const int segments=128;
    for(int n=0;n<=segments;++n){
      double bearing=n*6.283185307179586/segments;
      double y=std::asin(std::sin(lat)*std::cos(angle)+std::cos(lat)*std::sin(angle)*std::cos(bearing));
      double x=lon+std::atan2(std::sin(bearing)*std::sin(angle)*std::cos(lat),std::cos(angle)-std::sin(lat)*std::sin(y));
      bool visible=false;Point p=camera.project({std::remainder(x/radians,360.0),y/radians},&visible);
      if(n&&visible&&previousVisible&&std::abs(p.x-previous.x)<camera.width*.5&&(!dashed||(n/3)%2==0))draw->AddLine(vec(previous),vec(p),color,1.6f);
      previous=p;previousVisible=visible;
    }
  }
  void regionEffect(ImDrawList* d,int region,ImU32 color){
    if(region<0||region>=int(renderer.shapes.size()))return;
    for(const auto& patch:renderer.shapes[region].patches){
      for(size_t n=0;n<patch.size();++n){
        bool va=false,vb=false;auto a=camera.project(patch[n],&va),b=camera.project(patch[(n+1)%patch.size()],&vb);
        if(va&&vb&&std::abs(a.x-b.x)<camera.width*.5)d->AddLine(vec(a),vec(b),color,2.2f);
      }
    }
  }
  void pressureOverlay(ImDrawList* draw){
    if(!showPressure||!sim.state.war||camera.zoom<3)return;
    const float cell=16;
    int columns=int(std::ceil(camera.width/cell))+1,rows=int(std::ceil(camera.height/cell))+1;
    struct Sample {Point screen;double value=0;bool land=false;};
    std::vector<Sample> samples(size_t(columns)*rows);
    auto color=[](double value){int a=int(std::clamp(std::abs(value)*42.0,0.0,42.0));return value>=0?IM_COL32(55,208,180,a):IM_COL32(228,107,99,a);};
    for(int y=0;y<rows;++y)for(int x=0;x<columns;++x){
      auto& s=samples[size_t(y)*columns+x];s.screen={camera.x+x*cell,camera.y+y*cell};auto p=camera.unproject(s.screen);if(!p)continue;
      s.value=sim.frontlinePressure(*p);
      if(std::abs(s.value)>.001){s.land=sim.regionAt(*p)>=0;if(!s.land)s.value=0;}
    }
    for(int y=0;y<rows-1;++y)for(int x=0;x<columns-1;++x){
      const Sample* c[]={&samples[size_t(y)*columns+x],&samples[size_t(y)*columns+x+1],&samples[size_t(y+1)*columns+x+1],&samples[size_t(y+1)*columns+x]};
      if(!c[0]->land||!c[1]->land||!c[2]->land||!c[3]->land)continue;
      draw->AddRectFilledMultiColor(vec(c[0]->screen),vec(c[2]->screen),color(c[0]->value),color(c[1]->value),color(c[2]->value),color(c[3]->value));
      std::vector<Point> crossings;
      for(int e=0;e<4;++e){const auto& a=*c[e];const auto& b=*c[(e+1)%4];if(a.value*b.value>=0)continue;double t=a.value/(a.value-b.value);crossings.push_back({a.screen.x+(b.screen.x-a.screen.x)*t,a.screen.y+(b.screen.y-a.screen.y)*t});}
      for(size_t n=1;n<crossings.size();n+=2)draw->AddLine(vec(crossings[n-1]),vec(crossings[n]),IM_COL32(247,202,125,210),1.8f);
    }
  }
  void geographicLine(ImDrawList* d,const std::vector<Point>& points,ImU32 color,float width){
    // Rounded cubic joins interpolate the real source vertices in screen space.
    // Geographic interpolation also prevents long paths cutting across the globe.
    std::vector<ImVec2> run;
    auto flush=[&](){
      if(run.size()==2)d->AddLine(run[0],run[1],color,width);
      else if(run.size()>2)for(size_t n=0;n+1<run.size();++n){
        auto a=run[n],b=run[n+1],before=n?run[n-1]:a,after=n+2<run.size()?run[n+2]:b;
        ImVec2 ca{a.x+(b.x-before.x)/6,a.y+(b.y-before.y)/6},cb{b.x-(after.x-a.x)/6,b.y-(after.y-a.y)/6};
        d->AddBezierCubic(a,ca,cb,b,color,width,0);
      }
      run.clear();
    };
    for(size_t n=0;n<points.size();++n){
      Point a=n?points[n-1]:points[n],b=points[n];double dx=std::remainder(b.x-a.x,360.0),dy=b.y-a.y;
      int steps=n?std::clamp(int(std::ceil(std::hypot(dx,dy)/.2)),1,60):1;
      for(int q=n?1:0;q<=steps;++q){
        double t=double(q)/steps;bool visible=false;auto p=camera.project({std::remainder(a.x+dx*t,360.0),a.y+dy*t},&visible);
        if(!visible){flush();continue;}
        if(!run.empty()&&std::abs(p.x-run.back().x)>camera.width*.5)flush();
        if(run.empty()||std::hypot(p.x-run.back().x,p.y-run.back().y)>1.2)run.push_back(vec(p));
      }
    }
    flush();
  }
  void roadsOverlay(ImDrawList* draw){
    if(!showRoads||camera.zoom<2)return;
    double aspect=double(camera.width)/camera.height;
    double radius=camera.globe?std::asin(std::min(1.0,std::hypot(aspect,1.0)/camera.zoom))*57.2957795:180*std::hypot(1.0,1.0/aspect)/camera.zoom;
    if(camera.globe&&std::abs(camera.latitude)+radius>=89)radius=180;
    for(int id:world.nearbyRoads({camera.longitude,camera.latitude},radius+.3)){
      const auto& road=world.roads[id];double start=2+road.scalerank*1.8;float fade=reveal(camera.zoom,start,start*2.6);if(fade<.01)continue;
      geographicLine(draw,road.points,IM_COL32(212,189,142,int(142*fade)),.85f+.7f*reveal(camera.zoom,16,80));
    }
  }
  void riversOverlay(ImDrawList* draw){
    if(!showRivers||camera.zoom<1.5)return;
    for(const auto& river:world.rivers){
      float fade=reveal(camera.zoom,1.5+river.scalerank*.35,4+river.scalerank*.7);if(fade<.01)continue;
      geographicLine(draw,river.points,IM_COL32(95,167,210,int(165*fade)),1.1f+1.0f*reveal(camera.zoom,14,100));
    }
  }
  void mapLabels(){
    if(screen==Menu)return;
    auto* draw=ImGui::GetBackgroundDrawList();draw->PushClipRect({camera.x,camera.y},{camera.x+camera.width,camera.y+camera.height},true);
    riversOverlay(draw);roadsOverlay(draw);
    if(screen==Game)pressureOverlay(draw);
    std::vector<Point> named;
    if(camera.zoom>2.5&&camera.zoom<22)for(size_t i=0;i<world.countries.size();++i){
      if(world.canonical(int(i))!=int(i)||world.countries[i].code=="ATA")continue;
      bool visible=false;auto p=camera.project(world.countries[i].center,&visible);if(!visible)continue;
      bool clear=true;for(auto q:named)if(std::hypot(q.x-p.x,q.y-p.y)<150)clear=false;
      if(clear){label(draw,p,world.countries[i].name,IM_COL32(145,166,180,160));named.push_back(p);}
    }
    if(screen==Game){
      auto& s=sim.state;
      if(showCities){
        std::vector<size_t> order(s.settlements.size());for(size_t n=0;n<order.size();++n)order[n]=n;
        std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){bool ma=sim.mainSettlement(s.settlements[a].region)==int(a),mb=sim.mainSettlement(s.settlements[b].region)==int(b);if(ma!=mb)return ma;return s.settlements[a].population>s.settlements[b].population;});
        std::vector<Point> labels;
        for(auto n:order){const auto& city=s.settlements[n];bool main=sim.mainSettlement(city.region)==int(n);bool visible=false;auto p=camera.project(city.position,&visible);if(!visible)continue;
          ImU32 c=city.owner==s.player?IM_COL32(133,215,194,255):IM_COL32(236,146,126,255);float r=main?4.2f:2.7f;
          draw->AddCircleFilled(vec(p),r+2,IM_COL32(8,17,28,240));draw->AddCircleFilled(vec(p),r,c);
          if(main)draw->AddCircle(vec(p),r+3,IM_COL32(234,213,167,200),20,1.0f);
          if(city.capture>0){draw->PathArcTo(vec(p),r+6,-1.5708f,-1.5708f+float(city.capture)*6.28318f,32);draw->PathStroke(IM_COL32(248,199,104,240),0,2);}
          bool clear=camera.zoom>(main?6:16)&&(city.population>0||city.region==selectedRegion);for(auto q:labels)if(std::hypot(q.x-p.x,q.y-p.y)<125)clear=false;
          if(clear){label(draw,p,city.name,c);labels.push_back(p);}
        }
      }
      for(size_t n=0;n<s.sites.size();++n){const auto& site=s.sites[n];if(site.destroyed)continue;bool visible=false;auto p=camera.project(site.position,&visible);if(!visible)continue;
        bool selected=selectedSite==int(n),hovered=hoverSite==int(n);ImU32 color=s.regions[site.region].owner==s.player?IM_COL32(105,197,214,255):IM_COL32(225,128,115,255);if(!site.enabled)color=IM_COL32(128,145,154,255);
        command_icons::badge(draw,vec(p),16,20+int(site.type),color,selected||hovered);
        if(showRanges&&(selected||hovered)){
          if(site.type==Facility::Radar){geographicalRing(draw,site.position,75,IM_COL32(116,197,224,170),true);label(draw,{p.x,p.y-25},"Radar coverage preview (75 km)",color);}
          else if(site.type==Facility::IndustrialPark||site.type==Facility::Factory)regionEffect(draw,site.region,IM_COL32(128,210,198,195));
        }
      }
      for(size_t n=0;n<s.units.size();++n){const auto& unit=s.units[n];if(unit.health<=0)continue;bool visible=false;Point actual=displayedUnit(n),p=camera.project(actual,&visible);if(!visible)continue;
        bool selected=std::find(selectedUnits.begin(),selectedUnits.end(),int(n))!=selectedUnits.end()||selectedUnit==int(n),hovered=hoverUnit==int(n);
        ImU32 color=unit.owner==s.player?IM_COL32(136,228,210,255):IM_COL32(245,143,122,255);if(unit.remaining>0)color=IM_COL32(226,189,115,255);
        if(showRanges&&(selected||hovered)&&unit.remaining<=0){geographicalRing(draw,actual,sim.unitRangeKm(unit),IM_COL32(248,190,100,185));if(camera.zoom>24)label(draw,{p.x,p.y-32},std::to_string(int(sim.unitRangeKm(unit)))+" km weapons",IM_COL32(250,206,131,235));}
        if(sim.state.war&&(selected||hovered)&&unit.remaining<=0){
          int target=-1;double nearest=sim.unitRangeKm(unit);
          for(size_t t=0;t<s.units.size();++t){const auto& enemy=s.units[t];if(enemy.owner==unit.owner||enemy.health<=0||enemy.remaining>0||!canEngage(unit.type,enemy.type))continue;double d=distanceKm(unit.position,enemy.position);if(d<=nearest){nearest=d;target=int(t);}}
          if(target>=0){
            Point end=displayedUnit(size_t(target));std::vector<Point> shot={actual,end};geographicLine(draw,shot,IM_COL32(248,190,100,115),1.3f);
            double pulse=std::fmod(s.day*simulationTickSeconds+accumulator,1.0);bool visible=false;
            auto spark=camera.project({std::remainder(actual.x+std::remainder(end.x-actual.x,360.0)*pulse,360.0),actual.y+(end.y-actual.y)*pulse},&visible);
            if(visible)draw->AddCircleFilled(vec(spark),2.5f,IM_COL32(255,218,146,220));
          }
        }
        if(selected&&unit.moving){std::vector<Point> route={actual};route.insert(route.end(),unit.waypoints.begin(),unit.waypoints.end());geographicLine(draw,route,IM_COL32(116,218,196,170),1.5f);}
        unitSymbol(draw,p,unit.type,color,selected||hovered);
        draw->AddRectFilled({float(p.x)-11,float(p.y)+13},{float(p.x)+11,float(p.y)+16},IM_COL32(6,15,24,235),1);
        draw->AddRectFilled({float(p.x)-11,float(p.y)+13},{float(p.x)-11+22*float(unit.health/100),float(p.y)+16},color,1);
      }
      if(construction>=0&&!ImGui::GetIO().WantCaptureMouse){
        auto geo=camera.unproject(pointer);int region=geo?sim.regionAt(*geo):-1;
        std::string reason=geo?sim.placementReason(Facility(construction),region,*geo):"Choose visible controlled land";
        ImU32 color=reason.empty()?IM_COL32(116,232,188,230):IM_COL32(239,123,111,230);
        command_icons::badge(draw,vec(pointer),23,20+construction,color,true);
        label(draw,{pointer.x+16,pointer.y},reason.empty()?"Place "+std::string(facilityNames[construction]):reason,color);
        if(geo&&showRanges&&Facility(construction)==Facility::Radar)geographicalRing(draw,*geo,75,IM_COL32(103,196,224,140),true);
      }
      if(leftStarted&&ImGui::IsMouseDragging(ImGuiMouseButton_Left,8)&&construction<0){ImVec2 a{float(std::min(leftStart.x,pointer.x)),float(std::min(leftStart.y,pointer.y))},b{float(std::max(leftStart.x,pointer.x)),float(std::max(leftStart.y,pointer.y))};draw->AddRectFilled(a,b,IM_COL32(89,207,176,25));draw->AddRect(a,b,IM_COL32(124,225,202,190));}
    }
    draw->PopClipRect();
  }
