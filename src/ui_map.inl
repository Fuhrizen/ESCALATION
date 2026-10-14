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
    if(!activeUnit(type))return IM_COL32(0,0,0,0);
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
  int cityPick(Point pointer) const{
    if(!showCities)return -1;int picked=-1;double nearest=14*uiScale;
    for(size_t n=0;n<sim.state.settlements.size();++n){bool visible=false;auto p=camera.project(sim.state.settlements[n].position,&visible);double distance=std::hypot(p.x-pointer.x,p.y-pointer.y);if(visible&&distance<nearest){nearest=distance;picked=int(n);}}return picked;
  }
  int worldCityPick(Point pointer) const{
    if(!showCities||camera.zoom<6)return -1;int picked=-1;double nearest=14*uiScale;
    for(size_t n=0;n<world.cities.size();++n){const auto& city=world.cities[n];if(camera.zoom<16&&city.population<500000)continue;bool visible=false;auto p=camera.project(city.position,&visible);double distance=std::hypot(p.x-pointer.x,p.y-pointer.y);if(visible&&distance<nearest){nearest=distance;picked=int(n);}}return picked;
  }
  // Original isometric meshes anchored to real projected geographic positions.
  // Building height is symbolic; reference population controls cluster density.
  void cityMesh(ImDrawList* draw,Point centre,double population,ImU32 color,bool selected=false){
    float fade=reveal(camera.zoom,5,25);if(fade<.01||population<=0)return;
    float scale=(.55f+.65f*reveal(camera.zoom,20,120))*uiScale;
    int count=std::clamp(int(std::log10(std::max(1.0,population)))*2-5,2,11);
    auto col=[&](int r,int g,int b,int a=255){return IM_COL32(r,g,b,int(a*fade));};
    auto prism=[&](float x,float y,float width,float depth,float height){
      auto project=[&](float ax,float ay,float z){return ImVec2{float(centre.x)+(ax-ay)*scale,float(centre.y)+(ax+ay)*.42f*scale-z*scale};};
      ImVec2 a=project(x,y,0),b=project(x+width,y,0),c=project(x+width,y+depth,0),e=project(x,y+depth,0),at=project(x,y,height),bt=project(x+width,y,height),ct=project(x+width,y+depth,height),et=project(x,y+depth,height);
      ImVec2 shadow[]={a,b,{c.x+height*.45f*scale,c.y+height*.24f*scale},{e.x+height*.45f*scale,e.y+height*.24f*scale}};draw->AddConvexPolyFilled(shadow,4,col(2,8,14,95));
      ImVec2 right[]={bt,b,c,ct},left[]={ct,c,e,et},roof[]={at,bt,ct,et};draw->AddConvexPolyFilled(right,4,col(65,82,94));draw->AddConvexPolyFilled(left,4,col(91,111,124));draw->AddConvexPolyFilled(roof,4,col(174,187,187));draw->AddPolyline(roof,4,col(206,216,207,150),ImDrawFlags_Closed,.6f);
      if(height>8&&scale>.7f)for(float z=3;z<height-2;z+=4){auto a=project(x+width,y+.8f,z),b=project(x+width,y+depth-.8f,z);draw->AddLine(a,b,col(194,199,159,130),.7f);}
    };
    draw->AddEllipseFilled({float(centre.x),float(centre.y)+3*scale},{18*scale,7*scale},col(23,37,38,115),0,24);
    for(int n=0;n<count;++n){float x=(n%4-1.5f)*6,y=(n/4-1.f)*6;float height=5+float((n*7+count*3)%14);if(n==count/2&&population>1000000)height+=12;prism(x,y,4,4,height);}
    if(selected)draw->AddEllipse(vec(centre),{22*scale,10*scale},color,0,32,1.5f);
  }
  void landmarksOverlay(ImDrawList* draw){
    if(!showCities)return;float fade=reveal(camera.zoom,12,40);if(fade<.01)return;
    struct Monument{const char* name;Point position;int model;};
    // Coordinates: official Eiffel access map, UNESCO property maps (Liberty,
    // Giza pyramid fields). Symbolic original meshes, no downloaded artwork.
    static const Monument monuments[]={{"Eiffel Tower",{2.2945,48.8584},0},{"Statue of Liberty",{-74.044722,40.689444},1},{"Giza pyramid fields",{31.13041,29.97604},2}};
    for(const auto& monument:monuments){bool visible=false;auto p=camera.project(monument.position,&visible);if(!visible)continue;float scale=(.7f+.6f*reveal(camera.zoom,30,120))*uiScale;
      auto at=[&](float x,float y,float z){return ImVec2{float(p.x)+(x-y)*scale,float(p.y)+(x+y)*.4f*scale-z*scale};};
      auto color=[&](int r,int g,int b){return IM_COL32(r,g,b,int(255*fade));};
      draw->AddEllipseFilled({float(p.x)+6*scale,float(p.y)+3*scale},{13*scale,5*scale},IM_COL32(0,6,13,int(95*fade)),0,24);
      auto line=[&](float x,float y,float z,float a,float b,float c){draw->AddLine(at(x,y,z),at(a,b,c),color(218,205,158),1.3f*scale);};
      if(monument.model==0){for(int x:{-1,1})for(int y:{-1,1}){line(x*8,y*8,0,x*3,y*3,20);line(x*3,y*3,20,0,0,42);}for(int z:{10,20,28}){float width=z==10?6:z==20?3:2;line(-width,-width,z,width,-width,z);line(width,-width,z,width,width,z);line(width,width,z,-width,width,z);}line(0,0,42,0,0,49);}
      else if(monument.model==1){ImVec2 front[]={at(-6,4,0),at(6,4,0),at(6,4,11),at(-6,4,11)};draw->AddConvexPolyFilled(front,4,color(165,159,138));ImVec2 side[]={at(6,4,0),at(6,-4,0),at(6,-4,11),at(6,4,11)};draw->AddConvexPolyFilled(side,4,color(102,113,104));ImVec2 robe[]={at(-4,0,11),at(4,0,11),at(2,0,28),at(-2,0,28)};draw->AddConvexPolyFilled(robe,4,color(109,166,143));draw->AddCircleFilled(at(0,0,31),2.3f*scale,color(131,190,164),12);draw->AddLine(at(1,0,26),at(8,0,39),color(131,190,164),3*scale);draw->AddCircleFilled(at(8,0,41),2*scale,color(245,215,112),12);for(int n=-2;n<=2;++n)draw->AddLine(at(n,0,32),at(n*2,0,36-std::abs(n)),color(131,190,164),scale);}
      else for(int n=0;n<3;++n){float x=n*17-16,y=n*4,width=12-n*2,height=20-n*4;ImVec2 a=at(x-width,y,0),b=at(x+width,y,0),c=at(x,y+width,0),top=at(x,y,height);draw->AddTriangleFilled(a,b,top,color(208,184,123));draw->AddTriangleFilled(b,c,top,color(164,137,87));}
      if(camera.zoom>28)label(draw,{p.x+12*scale,p.y-35*scale},monument.name,color(235,221,178));
    }
  }
  void mapLabels(){
    if(screen==Menu||(screen==Game&&panel!=None))return;
    auto* draw=ImGui::GetBackgroundDrawList();draw->PushClipRect({camera.x,camera.y},{camera.x+camera.width,camera.y+camera.height},true);
    riversOverlay(draw);roadsOverlay(draw);
    if(showCities&&camera.zoom>5){for(size_t n=0;n<world.cities.size();++n){const auto& city=world.cities[n];if(camera.zoom<16&&city.population<500000)continue;bool visible=false;auto p=camera.project(city.position,&visible);if(!visible||p.x<camera.x-40||p.x>camera.x+camera.width+40||p.y<camera.y-40||p.y>camera.y+camera.height+60)continue;cityMesh(draw,p,city.population,IM_COL32(170,197,188,255),selectedWorldCity==int(n));if(camera.zoom>40&&city.population>250000)label(draw,p,city.name,IM_COL32(182,207,194,215));}}
    landmarksOverlay(draw);
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
          if(selectedSettlement==int(n))draw->AddCircle(vec(p),r+9,IM_COL32(226,241,255,255),24,2);
          draw->AddCircleFilled(vec(p),r+2,IM_COL32(8,17,28,240));draw->AddCircleFilled(vec(p),r,c);
          if(main)draw->AddCircle(vec(p),r+3,IM_COL32(234,213,167,200),20,1.0f);
          if(city.capture>0){draw->PathArcTo(vec(p),r+6,-1.5708f,-1.5708f+float(city.capture)*6.28318f,32);draw->PathStroke(IM_COL32(248,199,104,240),0,2);}
          bool clear=camera.zoom>(main?6:16)&&(city.population>0||city.region==selectedRegion);for(auto q:labels)if(std::hypot(q.x-p.x,q.y-p.y)<125)clear=false;
          if(clear){label(draw,p,city.name,c);labels.push_back(p);}
        }
      }
      for(size_t n=0;n<s.sites.size();++n){const auto& site=s.sites[n];if(site.destroyed||site.type==Facility::Airbase)continue;bool visible=false;auto p=camera.project(site.position,&visible);if(!visible)continue;
        bool selected=selectedSite==int(n),hovered=hoverSite==int(n);ImU32 color=s.regions[site.region].owner==s.player?IM_COL32(102,178,249,255):IM_COL32(255,135,78,255);if(!site.enabled)color=IM_COL32(128,145,154,255);
        command_icons::badge(draw,vec(p),16,20+int(site.type),color,selected||hovered);
        if(showRanges&&(selected||hovered)){
          if(site.type==Facility::Radar){geographicalRing(draw,site.position,75,IM_COL32(116,197,224,170),true);label(draw,{p.x,p.y-25},"Radar coverage preview (75 km)",color);}
          else if(site.type==Facility::IndustrialPark||site.type==Facility::Factory)regionEffect(draw,site.region,IM_COL32(128,210,198,195));
        }
      }
      for(size_t n=0;n<s.units.size();++n){const auto& unit=s.units[n];if(unit.health<=0||!unit.deployed||!activeUnit(unit.type))continue;bool visible=false;Point actual=displayedUnit(n),p=camera.project(actual,&visible);if(!visible)continue;
        bool selected=std::find(selectedUnits.begin(),selectedUnits.end(),int(n))!=selectedUnits.end()||selectedUnit==int(n),hovered=hoverUnit==int(n);
        ImU32 color=unit.owner==s.player?IM_COL32(91,174,255,255):IM_COL32(255,135,78,255);if(unit.remaining>0)color=IM_COL32(226,189,115,255);
        if(showRanges&&(selected||hovered)&&unit.remaining<=0){auto weapons=sim.unitWeapons(unit);for(size_t w=0;w<weapons.size();++w){const auto& weapon=weapons[w];ImU32 ring=weapon.targetMask&4?IM_COL32(101,223,188,165):weapon.targetMask&2?IM_COL32(190,158,249,165):IM_COL32(98,181,255,165);geographicalRing(draw,actual,weapon.maxRangeKm,ring,w%2!=0);if(weapon.minRangeKm>0)geographicalRing(draw,actual,weapon.minRangeKm,ring,true);}}
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
      // Runtime engagements snapshot each actual fired shot and finite travel time.
      for(const auto& shot:s.engagements){double progress=std::clamp(1-shot.remaining/std::max(.001,shot.total),0.0,1.0);Point moving{std::remainder(shot.origin.x+std::remainder(shot.target.x-shot.origin.x,360.0)*progress,360.0),shot.origin.y+(shot.target.y-shot.origin.y)*progress};bool visible=false;auto p=camera.project(moving,&visible);if(!visible)continue;ImU32 color=shot.targetMask&4?IM_COL32(104,229,207,235):IM_COL32(255,219,149,235);geographicLine(draw,{shot.origin,moving},IM_COL32(245,208,139,105),1.2f,false,false);if(shot.guided){Point prior{std::remainder(shot.origin.x+std::remainder(shot.target.x-shot.origin.x,360.0)*std::max(0.0,progress-.02),360.0),shot.origin.y+(shot.target.y-shot.origin.y)*std::max(0.0,progress-.02)};auto q=camera.project(prior);double length=std::hypot(p.x-q.x,p.y-q.y);float dx=length>.01?float((p.x-q.x)/length):1,dy=length>.01?float((p.y-q.y)/length):0;draw->AddTriangleFilled({float(p.x)+dx*4,float(p.y)+dy*4},{float(p.x)-dx*3-dy*2,float(p.y)-dy*3+dx*2},{float(p.x)-dx*3+dy*2,float(p.y)-dy*3-dx*2},color);}else draw->AddCircleFilled(vec(p),2.f,color,12);}
      if(readyUnitPlacement>=0&&readyUnitPlacement<int(UnitType::Count)&&activeUnit(UnitType(readyUnitPlacement))&&!ImGui::GetIO().WantCaptureMouse){
        const auto type=UnitType(readyUnitPlacement);auto geo=camera.unproject(pointer);
        std::string reason=geo?sim.deploymentReason(type,*geo):(navalUnit(type)?"Choose visible water near a friendly shipyard":"Choose visible controlled land");
        const ImU32 color=reason.empty()?IM_COL32(91,174,255,230):IM_COL32(255,135,78,230);
        if(geo&&showRanges){Unit preview;preview.type=type;preview.owner=s.player;preview.position=*geo;for(const auto& weapon:sim.unitWeapons(preview))geographicalRing(draw,*geo,weapon.maxRangeKm,color,true);}
        unitSymbol(draw,pointer,type,color,true);
        label(draw,{pointer.x+24,pointer.y},reason.empty()?"Deploy "+std::string(unitNames[readyUnitPlacement]):reason,color);
      }
      if(construction>=0&&Facility(construction)!=Facility::Airbase&&!ImGui::GetIO().WantCaptureMouse){
        auto geo=camera.unproject(pointer);int region=geo?sim.regionAt(*geo):-1;
        std::string reason=geo?sim.placementReason(Facility(construction),region,*geo):(Facility(construction)==Facility::Shipyard?"Choose a visible controlled coast":"Choose visible controlled land");
        ImU32 color=reason.empty()?IM_COL32(91,174,255,230):IM_COL32(255,135,78,230);
        command_icons::badge(draw,vec(pointer),23,20+construction,color,true);
        label(draw,{pointer.x+16,pointer.y},reason.empty()?"Place "+std::string(facilityNames[construction]):reason,color);
        if(geo&&showRanges&&Facility(construction)==Facility::Radar)geographicalRing(draw,*geo,75,IM_COL32(103,196,224,140),true);
      }
      if(leftStarted&&ImGui::IsMouseDragging(ImGuiMouseButton_Left,8)&&construction<0&&readyUnitPlacement<0){ImVec2 a{float(std::min(leftStart.x,pointer.x)),float(std::min(leftStart.y,pointer.y))},b{float(std::max(leftStart.x,pointer.x)),float(std::max(leftStart.y,pointer.y))};draw->AddRectFilled(a,b,IM_COL32(71,152,255,25));draw->AddRect(a,b,IM_COL32(91,174,255,190));}
    }
    draw->PopClipRect();
  }
