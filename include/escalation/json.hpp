#pragma once
#include <picojson.h>
#include <fstream>
#include <stdexcept>
#include <sstream>
namespace escalation {
  using Json = picojson::value;
  using Object=picojson::object;
  using Array=picojson::array;
  inline Json readJson(const std::string& path) {
    std::ifstream f(path);
    if(!f) throw std::runtime_error("Cannot open "+path);
    Json v;
    std::string e=picojson::parse(v,f);
    if(!e.empty()) throw std::runtime_error(path+": "+e);
    return v;
  }
  inline const Json& field(const Json& j,const std::string& k) {
    if(!j.is<Object>()||!j.contains(k)) throw std::runtime_error("Missing field: "+k);
    return j.get(k);
  }
  inline double number(const Json& j,const std::string& k){
    auto& v=field(j,k);
    if(!v.is<double>())throw std::runtime_error("Expected number: "+k);
    return v.get<double>();
  }
  inline std::string string(const Json& j,const std::string& k){
    auto& v=field(j,k);
    if(!v.is<std::string>())throw std::runtime_error("Expected text: "+k);
    return v.get<std::string>();
  }
  inline const Array& asArray(const Json& j){
    if(!j.is<Array>())throw std::runtime_error("Expected JSON array");
    return j.get<Array>();
  }
  inline double asNumber(const Json& j){
    if(!j.is<double>())throw std::runtime_error("Expected JSON number");
    return j.get<double>();
  }
  inline const std::string& asString(const Json& j){
    if(!j.is<std::string>())throw std::runtime_error("Expected JSON text");
    return j.get<std::string>();
  }
  inline Json num(double n){
    return Json(n);
  }
  inline Json str(const std::string& s){
    return Json(s);
  }
}
