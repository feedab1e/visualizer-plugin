#pragma once
#include <glm/glm.hpp>
#include <concepts>
#include <functional>
#include <string_view>

namespace plugin{

struct renderer_context{
  glm::uvec2 resolution() const;
  glm::vec3 position() const;
  glm::vec3 focus() const;
  float camera_scale() const;
  glm::mat4 matrix() const;
private:
  struct pimpl;
  friend struct render_core;
  renderer_context(pimpl& x):impl(x){}
  pimpl& impl;
};

struct renderer_base{
  virtual void render(const renderer_context c) = 0;
  virtual bool is_transparent() const = 0;
  virtual ~renderer_base() = default;
};
namespace impl{
  void add(const std::function<renderer_base*()>&);
}

template<class T>
struct renderer{
  struct type;
  static void add(T x){
    //static_assert(
    //  std::derived_from<renderer_base, type>,
    //  "your renderer must be derived from \"::plugin::renderer_base\""
    //);
    //static_assert(
    //  std::constructible_from<T, type>,
    //  "your renderer must be constructible from the value you are trying to render"
    //);
    impl::add([x]{return new type{x};});
  }
};

}
