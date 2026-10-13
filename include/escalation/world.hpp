#pragma once
#include <array>
#include <string>
#include <vector>
namespace escalation {
  struct Point {
    double x=0,y=0;
  };
  using Ring=std::vector<Point>;
  struct Polygon {
    std::vector<Ring> rings;
  };
  struct Country {
    std::string code,name,history;
    int group=-1;
    double population=0,gdp=0;
    Point label,center;
    std::vector<Polygon> polygons;
    std::vector<std::array<Point,3>> triangles;
    std::vector<int> neighbours;
    bool contains(Point p) const;
  };
  struct RegionShape {
    Point center;
    std::vector<Ring> patches;
    std::vector<int> neighbours;
  };
  struct WorldCity {
    std::string name;
    Point position;
    double population=0;
    std::string countryCode;
  };
  struct WorldRoad {
    std::vector<Point> points;
  };
  class World {
    public: std::vector<Country> countries;
    std::vector<WorldCity> cities;
    std::vector<WorldRoad> roads;
    void load(const std::string& file,const std::string& histories="");
    // Conservative candidates for a local geographic radius (degrees). The
    // longitude extent uses cos(latitude), clamped to 0.2 near the poles.
    std::vector<int> nearbyRoads(Point center,double radius) const;
    void rebuildRoadIndex();
    int pick(Point p) const;
    int canonical(int nation) const;
    void applyGroups(const std::string& file);
    int find(const std::string& code) const;
    int opponent(int nation) const;
    std::vector<RegionShape> subdivide(int nation,int count=24) const;
    private:
    std::array<std::vector<int>,72*36> roadIndex_;
    std::size_t indexedRoadCount_=0;
  };
  double distance(Point a,Point b);
}
