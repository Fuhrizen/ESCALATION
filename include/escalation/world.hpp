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
  class World {
    public: std::vector<Country> countries;
    void load(const std::string& file,const std::string& histories="");
    int pick(Point p) const;
    int find(const std::string& code) const;
    int opponent(int nation) const;
    std::vector<RegionShape> subdivide(int nation,int count=24) const;
  };
  double distance(Point a,Point b);
}
