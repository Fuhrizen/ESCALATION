#include "escalation/renderer.hpp"
#include <algorithm>
#include <cmath>
#include <map>
namespace escalation {
  static constexpr double rad=0.017453292519943295;
  Point Camera::project(Point p,bool* visible) const {
    double dx=std::remainder(p.x-longitude,360.0),a=width/height,x,y,z=1;
    if(globe){
      double lat=p.y*rad,lon=dx*rad,tilt=latitude*rad;
      x=std::cos(lat)*std::sin(lon);
      double py=std::sin(lat),pz=std::cos(lat)*std::cos(lon);
      y=py*std::cos(tilt)-pz*std::sin(tilt);
      z=py*std::sin(tilt)+pz*std::cos(tilt);
      x=x/a*zoom;
      y*=zoom;
    }
    else{
      x=dx/180*zoom;
      y=(p.y-latitude)/180*a*zoom;
    }
    if(visible)*visible=z>0&&std::abs(x)<=1&&std::abs(y)<=1;
    return {this->x+(x+1)*width/2,this->y+(1-y)*height/2};
  }
  std::optional<Point> Camera::unproject(Point p) const {
    double x=(p.x-this->x)*2/width-1,y=1-(p.y-this->y)*2/height,a=width/height;
    if(!globe)return Point{std::remainder(longitude+x*180/zoom,360.0),std::clamp(latitude+y*180/a/zoom,-90.0,90.0)};
    x*=a/zoom;
    y/=zoom;
    double zz=1-x*x-y*y;
    if(zz<0)return {};
    double z=std::sqrt(zz),t=latitude*rad;
    double py=y*std::cos(t)+z*std::sin(t),pz=-y*std::sin(t)+z*std::cos(t);
    return Point{std::remainder(longitude+std::atan2(x,pz)/rad,360.0),std::asin(std::clamp(py,-1.0,1.0))/rad};
  }
  static const char* vertexSource=R"(#version 330 core
layout(location=0) in vec2 position;layout(location=1) in float country;
uniform vec2 center;uniform float zoom,aspect,offset,layer;uniform int globe,sky;
out float depth;out vec2 geo;flat out int id;
void main(){geo=position;id=int(country);if(sky==1){depth=1;gl_Position=vec4(position,0,1);return;}vec2 p=position+vec2(offset,0);if(globe==1){float lat=radians(p.y),lon=radians(p.x-center.x),tilt=radians(center.y);vec3 v=vec3(cos(lat)*sin(lon),sin(lat),cos(lat)*cos(lon));vec2 yz=mat2(cos(tilt),sin(tilt),-sin(tilt),cos(tilt))*v.yz;depth=yz.y;gl_Position=vec4(v.x/aspect*zoom,yz.x*zoom,-yz.y*layer*0.45,1);}else{depth=1;gl_Position=vec4((p.x-center.x)/180*zoom,(p.y-center.y)/180*aspect*zoom,0,1);}})";
  static const char* fragmentSource=R"(#version 330 core
