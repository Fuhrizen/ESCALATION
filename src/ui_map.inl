// Included inside App. Geographic overlays and original vector command assets.
  static float reveal(double zoom,double from,double to){
    float x=float(std::clamp((std::log(std::max(.01,zoom))-std::log(from))/(std::log(to)-std::log(from)),0.0,1.0));
    return x*x*(3-2*x);
  }
  void unitSymbol(ImDrawList* draw,Point point,UnitType type,ImU32 color,bool selected,bool hostile=false){
    command_icons::badge(draw,vec(point),16,int(type),color,selected,hostile);
  }
  void label(ImDrawList* d,Point p,const std::string& text,ImU32 color){
    auto size=ImGui::CalcTextSize(text.c_str());
    ImVec2 a{float(p.x)+9,float(p.y)-size.y*.5f};
    d->AddRectFilled({a.x-4,a.y-2},{a.x+size.x+4,a.y+size.y+2},IM_COL32(8,18,28,205),3);
    d->AddText(a,color,text.c_str());
  }
  static ImU32 attackColor(UnitType type,int alpha=190){
    if(!groundUnit(type))return IM_COL32(0,0,0,0);
    return IM_COL32(80,169,255,alpha);
  }
  void geographicalRing(ImDrawList* draw,Point centre,double kilometres,ImU32 color,bool dashed=false){
    if(kilometres<=0)return;
    constexpr double radians=.017453292519943295,pi=3.141592653589793;
    const double lat=centre.y*radians,lon=centre.x*radians,angle=std::min(pi,kilometres/6371.0088);
    auto destination=[&](double radius,double bearing){
      double y=std::asin(std::clamp(std::sin(lat)*std::cos(radius)+std::cos(lat)*std::sin(radius)*std::cos(bearing),-1.0,1.0));
      double x=lon+std::atan2(std::sin(bearing)*std::sin(radius)*std::cos(lat),std::cos(radius)-std::sin(lat)*std::sin(y));
      return Point{std::remainder(x/radians,360.0),y/radians};
    };
    // Clip the geographic mesh against the visible hemisphere before projection.
    // Viewport clipping remains ImGui's responsibility, so offscreen centres work.
    auto depth=[&](Point p){return camera.globe?std::sin(p.y*radians)*std::sin(camera.latitude*radians)+std::cos(p.y*radians)*std::cos(camera.latitude*radians)*std::cos((p.x-camera.longitude)*radians):1.0;};
    auto interpolate=[](Point a,Point b,double t){return Point{std::remainder(a.x+std::remainder(b.x-a.x,360.0)*t,360.0),a.y+(b.y-a.y)*t};};
    ImU32 fill=(color&~IM_COL32_A_MASK)|IM_COL32(0,0,0,dashed?12:24);
    auto triangle=[&](Point a,Point b,Point c){
      std::vector<Point> polygon{a,b,c},clipped;
      for(size_t i=0;i<polygon.size();++i){Point from=polygon[i],to=polygon[(i+1)%polygon.size()];bool va=depth(from)>=0,vb=depth(to)>=0;
        if(va)clipped.push_back(from);
        if(va!=vb){double lo=0,hi=1;for(int n=0;n<24;++n){double mid=(lo+hi)*.5;if((depth(interpolate(from,to,mid))>=0)==va)lo=mid;else hi=mid;}clipped.push_back(interpolate(from,to,(lo+hi)*.5));}
      }
      if(clipped.size()<3)return;
      std::vector<ImVec2> projected;for(auto p:clipped)projected.push_back(vec(camera.project(p)));
      double period=camera.width*camera.zoom;
      if(!camera.globe)for(size_t i=1;i<projected.size();++i)projected[i].x=float(projected[0].x+std::remainder(projected[i].x-projected[0].x,period));
      for(int copy=camera.globe?0:-1;copy<=(camera.globe?0:1);++copy){auto points=projected;float left=1e20f,right=-1e20f;for(auto& p:points){p.x+=float(copy*period);left=std::min(left,p.x);right=std::max(right,p.x);}if(right<camera.x||left>camera.x+camera.width)continue;draw->AddConvexPolyFilled(points.data(),int(points.size()),fill);}
    };
    const int segments=128,rings=std::clamp(int(std::ceil(angle/.025)),1,64);
    const ImDrawListFlags oldFlags=draw->Flags;draw->Flags&=~ImDrawListFlags_AntiAliasedFill;
    for(int r=0;r<rings;++r)for(int n=0;n<segments;++n){double b0=n*2*pi/segments,b1=(n+1)*2*pi/segments;
      Point a=destination(angle*r/rings,b0),b=destination(angle*(r+1)/rings,b0),c=destination(angle*(r+1)/rings,b1),e=destination(angle*r/rings,b1);
      triangle(a,b,c);if(r)triangle(a,c,e);
    }
    draw->Flags=oldFlags;
    Point previous=destination(angle,0);
    for(int n=1;n<=segments;++n){Point next=destination(angle,n*2*pi/segments);if(!dashed||(n/3)%2==0)geographicLine(draw,{previous,next},color,1.5f);previous=next;}
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
    auto color=[](double value){int a=int(std::clamp(std::abs(value)*42.0,0.0,42.0));return value>=0?IM_COL32(70,155,239,a):IM_COL32(244,131,78,a);};
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
  void geographicLine(ImDrawList* d,const std::vector<Point>& points,ImU32 color,float width,bool dashed=false,bool smoothCorners=true){
    if(points.size()<2)return;
    // Quadratic corner rounding stays in each source corner's convex hull.
    // Unlike screen-space Catmull-Rom, it cannot invent loops or overshoot banks.
    auto mix=[](Point a,Point b,double t){return Point{a.x+std::remainder(b.x-a.x,360.0)*t,a.y+(b.y-a.y)*t};};
    std::vector<Point> smooth{points.front()};
    for(size_t i=1;i+1<points.size();++i){if(!smoothCorners){smooth.push_back(points[i]);continue;}Point corner=points[i],a=mix(corner,points[i-1],.15),b=mix(corner,points[i+1],.15);
      smooth.push_back(a);for(int j=1;j<=6;++j){double t=j/6.0;smooth.push_back(mix(mix(a,corner,t),mix(corner,b,t),t));}
    }
    smooth.push_back(points.back());
    constexpr double rad=.017453292519943295;
    auto visibleSurface=[&](Point p){return !camera.globe||std::sin(p.y*rad)*std::sin(camera.latitude*rad)+std::cos(p.y*rad)*std::cos(camera.latitude*rad)*std::cos((p.x-camera.longitude)*rad)>=0;};
    int segment=0;
    for(size_t n=1;n<smooth.size();++n){Point a=smooth[n-1],b=smooth[n];double dx=std::remainder(b.x-a.x,360.0),dy=b.y-a.y;int steps=std::clamp(int(std::ceil(std::hypot(dx,dy)/.12)),1,256);
      for(int j=0;j<steps;++j,++segment){Point ga=mix(a,b,double(j)/steps),gb=mix(a,b,double(j+1)/steps);bool va=visibleSurface(ga),vb=visibleSurface(gb);if(!va&&!vb)continue;
        if(va!=vb){double lo=0,hi=1;for(int k=0;k<24;++k){double mid=(lo+hi)*.5;if(visibleSurface(mix(ga,gb,mid))==va)lo=mid;else hi=mid;}Point edge=mix(ga,gb,(lo+hi)*.5);if(va)gb=edge;else ga=edge;}
        if(dashed&&(segment/4)%2)continue;
        Point pa=camera.project(ga),pb=camera.project(gb);double period=camera.width*camera.zoom;
        if(!camera.globe)pb.x=pa.x+std::remainder(pb.x-pa.x,period);
        for(int copy=camera.globe?0:-1;copy<=(camera.globe?0:1);++copy){float shift=float(copy*period);if(std::max(pa.x,pb.x)+shift<camera.x||std::min(pa.x,pb.x)+shift>camera.x+camera.width)continue;d->AddLine({float(pa.x)+shift,float(pa.y)},{float(pb.x)+shift,float(pb.y)},color,width);}
      }
    }
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
    if(screen==Menu||(screen==Game&&panel!=None))return;
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
          ImU32 c=city.owner==s.player?IM_COL32(142,201,249,255):IM_COL32(255,153,95,255);float r=main?4.2f:2.7f;
          draw->AddCircleFilled(vec(p),r+2,IM_COL32(8,17,28,240));draw->AddCircleFilled(vec(p),r,c);
          if(main)draw->AddCircle(vec(p),r+3,IM_COL32(234,213,167,200),20,1.0f);
          if(city.capture>0){draw->PathArcTo(vec(p),r+6,-1.5708f,-1.5708f+float(city.capture)*6.28318f,32);draw->PathStroke(IM_COL32(248,199,104,240),0,2);}
          bool clear=camera.zoom>(main?6:16)&&(city.population>0||city.region==selectedRegion);for(auto q:labels)if(std::hypot(q.x-p.x,q.y-p.y)<125)clear=false;
          if(clear){label(draw,p,city.name,c);labels.push_back(p);}
        }
      }
      for(size_t n=0;n<s.sites.size();++n){const auto& site=s.sites[n];if(site.destroyed||site.type==Facility::Shipyard||site.type==Facility::Airbase)continue;bool visible=false;auto p=camera.project(site.position,&visible);if(!visible)continue;
        bool selected=selectedSite==int(n),hovered=hoverSite==int(n);ImU32 color=s.regions[site.region].owner==s.player?IM_COL32(102,178,249,255):IM_COL32(255,135,78,255);if(!site.enabled)color=IM_COL32(128,145,154,255);
        command_icons::badge(draw,vec(p),16,20+int(site.type),color,selected||hovered);
        if(showRanges&&(selected||hovered)){
          if(site.type==Facility::Radar){geographicalRing(draw,site.position,75,IM_COL32(116,197,224,170),true);label(draw,{p.x,p.y-25},"Radar coverage preview (75 km)",color);}
          else if(site.type==Facility::IndustrialPark||site.type==Facility::Factory)regionEffect(draw,site.region,IM_COL32(128,210,198,195));
        }
      }
      for(size_t n=0;n<s.units.size();++n){const auto& unit=s.units[n];if(unit.health<=0||!unit.deployed||!groundUnit(unit.type))continue;bool visible=false;Point actual=displayedUnit(n),p=camera.project(actual,&visible);if(!visible)continue;
        bool selected=std::find(selectedUnits.begin(),selectedUnits.end(),int(n))!=selectedUnits.end()||selectedUnit==int(n),hovered=hoverUnit==int(n);
        ImU32 color=unit.owner==s.player?IM_COL32(91,174,255,255):IM_COL32(255,135,78,255);if(unit.remaining>0)color=IM_COL32(226,189,115,255);
        if(showRanges&&(selected||hovered)&&unit.remaining<=0){geographicalRing(draw,actual,sim.unitRangeKm(unit),unit.owner==s.player?attackColor(unit.type):IM_COL32(255,135,78,170));}
        if(sim.state.war&&(selected||hovered)&&unit.remaining<=0){
          int target=-1;double nearest=sim.unitRangeKm(unit);
          for(size_t t=0;t<s.units.size();++t){const auto& enemy=s.units[t];if(enemy.owner==unit.owner||enemy.health<=0||!enemy.deployed||!groundUnit(enemy.type)||enemy.remaining>0||!canEngage(unit.type,enemy.type))continue;double d=distanceKm(unit.position,enemy.position);if(d<=nearest){nearest=d;target=int(t);}}
          if(target>=0){
            Point end=displayedUnit(size_t(target));std::vector<Point> shot={actual,end};geographicLine(draw,shot,IM_COL32(248,190,100,115),1.3f);
            double pulse=std::fmod(s.day*simulationTickSeconds+accumulator,1.0);bool visible=false;
            auto spark=camera.project({std::remainder(actual.x+std::remainder(end.x-actual.x,360.0)*pulse,360.0),actual.y+(end.y-actual.y)*pulse},&visible);
            if(visible)draw->AddCircleFilled(vec(spark),2.5f,IM_COL32(255,218,146,220));
          }
        }
        if(selected&&unit.moving){
          const auto& planned=orderPreview(n).path;
          std::vector<Point> route={actual};route.insert(route.end(),unit.waypoints.begin(),unit.waypoints.end());geographicLine(draw,route,IM_COL32(128,199,255,235),2.0f,true,false);
          if(!unit.pendingDestinations.empty()&&planned.size()>unit.waypoints.size()){
            std::vector<Point> queued{route.back()};queued.insert(queued.end(),planned.begin()+unit.waypoints.size(),planned.end());geographicLine(draw,queued,IM_COL32(170,211,242,185),1.4f,true,false);
          }
          label(draw,{p.x,p.y-32},"Arrival "+durationText(arrivalETA(n))+" at 1x",IM_COL32(184,218,250,240));
        }
        unitSymbol(draw,p,unit.type,color,selected||hovered,unit.owner!=s.player);
        for(int pip=0;pip<3;++pip){ImVec2 dot{float(p.x)+(pip-1)*6,float(p.y)-15};draw->AddCircleFilled(dot,2.4f,unit.health>pip*100.0/3?color:IM_COL32(33,48,67,230),12);draw->AddCircle(dot,2.4f,IM_COL32(6,16,30,230),12,1); }
      }
      if(readyUnitPlacement>=0&&readyUnitPlacement<int(UnitType::Count)&&groundUnit(UnitType(readyUnitPlacement))&&!ImGui::GetIO().WantCaptureMouse){
        const auto type=UnitType(readyUnitPlacement);auto geo=camera.unproject(pointer);
        std::string reason=geo?sim.deploymentReason(type,*geo):"Choose visible controlled land";
        const ImU32 color=reason.empty()?IM_COL32(91,174,255,230):IM_COL32(255,135,78,230);
        if(geo&&showRanges){Unit preview;preview.type=type;preview.owner=s.player;preview.position=*geo;geographicalRing(draw,*geo,sim.unitRangeKm(preview),color,true);}
        unitSymbol(draw,pointer,type,color,true);
        label(draw,{pointer.x+24,pointer.y},reason.empty()?"Deploy "+std::string(unitNames[readyUnitPlacement]):reason,color);
      }
      if(construction>=0&&Facility(construction)!=Facility::Shipyard&&Facility(construction)!=Facility::Airbase&&!ImGui::GetIO().WantCaptureMouse){
        auto geo=camera.unproject(pointer);int region=geo?sim.regionAt(*geo):-1;
        std::string reason=geo?sim.placementReason(Facility(construction),region,*geo):"Choose visible controlled land";
        ImU32 color=reason.empty()?IM_COL32(91,174,255,230):IM_COL32(255,135,78,230);
        command_icons::badge(draw,vec(pointer),23,20+construction,color,true);
        label(draw,{pointer.x+16,pointer.y},reason.empty()?"Place "+std::string(facilityNames[construction]):reason,color);
        if(geo&&showRanges&&Facility(construction)==Facility::Radar)geographicalRing(draw,*geo,75,IM_COL32(103,196,224,140),true);
      }
      if(leftStarted&&ImGui::IsMouseDragging(ImGuiMouseButton_Left,8)&&construction<0&&readyUnitPlacement<0){ImVec2 a{float(std::min(leftStart.x,pointer.x)),float(std::min(leftStart.y,pointer.y))},b{float(std::max(leftStart.x,pointer.x)),float(std::max(leftStart.y,pointer.y))};draw->AddRectFilled(a,b,IM_COL32(71,152,255,25));draw->AddRect(a,b,IM_COL32(91,174,255,190));}
    }
    draw->PopClipRect();
  }
