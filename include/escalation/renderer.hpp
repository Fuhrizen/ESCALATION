#pragma once
#include "simulation.hpp"
#include "gl.hpp"
#include <optional>
namespace escalation {
  struct Camera {
    bool globe=true;
    double longitude=15,latitude=22,zoom=0.88;
    float x=0,y=60,width=900,height=600;
    Point project(Point p,bool* visible=nullptr) const;
    std::optional<Point> unproject(Point p) const;
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
    Mesh land_,borders_,ocean_,grid_,regions_,regionBorders_;
    Mesh upload(const std::vector<Vertex>& vertices);
    void destroy(Mesh& mesh);
    void draw(const Mesh& mesh,gl::Enum primitive,const Camera& c,float layer,bool borders=false);
    public: std::vector<RegionShape> shapes;
    std::string version;
    size_t landVertices=0;
    void initialise(const World& world);
    void campaign(const World& world,const Campaign& state);
    void render(const World& world,const Campaign& state,const Camera& camera,int selectedNation,int selectedRegion,int framebufferWidth,int framebufferHeight,float scaleX,float scaleY);
    void shutdown();
  };
}