in float depth;in vec2 geo;flat in int id;
uniform sampler2D palette,heightmap;uniform int globe,border,relief,sky;uniform vec2 viewport,center;uniform float zoom;out vec4 color;
float hash3(vec3 p){p=fract(p*0.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
float grain(vec3 p){
 vec3 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
 return mix(mix(mix(hash3(i),hash3(i+vec3(1,0,0)),f.x),mix(hash3(i+vec3(0,1,0)),hash3(i+vec3(1,1,0)),f.x),f.y),mix(mix(hash3(i+vec3(0,0,1)),hash3(i+vec3(1,0,1)),f.x),mix(hash3(i+vec3(0,1,1)),hash3(i+vec3(1,1,1)),f.x),f.y),f.z);
}
// Fixed Earth-space texture: longitude seam and polar convergence have no noise seam.
vec3 earthPoint(vec2 p){float a=radians(p.x),b=radians(p.y);return vec3(cos(b)*cos(a),sin(b),cos(b)*sin(a));}
// Fade each octave before it reaches one pixel. Stable geography-anchored detail
// replaces screen-space grain and avoids flicker when zooming or dragging.
float octave(vec3 p,float scale,float footprint){
 float fade=1.0-smoothstep(0.28,0.85,scale*footprint);
 if(fade<0.001)return 0.5;
 return mix(0.5,grain(p*scale),fade);
}
float dryZone(vec2 p,vec2 c,vec2 size){vec2 d=vec2(mod(p.x-c.x+180.0,360.0)-180.0,p.y-c.y)/size;return exp(-dot(d,d)*1.8);}
void main(){
 if(sky==1){
  // A fixed celestial field rotates with longitude rather than crawling with
  // frame time. Pixel-filtered stars stay sparse and stable at any resolution.
  vec2 uv=geo*0.5+0.5;
  vec2 starSpace=uv*vec2(viewport.x/5.0,viewport.y/5.0)+center*vec2(0.15,0.08);
  vec2 cell=floor(starSpace),local=fract(starSpace)-0.5;
  float seed=hash3(vec3(cell,27.0));
  float star=(1.0-smoothstep(0.05,0.18,length(local)))*step(0.985,seed);
  float haze=grain(vec3(uv*vec2(3.0,5.0),4.0));
  float band=exp(-pow((uv.y-0.20-uv.x*0.40)*7.0,2.0));
  vec3 space=mix(vec3(0.005,0.009,0.023),vec3(0.026,0.040,0.070),band*haze);
  vec2 sphere=geo*vec2(viewport.x/viewport.y,1.0);
  // Soft atmospheric rim surrounds the same orthographic globe as the map.
  float radius=length(sphere);
  float halo=exp(-abs(radius-zoom)*70.0/max(zoom,0.1));
  space+=vec3(0.025,0.105,0.175)*halo;
  color=vec4(space+vec3(0.52,0.62,0.78)*star,1.0);return;
 }
 if(globe==1&&depth<0)discard;
 vec4 base=texelFetch(palette,ivec2(id%16,id/16),0);
 if(relief==1&&border==0&&id!=254){
  vec2 uv=vec2((geo.x+180.0)/360.0,(90.0-geo.y)/180.0),pixel=1.0/vec2(textureSize(heightmap,0));
  vec4 surface=texture(heightmap,uv);
  vec3 earth=earthPoint(geo);
  float broad=grain(earth*48.0),footprint=max(length(dFdx(earth)),length(dFdy(earth)));
  float detailFade=1.0-smoothstep(0.002,0.014,footprint);
  float meso=octave(earth,220.0,footprint);
  float fine=octave(earth,820.0,footprint)-0.5;
  float canopy=octave(earth,2700.0,footprint);
  float micro=octave(earth,8100.0,footprint)-0.5;
  float closeGrain=octave(earth,24300.0,footprint)-0.5;
  if(id==255){
   // Coast distance illustrates a continental shelf; this is not bathymetric data.
   float shelf=1.0-smoothstep(0.0,0.7,surface.g);
   vec3 water=mix(vec3(0.035,0.083,0.109),vec3(0.105,0.210,0.235),shelf*shelf);
   water+=(broad-0.5)*0.008;
   float waves=sin(dot(earth,vec3(680.0,210.0,320.0))+broad*5.0);
   water+=waves*0.0025*detailFade*shelf;
   base=vec4(water,1.0);
  }else{
   float h=surface.r;
   // Relative elevation is geographic; climate colours are illustrative atlas styling.
   float arid=max(max(dryZone(geo,vec2(20,24),vec2(38,15)),dryZone(geo,vec2(46,26),vec2(19,12))),max(dryZone(geo,vec2(134,-25),vec2(22,14)),dryZone(geo,vec2(70,42),vec2(30,12))));
   arid=max(arid,max(dryZone(geo,vec2(-112,34),vec2(16,13))*0.68,dryZone(geo,vec2(20,-24),vec2(15,12))*0.8));
   arid=clamp(arid+(broad-0.5)*0.20,0.0,1.0);
   // Original illustrative land-cover shading; the source atlas supplies real
   // relative elevation, while these geographic-space textures supply surface detail.
   float hWest=texture(heightmap,uv-vec2(pixel.x,0)).r;
   float hEast=texture(heightmap,uv+vec2(pixel.x,0)).r;
   float hNorth=texture(heightmap,uv-vec2(0,pixel.y)).r;
   float hSouth=texture(heightmap,uv+vec2(0,pixel.y)).r;
   float dx=hEast-hWest,dy=hSouth-hNorth;
   float slope=length(vec2(dx,dy));
   float rock=smoothstep(0.15,0.48,h);
   float mountain=max(rock,smoothstep(0.015,0.090,slope));
   float woodland=(1.0-arid)*(1.0-smoothstep(0.28,0.58,h));
   woodland*=smoothstep(0.30,0.72,meso+broad*0.14);
   vec3 meadow=mix(vec3(0.265,0.310,0.175),vec3(0.400,0.390,0.240),meso);
   vec3 forest=mix(vec3(0.105,0.180,0.115),vec3(0.220,0.270,0.150),canopy);
   vec3 terrain=mix(meadow,forest,woodland*0.78);
   terrain=mix(terrain,mix(vec3(0.490,0.390,0.230),vec3(0.650,0.565,0.360),meso),arid);
   vec3 stone=mix(vec3(0.260,0.280,0.210),vec3(0.525,0.475,0.340),meso);
   terrain=mix(terrain,stone,rock*0.80);
   // Sharper ridge/valley microrelief is artistic detail, aligned to the large
   // scale elevation slope. It supplements rather than moves actual mountains.
   vec3 stretch=earth*vec3(1.0,1.65,0.82)+vec3(meso*0.002);
   float ridgeA=1.0-abs(octave(stretch,1500.0,footprint*1.65)*2.0-1.0);
   float ridgeB=1.0-abs(canopy*2.0-1.0);
   float ridge=(ridgeA-0.63)*0.25+(ridgeB-0.63)*0.12+micro*0.10;
   float snow=max(smoothstep(0.56,0.79,h),smoothstep(64.0,79.0,abs(geo.y)));
   terrain=mix(terrain,vec3(0.800,0.805,0.750),snow);
   vec3 normal=normalize(vec3(-dx*24.0,dy*24.0,1.0));
   float shade=0.57+0.57*max(0.0,dot(normal,normalize(vec3(-0.65,0.72,0.85))));
   terrain*=shade+ridge*mountain*(1.0-snow*0.65);
   // Agricultural mosaics on flatter, temperate ground. Cell edges are
   // filtered at distance, and do not imply mapped farmland boundaries.
   float farm=(1.0-woodland)*(1.0-arid)*(1.0-mountain)*(1.0-snow);
   vec3 cells=earth*1700.0;
   vec3 edge=min(fract(cells),1.0-fract(cells));
   float fieldEdge=1.0-smoothstep(0.025,0.070,min(edge.x,min(edge.y,edge.z)));
   float cellFade=1.0-smoothstep(0.15,0.70,footprint*1700.0);
   float fieldShade=hash3(floor(cells))-0.5;
   terrain+=(fieldShade*0.038-fieldEdge*0.024)*farm*cellFade;
   terrain+=fine*0.036+micro*mix(0.032,0.055,woodland)+closeGrain*0.024;
   // Palette alpha is ownership tint strength; geographic fills remain opaque.
   base=vec4(mix(terrain,base.rgb,base.a),1.0);
  }
 }
 if(border==0&&id<254)base.a=1.0;
 if(border==1)base=vec4(base.rgb*1.35,0.48);
 float light=globe==1?0.76+0.24*sqrt(max(depth,0)):1.0;
 vec3 shaded=base.rgb*light;
 if(globe==1&&border==0){
  // Atmospheric scattering follows the true sphere limb, so it remains
  // attached to coastlines as the globe rotates and the camera zooms.
  float limb=pow(1.0-clamp(depth,0.0,1.0),6.0);
  shaded=mix(shaded,vec3(0.12,0.34,0.48),limb*0.32);
 }
 color=vec4(shaded,base.a);
})";
  static gl::UInt shader(gl::Enum type,const char* source){
    auto s=gl::CreateShader(type);
    gl::ShaderSource(s,1,&source,nullptr);
    gl::CompileShader(s);
    int ok=0;
    gl::GetShaderiv(s,gl::COMPILE_STATUS,&ok);
    if(!ok){
      char error[2048];
      gl::GetShaderInfoLog(s,2048,nullptr,error);
      throw std::runtime_error(std::string("Shader compile: ")+error);
    }
    return s;
  }
  Renderer::Mesh Renderer::upload(const std::vector<Vertex>& vertices){
    Mesh m;
    m.count=int(vertices.size());
    gl::GenVertexArrays(1,&m.vao);
    gl::GenBuffers(1,&m.vbo);
    gl::BindVertexArray(m.vao);
    gl::BindBuffer(gl::ARRAY_BUFFER,m.vbo);
    gl::BufferData(gl::ARRAY_BUFFER,std::ptrdiff_t(vertices.size()*sizeof(Vertex)),vertices.data(),gl::STATIC_DRAW);
    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(0,2,gl::FLOAT,0,sizeof(Vertex),nullptr);
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(1,1,gl::FLOAT,0,sizeof(Vertex),reinterpret_cast<void*>(sizeof(float)*2));
    return m;
  }
  void Renderer::destroy(Mesh& m){
    if(m.vbo)gl::DeleteBuffers(1,&m.vbo);
    if(m.vao)gl::DeleteVertexArrays(1,&m.vao);
    m={};
  }
  void Renderer::initialise(const World& world,const std::string& data){
    if(world.countries.size()>192)throw std::runtime_error("Map exceeds the country palette capacity");
    assets.load(data,world);
    version=reinterpret_cast<const char*>(gl::GetString(gl::VERSION));
    auto vs=shader(gl::VERTEX_SHADER,vertexSource),fs=shader(gl::FRAGMENT_SHADER,fragmentSource);
    program_=gl::CreateProgram();
    gl::AttachShader(program_,vs);
    gl::AttachShader(program_,fs);
    gl::LinkProgram(program_);
    int ok=0;
    gl::GetProgramiv(program_,gl::LINK_STATUS,&ok);
    if(!ok){
      char error[2048];
      gl::GetProgramInfoLog(program_,2048,nullptr,error);
      throw std::runtime_error(error);
    }
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);
    auto uniform=[&](const char* name){return gl::GetUniformLocation(program_,name);};
    uniforms_={uniform("layer"),uniform("border"),uniform("offset"),uniform("heightmap"),uniform("relief"),uniform("palette"),uniform("center"),uniform("zoom"),uniform("aspect"),uniform("globe"),uniform("sky"),uniform("viewport")};
    sky_=upload({{-1,-1,253},{1,-1,253},{1,1,253},{-1,-1,253},{1,1,253},{-1,1,253}});
    gl::GenTextures(1,&palette_);
    gl::BindTexture(gl::TEXTURE_2D,palette_);
    gl::TexParameteri(gl::TEXTURE_2D,gl::TEXTURE_MIN_FILTER,gl::NEAREST);
    gl::TexParameteri(gl::TEXTURE_2D,gl::TEXTURE_MAG_FILTER,gl::NEAREST);
    gl::TexImage2D(gl::TEXTURE_2D,0,gl::RGBA32F,16,16,0,gl::RGBA,gl::FLOAT,nullptr);
    paletteReady_=false;
    std::vector<Vertex> land,borders,ocean,grid;
    auto emit=[&](std::vector<Vertex>& v,Point p,int id){
      v.push_back({float(p.x),float(p.y),float(id)});
    };
    auto triangle=[&](auto&& self,std::array<Point,3> t,int id,int level)->void{double d[3];for(int i=0;i<3;++i)d[i]=std::hypot(t[i].x-t[(i+1)%3].x,t[i].y-t[(i+1)%3].y);int k=int(std::max_element(d,d+3)-d);if(d[k]>1.5&&level<12){int next=(k+1)%3,third=(k+2)%3;Point mid{(t[k].x+t[next].x)/2,(t[k].y+t[next].y)/2};self(self,{t[k],mid,t[third]},id,level+1);self(self,{mid,t[next],t[third]},id,level+1);}else for(auto p:t)emit(land,p,id);};
    struct BorderEdge { Point a,b; int owner=0,count=0; };
    std::map<std::array<long long,4>,BorderEdge> edges;
    auto keyPoint=[](Point p){double x=std::remainder(p.x,360.0);if(x>=180)x-=360;return std::array<long long,2>{std::llround(x*1000000),std::llround(p.y*1000000)};};
    for(size_t i=0;i<world.countries.size();++i){
      if(world.canonical(int(i))!=int(i))continue;
      const auto& c=world.countries[i];
      for(auto t:c.triangles)triangle(triangle,t,int(i),0);
      // Canonical groups contain each child's geometry. Shared edges inside a
      // group occur twice and are removed rather than drawing old micro-borders.
      for(const auto& p:c.polygons)for(const auto& ring:p.rings)for(size_t k=0;k<ring.size();++k){
        Point a=ring[k],b=ring[(k+1)%ring.size()];auto ka=keyPoint(a),kb=keyPoint(b);if(kb<ka){std::swap(ka,kb);std::swap(a,b);}
        std::array<long long,4> key{ka[0],ka[1],kb[0],kb[1]};auto it=edges.find(key);
        if(it==edges.end())edges.emplace(key,BorderEdge{a,b,int(i),1});
        else if(it->second.owner==int(i))++it->second.count;
      }
    }
    for(const auto& entry:edges)if(entry.second.count==1){emit(borders,entry.second.a,entry.second.owner);emit(borders,entry.second.b,entry.second.owner);}
    for(int lat=-90;lat<90;lat+=3)for(int lon=-180;lon<180;lon+=3){
      Point a{
        double(lon),double(lat)
      },b{
        double(lon+3),double(lat)
      },c{
        double(lon+3),double(lat+3)
      },d{
        double(lon),double(lat+3)
      };
      for(auto p:{a,b,c,a,c,d})emit(ocean,p,255);
    }
    for(int lat=-60;lat<=60;lat+=30)for(int lon=-180;lon<180;lon+=3){
      emit(grid,{double(lon),double(lat)},254);
      emit(grid,{double(lon+3),double(lat)},254);
    }
    for(int lon=-180;lon<180;lon+=30)for(int lat=-90;lat<90;lat+=3){
      emit(grid,{double(lon),double(lat)},254);
      emit(grid,{double(lon),double(lat+3)},254);
    }
    landVertices=land.size();
    land_=upload(land);
    borders_=upload(borders);
    ocean_=upload(ocean);
    grid_=upload(grid);
  }
  void Renderer::campaign(const World& world,const Campaign& state){
    destroy(regions_);
    destroy(regionBorders_);
    shapes.clear();
    std::vector<Vertex> v,b;
    for(int nation:{state.player,state.enemy}){
      auto r=world.subdivide(nation);
      shapes.insert(shapes.end(),r.begin(),r.end());
    }
    for(size_t i=0;i<shapes.size();++i)for(auto& patch:shapes[i].patches){
      auto emit=[&](std::vector<Vertex>& out,Point p){
        out.push_back({float(p.x),float(p.y),float(192+i)});
      };
      for(size_t j=1;j+1<patch.size();++j)for(auto p:{patch[0],patch[j],patch[j+1]})emit(v,p);
      for(size_t j=0;j<patch.size();++j){
        auto p=patch[j],q=patch[(j+1)%patch.size()];
        for(int local:shapes[i].neighbours){
          size_t other=(i<24?0:24)+size_t(local);
          if(other<=i)continue;
          Point a{
            shapes[other].center.x-shapes[i].center.x,shapes[other].center.y-shapes[i].center.y
          };
          double edge=(shapes[other].center.x*shapes[other].center.x+shapes[other].center.y*shapes[other].center.y-shapes[i].center.x*shapes[i].center.x-shapes[i].center.y*shapes[i].center.y)/2;
          if(std::abs(p.x*a.x+p.y*a.y-edge)<1e-5&&std::abs(q.x*a.x+q.y*a.y-edge)<1e-5){
            emit(b,p);
            emit(b,q);
            break;
          }
        }
      }
    }
    regions_=upload(v);
    regionBorders_=upload(b);
  }
  void Renderer::draw(const Mesh& mesh,gl::Enum primitive,const Camera& c,float layer,bool borders){
    if(!mesh.count)return;
    gl::Uniform1f(uniforms_.layer,layer);
    gl::Uniform1i(uniforms_.border,borders);
    gl::BindVertexArray(mesh.vao);
    for(int offset=c.globe?0:-1;offset<=(c.globe?0:1);++offset){
      gl::Uniform1f(uniforms_.offset,float(offset*360));
      gl::DrawArrays(primitive,0,mesh.count);
    }
  }
  void Renderer::render(const World& world,const Campaign& s,const Camera& c,int selectedNation,int selectedRegion,int fw,int fh,float sx,float sy){
    gl::Viewport(0,0,fw,fh);
    gl::ClearColor(0.025f,0.04f,0.06f,1);
    gl::Clear(gl::COLOR_BUFFER_BIT|gl::DEPTH_BUFFER_BIT);
    gl::Viewport(int(c.x*sx),fh-int((c.y+c.height)*sy),int(c.width*sx),int(c.height*sy));
    std::array<std::array<float,4>,256> colors{};
    for(size_t i=0;i<world.countries.size();++i){
      float h=float((i*37)%17)/17;
      colors[i]={0.23f+h*0.045f,0.31f+h*0.055f,0.29f+h*0.035f,0.075f};
      if(world.canonical(int(i))==selectedNation)colors[i]={0.22f,0.58f,0.57f,0.35f};
      if(world.canonical(int(i))==s.player)colors[i]={0.13f,0.48f,0.42f,0.24f};
      if(world.canonical(int(i))==s.enemy)colors[i]={0.56f,0.23f,0.20f,0.24f};
    }
    for(size_t i=0;i<s.regions.size();++i){
      float h=float(i%4)*0.025f;
      auto& r=s.regions[i];
      colors[192+i]=r.owner==s.player?std::array<float,4>{0.13f+h,0.40f+h,0.36f+h,0.26f}:std::array<float,4>{0.46f+h,0.22f+h,0.20f+h,0.26f};
      // Movement targets are orders, not an abstract offensive region.

      if(int(i)==selectedRegion)colors[192+i]={0.34f,0.67f,0.63f,0.42f};
    }
    colors[254]={0.065f,0.14f,0.18f,0.6f};
    colors[255]={0.035f,0.09f,0.13f,1};
    gl::ActiveTexture(gl::TEXTURE0);
    gl::BindTexture(gl::TEXTURE_2D,palette_);
    if(!paletteReady_||colors!=paletteColors_){
      gl::TexSubImage2D(gl::TEXTURE_2D,0,0,0,16,16,gl::RGBA,gl::FLOAT,colors.data());
      paletteColors_=colors;paletteReady_=true;
    }
    gl::UseProgram(program_);
    gl::ActiveTexture(gl::TEXTURE1);gl::BindTexture(gl::TEXTURE_2D,assets.terrain);gl::Uniform1i(uniforms_.heightmap,1);gl::Uniform1i(uniforms_.relief,c.terrain);gl::ActiveTexture(gl::TEXTURE0);
    gl::Uniform1i(uniforms_.palette,0);
    gl::Uniform2f(uniforms_.center,float(c.longitude),float(c.latitude));
    gl::Uniform1f(uniforms_.zoom,float(c.zoom));
    gl::Uniform1f(uniforms_.aspect,c.width/c.height);
    gl::Uniform1i(uniforms_.globe,c.globe);
    gl::Enable(gl::BLEND);
    gl::BlendFunc(gl::SRC_ALPHA,gl::ONE_MINUS_SRC_ALPHA);
    gl::Uniform1i(uniforms_.sky,0);
    if(c.globe){
      gl::Disable(gl::DEPTH_TEST);
      gl::Uniform2f(uniforms_.viewport,c.width,c.height);
      gl::Uniform1i(uniforms_.sky,1);
      gl::BindVertexArray(sky_.vao);gl::DrawArrays(gl::TRIANGLES,0,sky_.count);
      gl::Uniform1i(uniforms_.sky,0);
      gl::Enable(gl::DEPTH_TEST);
      gl::DepthFunc(gl::LEQUAL);
    }
    else gl::Disable(gl::DEPTH_TEST);
    draw(ocean_,gl::TRIANGLES,c,1);
    draw(grid_,gl::LINES,c,1.001f);
    draw(land_,gl::TRIANGLES,c,1.003f);
    if(s.player>=0){
      draw(regions_,gl::TRIANGLES,c,1.005f);
      if(c.borders)draw(regionBorders_,gl::LINES,c,1.006f,true);
    }
    if(c.borders)draw(borders_,gl::LINES,c,1.007f,true);
    gl::Disable(gl::DEPTH_TEST);
    gl::BindVertexArray(0);
    gl::UseProgram(0);
    gl::Viewport(0,0,fw,fh);
  }
  void Renderer::shutdown(){
    assets.shutdown();
    for(auto* m:{&land_,&borders_,&ocean_,&grid_,&regions_,&regionBorders_,&sky_})destroy(*m);
    if(palette_)gl::DeleteTextures(1,&palette_);
    if(program_)gl::DeleteProgram(program_);
    palette_=program_=0;paletteReady_=false;
  }
}
