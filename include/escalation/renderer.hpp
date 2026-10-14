#pragma once
#include "simulation.hpp"
#include "gl.hpp"
#include "assets.hpp"
#include <optional>
#include <algorithm>
#include <cmath>
namespace escalation {
  struct Camera {
    bool globe=true,terrain=true,borders=true;
    double longitude=15,latitude=22,zoom=0.88;
    float x=0,y=60,width=900,height=600;
    Point project(Point p,bool* visible=nullptr) const;
    std::optional<Point> unproject(Point p) const;
  };
  // A map gesture owns its pointer until release, including travel over docks.
  // Remember the furthest travel so returning to the start cannot become a click.
  struct MapGesture {
    Point origin{};
    double furthest=0;
    bool active=false;
    void begin(Point p){origin=p;furthest=0;active=true;}
    void observe(Point p){if(active)furthest=std::max(furthest,std::hypot(p.x-origin.x,p.y-origin.y));}
    bool dragged(double threshold) const{return active&&furthest>threshold;}
    bool click(double threshold) const{return active&&!dragged(threshold);}
    void cancel(){active=false;}
  };
  // Exponential easing consumes motion in geographic space and preserves the
  // geographic point below the wheel cursor throughout each zoom animation.
  struct CameraMotion {
    Point pending{},zoomAnchor{};
    double desiredZoom=0;
    bool zooming=false;
    void reset(){ pending={}; zooming=false; }
    void pan(Point delta){ pending.x+=delta.x; pending.y+=delta.y; }
    void directPan(Camera& c,Point delta){
      c.longitude=std::remainder(c.longitude+delta.x,360.0);c.latitude=std::clamp(c.latitude+delta.y,-89.0,89.0);
    }
    void drag(Camera& c,Point delta){
      directPan(c,{delta.x*.88,delta.y*.88});pan({delta.x*.12,delta.y*.12});
    }
    void wheel(const Camera& c,Point anchor,double steps){
      desiredZoom=std::clamp((zooming?desiredZoom:c.zoom)*std::exp(steps*0.16),c.globe?0.55:0.8,512.0);
      zoomAnchor=anchor; zooming=true;
    }
    void advance(Camera& c,double dt){
      dt=std::clamp(dt,0.0,0.1);
      double ease=1-std::exp(-28*dt);
      c.longitude=std::remainder(c.longitude+pending.x*ease,360.0);
      c.latitude=std::clamp(c.latitude+pending.y*ease,-89.0,89.0);
      pending.x*=1-ease; pending.y*=1-ease;
      if(zooming){
        auto before=c.unproject(zoomAnchor);
        c.zoom=std::exp(std::log(c.zoom)+(std::log(desiredZoom)-std::log(c.zoom))*ease);
        auto after=c.unproject(zoomAnchor);
        if(before&&after&&!c.globe){
          c.longitude=std::remainder(c.longitude+std::remainder(before->x-after->x,360.0),360.0);
          c.latitude=std::clamp(c.latitude+before->y-after->y,-89.0,89.0);
        }
        else if(before&&after){
          // Solve the orthographic tilt and longitude so the cursor's original
          // geographic point remains fixed, including near the dateline.
          constexpr double rad=0.017453292519943295;
          double aspect=c.width/c.height;
          double sx=((zoomAnchor.x-c.x)*2/c.width-1)*aspect/c.zoom;
          double sy=(1-(zoomAnchor.y-c.y)*2/c.height)/c.zoom;
          double sz=std::sqrt(std::max(0.0,1-sx*sx-sy*sy));
          double radius=std::hypot(sy,sz),target=std::sin(before->y*rad);
          if(radius>0&&std::abs(target)<=radius){
            double angle=std::asin(std::clamp(target/radius,-1.0,1.0)),offset=std::atan2(sy,sz);
            double tilt=std::remainder(angle-offset,2*3.141592653589793)/rad;
            double alternate=std::remainder(3.141592653589793-angle-offset,2*3.141592653589793)/rad;
            if(std::abs(alternate)<=89&&std::abs(alternate-c.latitude)<std::abs(tilt-c.latitude))tilt=alternate;
            if(std::abs(tilt)<=89){
              c.latitude=tilt;
              double pz=-sy*std::sin(tilt*rad)+sz*std::cos(tilt*rad);
              c.longitude=std::remainder(before->x-std::atan2(sx,pz)/rad,360.0);
            }
          }
        }
        if(std::abs(std::log(desiredZoom/c.zoom))<1e-6)zooming=false;
      }
    }
  };
  class Renderer {
    struct Vertex {
      float x,y,id;
    };
    struct Mesh{
      gl::UInt vao=0,vbo=0;
      int count=0;
    };
    gl::UInt program_=0,palette_=0;
    Mesh land_,borders_,ocean_,grid_,regions_,regionBorders_,sky_;
    std::array<std::array<float,4>,256> paletteColors_{};
    bool paletteReady_=false;
    struct Uniforms { int layer,border,offset,heightmap,relief,palette,center,zoom,aspect,globe,sky,viewport; } uniforms_{};
    Mesh upload(const std::vector<Vertex>& vertices);
    void destroy(Mesh& mesh);
    void draw(const Mesh& mesh,gl::Enum primitive,const Camera& c,float layer,bool borders=false);
    public: std::vector<RegionShape> shapes;
    std::string version;
    size_t landVertices=0;
    SurfaceAssets assets;
    void initialise(const World& world,const std::string& data="data");
    void campaign(const World& world,const Campaign& state);
    void render(const World& world,const Campaign& state,const Camera& camera,int selectedNation,int selectedRegion,int framebufferWidth,int framebufferHeight,float scaleX,float scaleY);
    void shutdown();
  };
}
