#pragma once
// Original, shared geometric command icons. No glyph-font or external artwork.
namespace command_icons {
inline void line(ImDrawList* d,ImVec2 c,float s,ImVec2 a,ImVec2 b,ImU32 color,float w=1.7f){d->AddLine({c.x+a.x*s,c.y+a.y*s},{c.x+b.x*s,c.y+b.y*s},color,w);}
inline void icon(ImDrawList* d,ImVec2 c,float size,int kind,ImU32 color){
 float s=size*.5f;auto l=[&](float ax,float ay,float bx,float by){line(d,c,s,{ax,ay},{bx,by},color);};
 auto circle=[&](float x,float y,float r){d->AddCircle({c.x+x*s,c.y+y*s},r*s,color,16,1.7f);};
 // 0..8 units, 20+facility enum indices.
 if(kind==0){l(-.6,-.35,0,.3);l(0,.3,.6,-.35);l(-.6,.0,0,.65);l(0,.65,.6,0);}
 else if(kind==1){d->AddRect({c.x-.75f*s,c.y-.25f*s},{c.x+.75f*s,c.y+.4f*s},color,3,0,1.7f);l(-.35,-.25,.3,-.25);l(.15,-.35,.8,-.6);circle(-.45,.42,.12);circle(.45,.42,.12);}
 else if(kind==2){circle(-.25,.4,.22);l(-.7,.55,.6,.55);l(-.4,.2,.65,-.65);l(.25,-.35,.75,-.65);}
 else if(kind==3||kind==4){l(0,-.85,0,.75);l(0,-.25,-.85,.25);l(0,-.25,.85,.25);l(-.85,.25,.85,.25);l(-.35,.6,.35,.6);if(kind==4){l(-.6,.1,-.6,.55);l(.6,.1,.6,.55);}}
 else if(kind==5){l(-.85,.2,-.55,.65);l(-.55,.65,.6,.65);l(.6,.65,.85,.2);l(-.85,.2,.85,.2);l(-.45,.15,-.45,-.25);l(-.45,-.25,.4,-.25);l(.4,-.25,.4,.15);l(0,-.25,0,-.75);}
 else if(kind==6){d->AddEllipse(c,{.85f*s,.35f*s},color,0,24,1.7f);l(0,-.35,0,-.7);l(0,-.7,.4,-.7);}
 else if(kind==7){l(0,-.85,0,.55);l(-.3,.1,.3,.1);l(-.2,.55,.2,.55);l(-.2,-.55,0,-.85);l(0,-.85,.2,-.55);}
 else if(kind==8||kind==29){l(-.25,.45,-.25,-.35);l(-.25,-.35,0,-.8);l(0,-.8,.25,-.35);l(.25,-.35,.25,.45);l(-.25,.45,.25,.45);l(-.25,.1,-.55,.65);l(.25,.1,.55,.65);l(-.15,.6,0,.85);l(.15,.6,0,.85);}
 else if(kind==24||kind==34){l(-.8,.75,-.8,-.1);l(-.8,-.1,-.3,-.4);l(-.3,-.4,-.3,-.05);l(-.3,-.05,.2,-.4);l(.2,-.4,.2,.1);l(.2,.1,.8,.1);l(.8,.1,.8,.75);l(-.8,.75,.8,.75);l(.5,.1,.5,-.8);l(.75,.1,.75,-.8);}
 else if(kind==26){l(-.3,-.8,.3,-.8);l(-.2,-.8,-.2,-.15);l(.2,-.8,.2,-.15);l(-.2,-.15,-.7,.7);l(.2,-.15,.7,.7);l(-.7,.7,.7,.7);l(-.5,.35,.5,.35);}
 else if(kind==27){circle(0,0,.42);l(0,-.7,0,.7);l(-.7,0,.7,0);circle(0,0,.8);}
 else if(kind==28){circle(0,-.25,.23);l(0,-.8,0,.85);l(-.6,.5,0,.85);l(0,.85,.6,.5);l(-.6,.5,-.6,.2);l(.6,.5,.6,.2);l(-.45,-.1,.45,-.1);}
 else if(kind==30){l(-.8,.7,-.8,-.4);l(-.8,-.4,0,-.8);l(0,-.8,.8,-.4);l(.8,-.4,.8,.7);l(-.8,.7,.8,.7);l(-.45,-.1,.45,-.1);l(-.45,.2,.45,.2);}
 else if(kind==31){l(-.6,-.8,-.6,.8);l(.6,-.8,.6,.8);l(0,-.7,0,-.3);l(0,-.1,0,.3);l(0,.5,0,.8);}
 else if(kind==32){circle(0,0,.8);l(0,0,.55,-.55);l(-.55,.55,.55,-.55);circle(0,0,.15);}
 else {l(-.7,-.7,.7,.7);l(.7,-.7,-.7,.7);circle(0,0,.85);}
}
inline void badge(ImDrawList* d,ImVec2 c,float size,int kind,ImU32 color,bool selected=false){
 float r=size*.7f;
 d->AddRectFilled({c.x-r,c.y-r},{c.x+r,c.y+r},IM_COL32(10,20,30,242),5);
 d->AddRect({c.x-r,c.y-r},{c.x+r,c.y+r},selected?IM_COL32(247,217,142,255):color,5,0,selected?2.2f:1.0f);
 icon(d,c,size,kind,color);
}
}
