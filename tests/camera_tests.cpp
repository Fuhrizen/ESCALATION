#include "escalation/renderer.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace escalation;
int main(){
  try{
    Camera c;
    c.width=1100;
    c.height=700;
    c.x=12;
    c.y=60;
    c.zoom=1;
    int checks=0;
    for(double zoom:{1.0,8.0,64.0,512.0})for(bool globe:{false,true})for(double longitude:{-179.0,0.0,179.0})for(double latitude:{-65.0,0.0,65.0}){
      c.globe=globe;
      c.zoom=zoom;
      c.longitude=longitude;
      c.latitude=latitude;
      for(int dx=-60;dx<=60;dx+=20)for(int dy=-20;dy<=20;dy+=10){
        Point point{
          std::remainder(longitude+dx/zoom,360.0),std::clamp(latitude+dy/zoom,-85.0,85.0)
        };
        bool visible;
        auto screen=c.project(point,&visible);
        if(!visible)continue;
        auto result=c.unproject(screen);
        if(!result||std::abs(std::remainder(result->x-point.x,360.0))>1e-6||std::abs(result->y-point.y)>1e-6)throw std::runtime_error("Projection/picking mismatch");
        ++checks;
      }
    }
    c.globe=true;
    c.zoom=1;
    auto outside=c.unproject({c.x,c.y});
    if(outside)throw std::runtime_error("Outside globe must not select");
    std::cout<<"Passed "<<checks+1<<" projection checks\n";
    return 0;
  }
  catch(const std::exception& e){
    std::cerr<<e.what()<<"\n";
    return 1;
  }
}
