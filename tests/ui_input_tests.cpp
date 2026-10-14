// Exercise the real App input path using Dear ImGui's queued input events.
// No renderer/context initialization: these are native input integration checks.
#define main escalation_client_main
#include "../src/main.cpp"
#undef main
#include <stdexcept>
int main(){
  int checks=0;
  auto check=[&](bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);};
  try{
    IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();
    io.DisplaySize={1280,720};io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.Fonts->AddFontDefault();unsigned char* pixels=nullptr;int width=0,height=0;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    World w;w.load("data/world.geojson","data/histories.json");
    App app(std::move(w),"build-native/input-test-settings");app.screen=App::Game;app.sim.start(app.world.find("FRA"));
    app.camera.globe=false;app.camera.zoom=8;app.camera.width=900;app.camera.height=600;app.camera.x=0;app.camera.y=60;app.windowFocused=true;
    auto frame=[&](bool typing=false,bool captured=false){io.DeltaTime=1.f/60;ImGui::NewFrame();io.WantCaptureKeyboard=true;io.WantTextInput=typing;io.WantCaptureMouse=captured;app.controls(1.0/60);ImGui::EndFrame();};
    io.AddMousePosEvent(450,360);frame();
    double longitude=app.camera.longitude;io.AddKeyEvent(ImGuiKey_D,true);frame();
    check(app.camera.longitude>longitude,"D must pan despite generic ImGui keyboard navigation capture");
    io.AddKeyEvent(ImGuiKey_D,false);frame();longitude=app.camera.longitude;
    io.AddKeyEvent(ImGuiKey_LeftArrow,true);frame();check(app.camera.longitude<longitude,"Left arrow must pan");
    io.AddKeyEvent(ImGuiKey_LeftArrow,false);frame();longitude=app.camera.longitude;
    io.AddKeyEvent(ImGuiKey_D,true);frame(true);check(app.camera.longitude==longitude,"Typing suppresses map keyboard motion");
    io.AddKeyEvent(ImGuiKey_D,false);frame();
    app.panel=App::Research;longitude=app.camera.longitude;io.AddKeyEvent(ImGuiKey_D,true);frame();
    check(app.camera.longitude==longitude,"Research canvas must not pan the hidden map");
    io.AddKeyEvent(ImGuiKey_D,false);frame();app.panel=App::None;
    longitude=app.camera.longitude;app.openPause();io.AddKeyEvent(ImGuiKey_D,true);frame();
    check(app.camera.longitude==longitude,"Pause menu suppresses camera commands");
    io.AddKeyEvent(ImGuiKey_D,false);app.pauseOpen=false;frame();
    app.camera.longitude=app.sim.state.units[0].position.x;app.camera.latitude=app.sim.state.units[0].position.y;app.selectedUnit=0;app.selectedUnits={0};
    io.AddMousePosEvent(450,360);frame();io.AddMouseButtonEvent(ImGuiMouseButton_Middle,true);frame();
    longitude=app.camera.longitude;io.AddMousePosEvent(1100,360);frame(false,true);
    check(app.camera.longitude!=longitude,"A map-owned middle drag continues over the inspector");
    io.AddMouseButtonEvent(ImGuiMouseButton_Middle,false);frame(false,true);app.cameraMotion.reset();
    app.construction=int(Facility::Factory);app.sim.state.buildingInventory[int(Facility::Factory)]=1;
    io.AddMousePosEvent(450,360);frame();io.AddMouseButtonEvent(ImGuiMouseButton_Right,true);frame();
    io.AddMousePosEvent(1100,360);frame(false,true);io.AddMousePosEvent(450,360);frame();io.AddMouseButtonEvent(ImGuiMouseButton_Right,false);frame();
    check(app.construction==int(Facility::Factory),"A right drag returning from the dock must not become placement cancel");
    app.cameraMotion.reset();app.camera.longitude=app.sim.state.units[0].position.x;app.camera.latitude=app.sim.state.units[0].position.y;
    io.AddMousePosEvent(450,360);frame();io.AddMouseButtonEvent(ImGuiMouseButton_Left,true);frame();
    io.AddMousePosEvent(1100,360);frame(false,true);io.AddMousePosEvent(450,360);frame();io.AddMouseButtonEvent(ImGuiMouseButton_Left,false);frame();
    check(app.sim.state.buildingInventory[int(Facility::Factory)]==1,"A left drag returning to its origin must not deploy inventory");
    io.AddMouseButtonEvent(ImGuiMouseButton_Left,true);frame();app.openPause();io.AddMouseButtonEvent(ImGuiMouseButton_Left,false);frame();
    check(app.sim.state.buildingInventory[int(Facility::Factory)]==1,"Opening a modal mid-press prevents placement on release");
    app.pauseOpen=false;frame();io.AddMouseWheelEvent(0,1);frame();check(app.cameraMotion.zooming,"Wheel must begin anchored zoom");
    double zoom=app.camera.zoom;app.openPause();frame();check(!app.cameraMotion.zooming&&app.camera.zoom==zoom,"Modal opening cancels pending wheel zoom");
    app.pauseOpen=false;app.selectedUnit=0;app.selectedUnits={0};app.sim.state.units[0].health=0;frame();
    check(app.selectedUnit<0&&app.selectedUnits.empty(),"Destroyed selected formations stop offering live orders");
    ImGui::DestroyContext();std::cout<<"Passed "<<checks<<" native input integration checks\n";return 0;
  }catch(const std::exception& e){std::cerr<<"Input check failed after "<<checks<<": "<<e.what()<<"\n";ImGui::DestroyContext();return 1;}
}
