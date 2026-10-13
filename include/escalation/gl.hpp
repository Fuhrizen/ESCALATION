#pragma once
#include <cstddef>
#include <stdexcept>
namespace escalation::gl {
  using Enum=unsigned;
  using UInt=unsigned;
  using Int=int;
  using Size=int;
  using Char=char;
  using Bool=unsigned char;
  using Float=float;
#ifdef _WIN32
#define ES_GL_CALL __stdcall
#else
#define ES_GL_CALL
#endif
#define ES_GL_FUNCTIONS(F) \
 F(void,Viewport,(Int,Int,Size,Size)) F(void,ClearColor,(Float,Float,Float,Float)) F(void,Clear,(Enum)) \
 F(void,Enable,(Enum)) F(void,Disable,(Enum)) F(void,BlendFunc,(Enum,Enum)) F(void,DepthFunc,(Enum)) \
 F(UInt,CreateShader,(Enum)) F(void,ShaderSource,(UInt,Size,const Char* const*,const Int*)) F(void,CompileShader,(UInt)) \
 F(void,GetShaderiv,(UInt,Enum,Int*)) F(void,GetShaderInfoLog,(UInt,Size,Size*,Char*)) F(void,DeleteShader,(UInt)) \
 F(UInt,CreateProgram,()) F(void,AttachShader,(UInt,UInt)) F(void,LinkProgram,(UInt)) F(void,GetProgramiv,(UInt,Enum,Int*)) \
 F(void,GetProgramInfoLog,(UInt,Size,Size*,Char*)) F(void,UseProgram,(UInt)) F(void,DeleteProgram,(UInt)) \
 F(Int,GetUniformLocation,(UInt,const Char*)) F(void,Uniform1i,(Int,Int)) F(void,Uniform1f,(Int,Float)) F(void,Uniform2f,(Int,Float,Float)) \
 F(void,GenVertexArrays,(Size,UInt*)) F(void,BindVertexArray,(UInt)) F(void,DeleteVertexArrays,(Size,const UInt*)) \
 F(void,GenBuffers,(Size,UInt*)) F(void,BindBuffer,(Enum,UInt)) F(void,BufferData,(Enum,std::ptrdiff_t,const void*,Enum)) F(void,DeleteBuffers,(Size,const UInt*)) \
 F(void,EnableVertexAttribArray,(UInt)) F(void,VertexAttribPointer,(UInt,Int,Enum,Bool,Size,const void*)) F(void,DrawArrays,(Enum,Int,Size)) \
 F(void,GenTextures,(Size,UInt*)) F(void,BindTexture,(Enum,UInt)) F(void,TexParameteri,(Enum,Enum,Int)) \
 F(void,TexImage2D,(Enum,Int,Int,Size,Size,Int,Enum,Enum,const void*)) F(void,TexSubImage2D,(Enum,Int,Int,Int,Size,Size,Enum,Enum,const void*)) \
 F(void,ActiveTexture,(Enum)) F(void,DeleteTextures,(Size,const UInt*)) F(void,ReadPixels,(Int,Int,Size,Size,Enum,Enum,void*)) F(void,PixelStorei,(Enum,Int)) \
 F(const unsigned char*,GetString,(Enum)) F(Enum,GetError,())
#define DECLARE(result,name,args) inline result (ES_GL_CALL *name) args=nullptr;
  ES_GL_FUNCTIONS(DECLARE)
#undef DECLARE
  inline void load(void*(*proc)(const char*)) {
#define LOAD(result,name,args) name=reinterpret_cast<result(ES_GL_CALL *)args>(proc("gl" #name));if(!name)throw std::runtime_error("OpenGL function missing: gl" #name);
    ES_GL_FUNCTIONS(LOAD)
#undef LOAD
  }
  constexpr Enum COLOR_BUFFER_BIT=0x4000,DEPTH_BUFFER_BIT=0x100,DEPTH_TEST=0xB71,BLEND=0xBE2,SRC_ALPHA=0x302,ONE_MINUS_SRC_ALPHA=0x303,LEQUAL=0x203;
  constexpr Enum VERTEX_SHADER=0x8B31,FRAGMENT_SHADER=0x8B30,COMPILE_STATUS=0x8B81,LINK_STATUS=0x8B82,ARRAY_BUFFER=0x8892,STATIC_DRAW=0x88E4,FLOAT=0x1406,TRIANGLES=4,LINES=1;
  constexpr Enum LINEAR=0x2601,TEXTURE_WRAP_S=0x2802,TEXTURE_WRAP_T=0x2803,REPEAT=0x2901,CLAMP_TO_EDGE=0x812F,RGBA8=0x8058,TEXTURE1=0x84C1;
  constexpr Enum TEXTURE_2D=0xDE1,TEXTURE0=0x84C0,TEXTURE_MIN_FILTER=0x2801,TEXTURE_MAG_FILTER=0x2800,NEAREST=0x2600,RGBA32F=0x8814,RGBA=0x1908,RGB=0x1907,UNSIGNED_BYTE=0x1401,PACK_ALIGNMENT=0xD05,VERSION=0x1F02,RENDERER=0x1F01;
}
