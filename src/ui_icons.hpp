#pragma once
#include <cmath>
#include <string>
// Original, shared geometric command icons. No glyph-font or external artwork.
namespace command_icons {
inline void line(ImDrawList* d,ImVec2 c,float s,ImVec2 a,ImVec2 b,ImU32 color,float w=1.7f){d->AddLine({c.x+a.x*s,c.y+a.y*s},{c.x+b.x*s,c.y+b.y*s},color,w);}
inline void icon(ImDrawList* d,ImVec2 c,float size,int kind,ImU32 color){
 float s=size*.5f;auto l=[&](float ax,float ay,float bx,float by){line(d,c,s,{ax,ay},{bx,by},color);};
 auto circle=[&](float x,float y,float r){d->AddCircle({c.x+x*s,c.y+y*s},r*s,color,16,1.7f);};
 // Numeric IDs are stable for saved games; aircraft remain retired.
 if((kind>=3&&kind<=4)||kind==31)return;
 // 0..10 units, 20+facility enum indices.
 if(kind==0){l(-.65,-.45,.65,.45);l(.65,-.45,-.65,.45);}
 else if(kind==1){d->AddEllipse(c,{.72f*s,.38f*s},color,0,24,1.7f);}
 else if(kind==2){d->AddCircleFilled(c,.2f*s,color,16);}
 else if(kind==5||kind==9){l(-.85,.1,-.6,.55);l(-.6,.55,.6,.55);l(.6,.55,.85,.1);l(-.85,.1,.85,.1);l(-.35,.1,-.35,-.4);l(-.35,-.4,.4,-.4);l(.4,-.4,.4,.1);l(0,-.4,0,-.85);l(-.65,.8,-.2,.8);l(.15,.8,.65,.8);if(kind==9){l(-.65,-.15,-.65,-.7);l(.65,-.15,.65,-.7);l(-.75,-.5,-.65,-.7);l(.55,-.5,.65,-.7);}}
 else if(kind==6||kind==10){d->AddEllipse(c,{.85f*s,.27f*s},color,0,24,1.7f);l(-.1,-.2,-.1,-.55);l(-.1,-.55,.3,-.55);l(.3,-.55,.3,-.2);l(.1,-.55,.1,-.8);if(kind==10){l(-.6,-.2,-.6,-.65);l(.6,-.2,.6,-.65);}}
 else if(kind==28){l(-.8,.7,.8,.7);l(-.6,.35,.65,.35);l(-.6,.35,-.85,-.1);l(-.85,-.1,.85,-.1);l(.85,-.1,.65,.35);l(0,-.1,0,-.8);l(0,-.8,.7,-.8);l(.7,-.8,.7,-.3);l(-.5,-.1,-.5,-.55);}
 else if(kind==7){l(-.6,-.4,.6,.4);l(.6,-.4,-.6,.4);l(0,-.7,0,.7);}
 else if(kind==8||kind==29){l(-.25,.45,-.25,-.35);l(-.25,-.35,0,-.8);l(0,-.8,.25,-.35);l(.25,-.35,.25,.45);l(-.25,.45,.25,.45);l(-.25,.1,-.55,.65);l(.25,.1,.55,.65);l(-.15,.6,0,.85);l(.15,.6,0,.85);}
 else if(kind==20){l(-.75,.6,.1,-.5);l(-.4,-.7,.7,.4);l(-.75,-.35,-.35,-.75);l(.35,.75,.75,.35);}
 else if(kind==21){l(-.65,.7,0,-.8);l(0,-.8,.65,.7);l(-.65,.7,.65,.7);l(-.4,.15,.4,.15);l(-.22,-.3,.22,-.3);l(0,-.8,.7,-.8);l(.7,-.8,.7,-.35);}
 else if(kind==22){l(-.8,.7,-.8,-.1);l(-.8,-.1,.1,-.1);l(.1,-.1,.1,.7);l(-.8,.7,.8,.7);l(.5,.7,.5,-.7);l(.75,.7,.75,-.7);l(-.55,.25,-.15,.25);}
 else if(kind==23){l(-.7,.7,-.7,-.45);l(-.7,-.45,-.25,-.45);l(-.25,-.45,-.25,.7);l(.2,.7,.2,-.8);l(.65,.7,.65,-.8);l(.2,-.8,.65,-.8);l(-.8,.7,.8,.7);l(-.2,.1,.2,.1);}
 else if(kind==25){l(.1,-.85,-.55,.1);l(-.55,.1,.15,.1);l(.15,.1,-.1,.85);l(-.1,.85,.6,-.25);l(.6,-.25,-.05,-.25);}
 else if(kind==33){l(-.8,.7,.8,.7);l(-.8,.35,.8,.35);l(-.8,0,.8,0);l(0,0,0,-.8);l(0,-.45,-.4,-.7);l(0,-.2,.4,-.5);}
 else if(kind==35){l(-.75,.75,-.75,-.3);l(-.75,-.3,-.15,-.3);l(-.15,-.3,-.15,.75);l(.1,.75,.1,-.75);l(.1,-.75,.75,-.75);l(.75,-.75,.75,.75);l(-.75,.75,.75,.75);l(.35,-.4,.5,-.4);l(.35,0,.5,0);l(.35,.4,.5,.4);}
 else if(kind==24||kind==34){l(-.8,.75,-.8,-.1);l(-.8,-.1,-.3,-.4);l(-.3,-.4,-.3,-.05);l(-.3,-.05,.2,-.4);l(.2,-.4,.2,.1);l(.2,.1,.8,.1);l(.8,.1,.8,.75);l(-.8,.75,.8,.75);l(.5,.1,.5,-.8);l(.75,.1,.75,-.8);}
 else if(kind==26){l(-.3,-.8,.3,-.8);l(-.2,-.8,-.2,-.15);l(.2,-.8,.2,-.15);l(-.2,-.15,-.7,.7);l(.2,-.15,.7,.7);l(-.7,.7,.7,.7);l(-.5,.35,.5,.35);}
 else if(kind==27){l(-.7,.65,-.7,-.2);l(-.7,-.2,.7,-.2);l(.7,-.2,.7,.65);l(-.7,.65,.7,.65);l(0,-.2,0,-.85);l(0,-.85,.7,-.6);l(.7,-.6,0,-.4);l(-.3,.15,.3,.15);}
 else if(kind==30){l(-.8,.7,-.8,-.4);l(-.8,-.4,0,-.8);l(0,-.8,.8,-.4);l(.8,-.4,.8,.7);l(-.8,.7,.8,.7);l(-.45,-.1,.45,-.1);l(-.45,.2,.45,.2);}
 else if(kind==32){circle(0,0,.8);l(0,0,.55,-.55);l(-.55,.55,.55,-.55);circle(0,0,.15);}
 else {l(-.7,-.7,.7,.7);l(.7,-.7,-.7,.7);circle(0,0,.85);}
}
inline void weapon(ImDrawList* d,ImVec2 c,float size,const std::string& id,ImU32 color){
 float s=size*.5f;auto l=[&](float ax,float ay,float bx,float by){line(d,c,s,{ax,ay},{bx,by},color,2.f);};
 if(id=="rifle"||id=="carbine"||id=="coax"||id=="machine_gun"){
   l(-.85,.25,.9,-.15);l(-.85,.25,-.7,.55);l(-.7,.55,-.35,.2);l(-.4,.15,-.25,.6);l(-.25,.6,-.05,.55);l(-.05,.55,-.15,.08);l(-.25,-.02,.3,-.15);l(.3,-.15,.38,-.35);l(.5,-.14,.5,-.35);
 }else if(id=="torpedo"||id=="asw_torpedo"||id=="sub_torpedo"||id=="depth_charge"){
   d->AddEllipse(c,{.85f*s,.21f*s},color,0,24,2.f);l(-.68,-.14,-.93,-.4);l(-.68,.14,-.93,.4);l(.68,-.15,.68,.15);
 }else if(id=="howitzer"||id=="tank_gun"||id=="naval_gun"){
   l(-.65,.18,.3,-.25);l(-.5,.4,.46,-.03);l(.3,-.25,.9,-.4);l(.46,-.03,.97,-.3);d->AddCircle({c.x-.25f*s,c.y+.4f*s},.27f*s,color,16,2.f);l(-.1,.4,.75,.8);
 }else icon(d,c,size,8,color);
}
inline void badge(ImDrawList* d,ImVec2 c,float size,int kind,ImU32 color,bool selected=false,bool hostile=false){
 if((kind>=3&&kind<=4)||kind==31)return;
 const ImU32 edge=selected?IM_COL32(220,240,255,255):color;
 if(kind<20){
   float x=size*.82f,y=size*.56f;
   if(selected&&!hostile)d->AddRect({c.x-x-3,c.y-y-3},{c.x+x+3,c.y+y+3},IM_COL32(102,183,255,170),2,0,1.0f);
   if(hostile){ImVec2 shape[]={{c.x,c.y-y-3},{c.x+x+3,c.y},{c.x,c.y+y+3},{c.x-x-3,c.y}};d->AddConvexPolyFilled(shape,4,color);d->AddPolyline(shape,4,selected?edge:IM_COL32(15,32,51,255),ImDrawFlags_Closed,selected?2.0f:1.4f);icon(d,c,size*.65f,kind,IM_COL32(14,32,52,255));}
   else {d->AddRectFilled({c.x-x,c.y-y},{c.x+x,c.y+y},color,1);d->AddRect({c.x-x,c.y-y},{c.x+x,c.y+y},selected?edge:IM_COL32(15,32,51,255),1,0,selected?2.0f:1.4f);if(kind==0){
     // NATO infantry diagonals join the frame corners, rather than a small X.
     float inset=1.3f;ImU32 symbol=IM_COL32(10,29,49,255);
     d->AddLine({c.x-x+inset,c.y-y+inset},{c.x+x-inset,c.y+y-inset},symbol,1.7f);
     d->AddLine({c.x+x-inset,c.y-y+inset},{c.x-x+inset,c.y+y-inset},symbol,1.7f);
   }else icon(d,c,size*.85f,kind,IM_COL32(10,29,49,255));}
 } else {
   float r=size*.7f;
   d->AddRectFilled({c.x-r,c.y-r},{c.x+r,c.y+r},IM_COL32(10,20,30,242),3);
   d->AddRect({c.x-r,c.y-r},{c.x+r,c.y+r},edge,3,0,selected?2.2f:1.0f);
   // Installation tab makes infrastructure distinguishable from formations.
   d->AddRectFilled({c.x-size*.22f,c.y-r-3},{c.x+size*.22f,c.y-r+1},edge,1);
   icon(d,c,size,kind,color);
 }
}
}
