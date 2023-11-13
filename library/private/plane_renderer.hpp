#pragma once
#include"visualizer-plugin/visualizer-plugin.hpp"
#include"visualizer-plugin/abstraction/gl.hpp"

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(visualizer_plugin);

namespace plugin{
namespace impl{
struct plane_type{};
inline std::string_view get_file(const char* path){
  auto file = cmrc::visualizer_plugin::get_filesystem().open(path);
  return {file.begin(), file.end()};
}
}
template<>
struct renderer<impl::plane_type>:renderer_base{
  gl::program plane_p{
  gl::shader<gl::shader_type::vertex>  {impl::get_file("shaders/plane.vert")},
  gl::shader<gl::shader_type::fragment>{impl::get_file("shaders/plane.frag")},
  };
  gl::vertex_array plane_vao{plane_p};
  bool is_transparent() const override {
    return true;
  }
  void render(const renderer_context ctx) override {
    plane_p.bind();
    plane_vao.bind();
    auto mat = ctx.matrix();
    glUniform2f(plane_p.uniform_loc("size"), ctx.resolution().x, ctx.resolution().y);
    glUniform3f(plane_p.uniform_loc("offset"), ctx.focus().x, ctx.focus().y, ctx.focus().z);
    glUniform1f(plane_p.uniform_loc("scale"), 1/ctx.camera_scale());
    glUniformMatrix4fv(plane_p.uniform_loc("mvp"), 1, false, &mat[0][0]);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_CULL_FACE);
  }
};

}
