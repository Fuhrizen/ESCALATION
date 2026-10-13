#include "escalation/world.hpp"
#include "escalation/json.hpp"
#include <earcut.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <random>
#include <limits>
#include <filesystem>
namespace mapbox {
  namespace util {
    template<> struct nth<0,escalation::Point>{
      static double get(const escalation::Point& p){
        return p.x;
      }
    };
    template<> struct nth<1,escalation::Point>{
      static double get(const escalation::Point& p){
        return p.y;
      }
    };
  }
}
namespace escalation {
  double distance(Point a,Point b){
    double dx=std::remainder(a.x-b.x,360.0)*std::cos((a.y+b.y)*0.00872664626),dy=a.y-b.y;
    return std::hypot(dx,dy);
  }
  static bool inRing(const Ring& r,Point p){
    bool inside=false;
    for(size_t i=0,j=r.size()-1;i<r.size();j=i++){
      auto a=r[i],b=r[j];
      if((a.y>p.y)!=(b.y>p.y)&&p.x<(b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x)inside=!inside;
    }
    return inside;
  }
  bool Country::contains(Point p) const {
    for(auto& poly:polygons){
      Point q=p;
      double low=1e30,high=-1e30;
      for(auto v:poly.rings[0]){
        low=std::min(low,v.x);
        high=std::max(high,v.x);
      }
      double reference=(low+high)/2;
      while(q.x-reference>180)q.x-=360;
      while(q.x-reference< -180)q.x+=360;
      if(!inRing(poly.rings[0],q))continue;
      bool hole=false;
      for(size_t i=1;i<poly.rings.size();++i)if(inRing(poly.rings[i],q))hole=true;
      if(!hole)return true;
    }
    return false;
  }
  int World::find(const std::string& code) const {
    for(size_t i=0;i<countries.size();++i)if(countries[i].code==code)return int(i);
    return -1;
  }
  int World::pick(Point p) const{
    for(size_t i=0;i<countries.size();++i)if(countries[i].contains(p))return canonical(int(i));
    return -1;
  }
  void World::load(const std::string& file,const std::string& histories){
    auto root=readJson(file);
    countries.clear();
    for(auto& feature:field(root,"features").get<Array>()){
      Country c;
      auto& p=field(feature,"properties");
      c.code=string(p,"code");
      c.name=string(p,"name");
      c.population=number(p,"population");
      c.gdp=number(p,"gdp");
      c.label={number(p,"label_lon"),number(p,"label_lat")};
      auto& geom=field(feature,"geometry");
      Array polys;
      auto coords=field(geom,"coordinates").get<Array>();
      if(string(geom,"type")=="Polygon")polys.push_back(Json(coords));
      else polys=coords;
      double largest=0;
      for(auto& poly:polys){
        Polygon polygon;
        for(auto& ring:poly.get<Array>()){
          Ring points;
          double previous=0;
          for(auto& coord:ring.get<Array>()){
            auto& a=coord.get<Array>();
            Point pt{
              a[0].get<double>(),a[1].get<double>()
            };
            if(!points.empty()){
              while(pt.x-previous>180)pt.x-=360;
              while(pt.x-previous< -180)pt.x+=360;
            }
            previous=pt.x;
            points.push_back(pt);
          }
          if(points.size()>1&&points.front().x==points.back().x&&points.front().y==points.back().y)points.pop_back();
          if(points.size()>=3)polygon.rings.push_back(points);
        }
        if(polygon.rings.empty())continue;
        auto indices=mapbox::earcut<unsigned>(polygon.rings);
        std::vector<Point> flat;
        for(auto& r:polygon.rings)flat.insert(flat.end(),r.begin(),r.end());
        for(size_t t=0;t+2<indices.size();t+=3){
          std::array<Point,3> tri{
            flat[indices[t]],flat[indices[t+1]],flat[indices[t+2]]
          };
          c.triangles.push_back(tri);
          double area=std::abs((tri[1].x-tri[0].x)*(tri[2].y-tri[0].y)-(tri[2].x-tri[0].x)*(tri[1].y-tri[0].y));
          if(area>largest){
            largest=area;
            c.center={(tri[0].x+tri[1].x+tri[2].x)/3,(tri[0].y+tri[1].y+tri[2].y)/3};
          }
        }
        c.polygons.push_back(std::move(polygon));
      }
      if(!c.triangles.empty()){
        if(!c.contains(c.center)){
          if(c.contains(c.label))c.center=c.label;
          else for(auto t:c.triangles){
            Point seed{
              (t[0].x+t[1].x+t[2].x)/3,(t[0].y+t[1].y+t[2].y)/3
            };
            if(c.contains(seed)){
              c.center=seed;
              break;
            }
          }
        }
        countries.push_back(std::move(c));
      }
    }
    // Shared full edges, rather than bounding boxes or geographic proximity, establish land borders.
    using Vertex=std::pair<long long,long long>;
    using Edge=std::pair<Vertex,Vertex>;
    std::map<Edge,std::vector<int>> edges;
    auto vertex=[](Point p){
      return Vertex{std::llround(std::remainder(p.x,360.0)*10000),std::llround(p.y*10000)};
    };
    for(size_t i=0;i<countries.size();++i)for(auto& p:countries[i].polygons)for(auto& r:p.rings)for(size_t k=0;k<r.size();++k){
      auto a=vertex(r[k]),b=vertex(r[(k+1)%r.size()]);
      if(b<a)std::swap(a,b);
      edges[{
        a,b
      }
      ].push_back(int(i));
    }
    for(auto& [edge,owners]:edges)for(int a:owners)for(int b:owners)if(a!=b)countries[a].neighbours.push_back(b);
    for(auto& c:countries){
      std::sort(c.neighbours.begin(),c.neighbours.end());
      c.neighbours.erase(std::unique(c.neighbours.begin(),c.neighbours.end()),c.neighbours.end());
      c.history="A modern-boundary alternate-history nation. Start in 1920 and guide its industrial and strategic development; population and GDP are modern Natural Earth reference values, not 1920 statistics.";
    }
    if(!histories.empty()){
      auto h=readJson(histories);
      for(auto& c:countries)if(h.contains(c.code))c.history=h.get(c.code).get<std::string>();
    }
    auto grouping=std::filesystem::path(file).parent_path()/"nation-groups.json";
    if(std::filesystem::exists(grouping))applyGroups(grouping.string());
  }
  int World::opponent(int nation) const{
    nation=canonical(nation);
    auto& c=countries.at(nation);
    int best=-1;
    double d=std::numeric_limits<double>::max();
    auto consider=[&](int i){
      i=canonical(i);
      if(i==nation||countries[i].code=="ATA")return;
      double n=distance(c.center,countries[i].center);
      if(n<d){
        best=i;
        d=n;
      }
    };
    for(int i:c.neighbours)consider(i);
    if(best<0)for(size_t i=0;i<countries.size();++i)consider(int(i));
    return best;
  }
  static Ring clip(const Ring& in,Point a,double b){
    Ring out;
    if(in.empty())return out;
    auto side=[&](Point p){
      return p.x*a.x+p.y*a.y-b;
    };
    for(size_t i=0;i<in.size();++i){
      Point p=in[i],q=in[(i+1)%in.size()];
      double sp=side(p),sq=side(q);
      if(sp<=1e-9)out.push_back(p);
      if((sp<0)!=(sq<0)){
        double t=sp/(sp-sq);
        out.push_back({p.x+(q.x-p.x)*t,p.y+(q.y-p.y)*t});
      }
    }
    return out;
  }
  std::vector<RegionShape> World::subdivide(int nation,int count) const{
    auto& country=countries.at(nation);
    std::vector<RegionShape> regions;
    std::mt19937 rng(0xE5CA+unsigned(nation));
    // Area-weighted samples keep every site's candidate on actual land, including islands and holes.
    std::vector<double> weights;
    for(auto& t:country.triangles)weights.push_back(std::abs((t[1].x-t[0].x)*(t[2].y-t[0].y)-(t[2].x-t[0].x)*(t[1].y-t[0].y)));
    std::discrete_distribution<size_t> pick(weights.begin(),weights.end());
    std::uniform_real_distribution<double> uniform(0,1);
    std::vector<Point> candidates{
      country.center
    };
    for(int n=0;n<600;++n){
      auto& t=country.triangles[pick(rng)];
      double u=uniform(rng),v=uniform(rng);
      if(u+v>1){
        u=1-u;
        v=1-v;
      }
      Point p{
        t[0].x+(t[1].x-t[0].x)*u+(t[2].x-t[0].x)*v,t[0].y+(t[1].y-t[0].y)*u+(t[2].y-t[0].y)*v
      };
      while(p.x-country.center.x>180)p.x-=360;
      while(p.x-country.center.x< -180)p.x+=360;
      candidates.push_back(p);
    }
    regions.push_back({candidates[0],{}, {}});
    for(int n=1;n<count;++n){
      double best=-1;
      Point selected;
      for(auto p:candidates){
        double nearest=1e30;
        for(auto& r:regions)nearest=std::min(nearest,distance(p,r.center));
        if(nearest>best){
          best=nearest;
          selected=p;
        }
      }
      regions.push_back({selected,{}, {}});
    }
    // Clip original triangulated land against Voronoi halfplanes: no invented land or lake filling.
    for(int i=0;i<count;++i)for(auto tri:country.triangles){
      for(auto& p:tri){
        while(p.x-country.center.x>180)p.x-=360;
        while(p.x-country.center.x< -180)p.x+=360;
      }
      Ring patch{
        tri[0],tri[1],tri[2]
      };
      for(int j=0;j<count&&!patch.empty();++j)if(i!=j){
        Point a{
          regions[j].center.x-regions[i].center.x,regions[j].center.y-regions[i].center.y
        };
        double b=(regions[j].center.x*regions[j].center.x+regions[j].center.y*regions[j].center.y-regions[i].center.x*regions[i].center.x-regions[i].center.y*regions[i].center.y)/2;
        patch=clip(patch,a,b);
      }
      if(patch.size()<3)continue;
      regions[i].patches.push_back(patch);
      for(int j=0;j<count;++j)if(i!=j){
        Point a{
          regions[j].center.x-regions[i].center.x,regions[j].center.y-regions[i].center.y
        };
        double b=(regions[j].center.x*regions[j].center.x+regions[j].center.y*regions[j].center.y-regions[i].center.x*regions[i].center.x-regions[i].center.y*regions[i].center.y)/2;
        for(size_t k=0;k<patch.size();++k){
          auto p=patch[k],q=patch[(k+1)%patch.size()];
          if(std::abs(p.x*a.x+p.y*a.y-b)<1e-6&&std::abs(q.x*a.x+q.y*a.y-b)<1e-6&&std::hypot(p.x-q.x,p.y-q.y)>1e-6)regions[i].neighbours.push_back(j);
        }
      }
    }
    for(auto& r:regions){
      std::sort(r.neighbours.begin(),r.neighbours.end());
      r.neighbours.erase(std::unique(r.neighbours.begin(),r.neighbours.end()),r.neighbours.end());
    }
    return regions;
  }
  int World::canonical(int nation) const {
    if(nation<0||nation>=int(countries.size()))return -1;
    for(int depth=0;depth<8&&countries[nation].group>=0;++depth)nation=countries[nation].group;
    return nation;
  }
  void World::applyGroups(const std::string& file){
    auto mapping=field(readJson(file),"groups").get<Object>();
    for(auto& [from,to]:mapping){int child=find(from),parent=find(to.get<std::string>());if(child<0||parent<0||child==parent)throw std::runtime_error("Invalid nation group");countries[child].group=parent;}
    for(size_t i=0;i<countries.size();++i){int parent=canonical(int(i));if(parent==int(i))continue;auto& source=countries[i];auto& target=countries[parent];target.polygons.insert(target.polygons.end(),source.polygons.begin(),source.polygons.end());target.triangles.insert(target.triangles.end(),source.triangles.begin(),source.triangles.end());source.triangles.clear();target.population+=source.population;target.gdp+=source.gdp;target.name+=" + "+source.name;target.neighbours.insert(target.neighbours.end(),source.neighbours.begin(),source.neighbours.end());}
    for(size_t i=0;i<countries.size();++i){auto& ns=countries[i].neighbours;std::vector<int> grouped;for(int n:ns){int root=canonical(n);if(root!=canonical(int(i)))grouped.push_back(root);}std::sort(grouped.begin(),grouped.end());grouped.erase(std::unique(grouped.begin(),grouped.end()),grouped.end());ns=std::move(grouped);}
  }

}
