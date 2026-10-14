#include "escalation/assets.hpp"
#include "escalation/json.hpp"
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>
namespace escalation {
  static gl::UInt texture(const std::string& path,int& width,int& height,bool repeat){
    int components;
    auto* bytes=stbi_load(path.c_str(),&width,&height,&components,4);
    if(!bytes)throw std::runtime_error("Cannot load map asset: "+path);
    gl::UInt id;
    gl::GenTextures(1,&id);
    gl::BindTexture(gl::TEXTURE_2D,id);
    gl::TexParameteri(gl::TEXTURE_2D,gl::TEXTURE_MIN_FILTER,repeat?0x2703:gl::LINEAR); // linear mipmaps for geographic surfaces
    gl::TexParameteri(gl::TEXTURE_2D,gl::TEXTURE_MAG_FILTER,gl::LINEAR);
    gl::TexParameteri(gl::TEXTURE_2D,gl::TEXTURE_WRAP_S,repeat?gl::REPEAT:gl::CLAMP_TO_EDGE);
    gl::TexParameteri(gl::TEXTURE_2D,gl::TEXTURE_WRAP_T,gl::CLAMP_TO_EDGE);
    gl::TexImage2D(gl::TEXTURE_2D,0,gl::RGBA8,width,height,0,gl::RGBA,gl::UNSIGNED_BYTE,bytes);
    if(repeat){
      // Upload a complete filtered pyramid without adding a GL extension dependency.
      std::vector<unsigned char> previous(bytes,bytes+width*height*4);
      int w=width,h=height,level=1;
      while(w>1||h>1){
        int nw=std::max(1,w/2),nh=std::max(1,h/2);
        std::vector<unsigned char> next(nw*nh*4);
        for(int y=0;y<nh;++y)for(int x=0;x<nw;++x)for(int c=0;c<4;++c){
          unsigned sum=0;
          for(int j=0;j<2;++j)for(int i=0;i<2;++i)sum+=previous[(std::min(h-1,2*y+j)*w+std::min(w-1,2*x+i))*4+c];
          next[(y*nw+x)*4+c]=static_cast<unsigned char>((sum+2)/4);
        }
        gl::TexImage2D(gl::TEXTURE_2D,level++,gl::RGBA8,nw,nh,0,gl::RGBA,gl::UNSIGNED_BYTE,next.data());
        previous=std::move(next);w=nw;h=nh;
      }
    }
    stbi_image_free(bytes);
    return id;
  }
  void SurfaceAssets::load(const std::string& directory,const World& world){
    terrain=texture(directory+"/terrain-surface.png",terrainWidth,terrainHeight,true);
    int width,height;
    flags=texture(directory+"/flags-atlas.png",width,height,false);
    auto metadata=readJson(directory+"/flags.json");
    auto& nations=field(metadata,"nations");
    indices_.clear();
    for(auto& country:world.countries){
      int index=int(number(field(nations,country.code),"index"));
      if(index<0||index>=192)throw std::runtime_error("Invalid flag atlas index");
      indices_.push_back(index);
    }
  }
  FlagUV SurfaceAssets::flag(int country) const {
    int index=country>=0&&country<int(indices_.size())?indices_[country]:0;
    return {
      {
        (index%16)/16.0,(index/16)/12.0
      },{
        (index%16+1)/16.0,(index/16+1)/12.0
      }
    };
  }
  void SurfaceAssets::shutdown(){
    if(terrain)gl::DeleteTextures(1,&terrain);
    if(flags)gl::DeleteTextures(1,&flags);
    terrain=flags=0;
  }
}
