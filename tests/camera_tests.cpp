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
    auto require=[&](bool value,const char* message){if(!value)throw std::runtime_error(message);++checks;};
    MapGesture gesture;
    gesture.begin({400,250});gesture.observe({402,251});
    require(gesture.click(5),"Ordinary map click must tolerate small pointer jitter");
    // The excursion can cross a dock and return before the release frame.
    gesture.observe({1000,250});gesture.observe({400,250});
    require(gesture.dragged(5)&&!gesture.click(5),"A pan returning to its start must not issue a right-click order");
    require(gesture.dragged(8)&&!gesture.click(8),"A selection drag returning to its start must not select or place a building");
    gesture.cancel();gesture.observe({401,250});
    require(!gesture.active&&!gesture.click(8),"A modal or focus loss must cancel release actions");
    gesture.begin({400,250});gesture.observe({401,250});
    require(gesture.click(5),"A fresh map press after cancellation must behave normally");
    Camera smooth=c;
    smooth.longitude=0;smooth.latitude=0;
    CameraMotion motion;
    motion.pan({20,10});
    motion.advance(smooth,1.0/60);
    require(smooth.longitude>0&&smooth.longitude<20,"Pan must ease instead of snapping");
    for(int frame=0;frame<120;++frame)motion.advance(smooth,1.0/60);
    require(std::abs(smooth.longitude-20)<1e-7&&std::abs(smooth.latitude-10)<1e-7,"Pan must converge to its target");
    Camera tracking=c;tracking.longitude=tracking.latitude=0;
    CameraMotion drag;drag.drag(tracking,{10,5});
    require(std::abs(tracking.longitude-8.8)<1e-9&&std::abs(tracking.latitude-4.4)<1e-9,"Drag must apply 88 percent immediately for direct cursor tracking");
    for(int frame=0;frame<90;++frame)drag.advance(tracking,1.0/60);
    require(std::abs(tracking.longitude-10)<1e-9&&std::abs(tracking.latitude-5)<1e-9,"Residual drag must preserve total motion without runaway inertia");
    drag.directPan(tracking,{5,0});
    require(std::abs(tracking.longitude-15)<1e-9,"Keyboard motion must apply immediately");
    Camera a=c,b=c;
    a.longitude=b.longitude=0;a.latitude=b.latitude=0;
    CameraMotion ma,mb;ma.pan({20,10});mb.pan({20,10});
    for(int frame=0;frame<60;++frame)ma.advance(a,1.0/60);
    for(int frame=0;frame<10;++frame)mb.advance(b,0.1);
    require(std::abs(a.longitude-b.longitude)<1e-9,"Pan easing must be independent of frame rate");
    for(bool globe:{false,true})for(double longitude:{-179.0,0.0,179.0})for(double latitude:{-55.0,0.0,55.0})for(double steps:{-2.0,2.0}){
      c.globe=globe;c.zoom=8;c.longitude=longitude;c.latitude=latitude;
      Point cursor{c.x+c.width*0.57,c.y+c.height*0.44};
      auto anchor=c.unproject(cursor);
      require(bool(anchor),"Zoom anchor must lie on the visible map");
      CameraMotion zoom;zoom.wheel(c,cursor,steps);
      for(int frame=0;frame<100;++frame){
        zoom.advance(c,1.0/60);
        auto current=c.unproject(cursor);
        require(current&&std::abs(std::remainder(current->x-anchor->x,360.0))<1e-7&&std::abs(current->y-anchor->y)<1e-7,"Wheel zoom must preserve its geographic cursor anchor");
      }
      require(std::abs(std::log(c.zoom/(8*std::exp(steps*0.16))))<1e-6,"Zoom must converge to the wheel target");
    }
    c.longitude=178;c.latitude=88;
    motion.reset();motion.pan({15,20});
    for(int frame=0;frame<60;++frame)motion.advance(c,1.0/60);
    require(c.longitude>=-180&&c.longitude<=180&&c.latitude<=89,"Panning must wrap longitude and clamp latitude");
    motion.pan({100,100});motion.wheel(c,{c.x+c.width/2,c.y+c.height/2},1);motion.reset();
    auto before=c;
    motion.advance(c,0.1);
    require(c.longitude==before.longitude&&c.latitude==before.latitude&&c.zoom==before.zoom,"Focus reset must stop pending camera motion");
    std::cout<<"Passed "<<checks+1<<" projection and camera motion checks\n";
    return 0;
  }
  catch(const std::exception& e){
    std::cerr<<e.what()<<"\n";
    return 1;
  }
}
