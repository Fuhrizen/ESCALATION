#pragma once
#include <stdexcept>
#ifndef _WIN32
#include <dlfcn.h>
#endif
namespace escalation {
  // A genuine OpenGL pbuffer for repeatable render tests without an X11/Wayland session.
  // GLFW's null platform supplies input/window metadata; it cannot create EGL window surfaces.
  class HeadlessContext {
#ifndef _WIN32
    void* library_=nullptr;
    void* display_=nullptr;
    void* surface_=nullptr;
    void* context_=nullptr;
    inline static void* activeLibrary_=nullptr;
    template<class T>T function(const char* name){
      auto p=reinterpret_cast<T>(dlsym(library_,name));
      if(!p)throw std::runtime_error(std::string("Missing EGL function ")+name);
      return p;
    }
#endif
    public: void create(int width,int height){
#ifdef _WIN32
      (void)width;
      (void)height;
      throw std::runtime_error("Surfaceless smoke mode currently requires Linux EGL");
#else
      library_=dlopen("libEGL.so.1",RTLD_NOW|RTLD_LOCAL);
      if(!library_)throw std::runtime_error("Render smoke test requires libEGL.so.1");
      activeLibrary_=library_;
      using GetProc=void*(*)(const char*);
      auto proc=function<GetProc>("eglGetProcAddress");
      auto platform=reinterpret_cast<void*(*)(unsigned,void*,const int*)>(proc("eglGetPlatformDisplayEXT"));
      if(!platform)throw std::runtime_error("Surfaceless EGL platform unavailable");
      display_=platform(0x31DD,nullptr,nullptr);
      int major=0,minor=0;
      if(!function<unsigned(*)(void*,int*,int*)>("eglInitialize")(display_,&major,&minor))throw std::runtime_error("EGL display initialization failed");
      if(!function<unsigned(*)(unsigned)>("eglBindAPI")(0x30A2))throw std::runtime_error("Desktop OpenGL EGL API unavailable");
      int attributes[]={0x3033,1,0x3040,8,0x3024,8,0x3023,8,0x3022,8,0x3021,8,0x3025,24,0x3026,8,0x3038};
      void* config=nullptr;
      int count=0;
      if(!function<unsigned(*)(void*,const int*,void**,int,int*)>("eglChooseConfig")(display_,attributes,&config,1,&count)||count==0)throw std::runtime_error("EGL pbuffer config unavailable");
      int surfaceAttributes[]={0x3057,width,0x3056,height,0x3038};
      surface_=function<void*(*)(void*,void*,const int*)>("eglCreatePbufferSurface")(display_,config,surfaceAttributes);
      int contextAttributes[]={0x3098,3,0x30FB,3,0x30FD,1,0x3038};
      context_=function<void*(*)(void*,void*,void*,const int*)>("eglCreateContext")(display_,config,nullptr,contextAttributes);
      if(!surface_||!context_||!function<unsigned(*)(void*,void*,void*,void*)>("eglMakeCurrent")(display_,surface_,surface_,context_))throw std::runtime_error("EGL OpenGL 3.3 context creation failed");
#endif
    }
    static void* getProc(const char* name){
#ifdef _WIN32
      (void)name;
      return nullptr;
#else
      auto proc=reinterpret_cast<void*(*)(const char*)>(dlsym(activeLibrary_,"eglGetProcAddress"));
      return proc?proc(name):nullptr;
#endif
    }
    ~HeadlessContext(){
#ifndef _WIN32
      if(library_){
        auto current=reinterpret_cast<unsigned(*)(void*,void*,void*,void*)>(dlsym(library_,"eglMakeCurrent"));
        auto destroyContext=reinterpret_cast<unsigned(*)(void*,void*)>(dlsym(library_,"eglDestroyContext"));
        auto destroySurface=reinterpret_cast<unsigned(*)(void*,void*)>(dlsym(library_,"eglDestroySurface"));
        auto terminate=reinterpret_cast<unsigned(*)(void*)>(dlsym(library_,"eglTerminate"));
        if(display_){
          current(display_,nullptr,nullptr,nullptr);
          if(context_)destroyContext(display_,context_);
          if(surface_)destroySurface(display_,surface_);
          terminate(display_);
        }
        dlclose(library_);
        activeLibrary_=nullptr;
      }
#endif
    }
  };
}
