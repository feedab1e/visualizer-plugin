#pragma once
#include "GLFW/glfw3.h"
#include <string>
#include <glm/glm.hpp>
#include <tuple>
#include <utility>
#include <stdexcept>

namespace glfw{
struct glfw{
  static glfw& get(){
    static glfw x;
    return x;
  }
private:
  glfw(){
    glfwInit();
    const char* error;
    if(glfwGetError(&error))
      throw std::runtime_error(error);
  }
  ~glfw(){
    glfwTerminate();
  }
};
struct window{
  void make_current(){
    glfwMakeContextCurrent(w);
    const char* error;
    if(glfwGetError(&error))
      throw std::runtime_error(error);
  }
  void swap(){
    glfwSwapBuffers(w);
  }
private:
  window(GLFWwindow* w):w(w){}
  GLFWwindow* w;
  template<class... Acts>
  friend struct window_builder;
};

enum class window_api:decltype(GLFW_NO_API){
  gl = GLFW_OPENGL_API,
  gles = GLFW_OPENGL_ES_API,
  none = GLFW_NO_API
};
namespace detail{
  struct screen_size{
    auto operator()(){}
    glm::uvec2 size;
  };
  struct window_title{
    auto operator()(){}
    std::string title;
  };
}
template<class... Acts>
struct window_builder{
  window build(){
    glfw::get();
    std::apply([&](auto&&... xs){(xs(), ...);}, acts);
    auto size = std::get<detail::screen_size>(acts).size;
    auto window_title = [&]->const char*{
      if constexpr ((std::same_as<Acts, struct window_title> || ... || false))
        return std::get<detail::screen_size>(acts).title.c_str();
      else return "";
    }();
    auto w = glfwCreateWindow(size.x, size.y, window_title, 0, 0);
    
    const char* error;
    if(glfwGetError(&error))
      throw std::runtime_error(error);
    return w;
  }
  auto visible(bool b = true){
    return add([=]{glfwWindowHint(GLFW_VISIBLE, b);});
  }
  auto transparent(bool b = true){
    return add([=]{glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, b);});
  }
  auto resizeable(bool b = true){
    return add([=]{glfwWindowHint(GLFW_RESIZABLE, b);});
  }
  auto api(window_api a){
    return add([=]{glfwWindowHint(GLFW_CLIENT_API, std::to_underlying(a));});
  }
  auto version(unsigned major, unsigned minor){
    return add([=]{
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    });
  }
  auto size(glm::uvec2 s){
    return add(detail::screen_size{s});
  }
  auto title(std::string s){
    return add(detail::window_title{std::move(s)});
  }
private:
  std::tuple<Acts...> acts;
public:
  window_builder() requires(sizeof...(Acts) == 0){}
  window_builder(decltype(acts) x):acts(std::move(x)){}
private:

  template<class T>
  auto add(T&& x) const& {
    return std::apply([&, this](auto&&... xs){
      return window_builder<Acts..., std::remove_cvref_t<T>>{
        std::tuple<Acts..., std::remove_cvref_t<T>>{xs..., x}
      };
    }, acts);
  }
  template<class T>
  auto add(T&& x) && {
    return std::apply([&, this](auto&&... xs){
      return window_builder<Acts..., std::remove_cvref_t<T>>{
        std::tuple<Acts..., std::remove_cvref_t<T>>{std::move(xs)..., std::move(x)}
      };
    }, acts);
  }
};

window_builder()->window_builder<>;

}
