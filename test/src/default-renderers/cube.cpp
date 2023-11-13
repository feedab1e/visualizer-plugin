#include "visualizer-plugin/visualizer-plugin.hpp"
#include "visualizer-plugin/abstraction/gl.hpp"
#include <cmrc/cmrc.hpp>
#include <iostream>

CMRC_DECLARE(default_renderers);

namespace plugin::impl {
inline  namespace default_renderers {

inline std::string_view get_file(const char *path) {
  auto fs = cmrc::default_renderers::get_filesystem();
  auto file = fs.open(path);
  return {file.begin(), file.end()};
}

}
}
template<>
struct plugin::renderer<int>::type: plugin::renderer_base{
  
  struct vertex{
    glm::vec3 pos;
  };
  gl::program cube_p = {
    gl::shader<gl::shader_type::vertex>  {impl::get_file("shaders/cube.vert")},
    gl::shader<gl::shader_type::fragment>{impl::get_file("shaders/cube.frag")}
  };
  gl::buffer<vertex> cube_mesh = {
    {{-1.f,  1.f,  1.f}}, {{ 1.f,  1.f,  1.f}},
    {{-1.f, -1.f,  1.f}}, {{ 1.f, -1.f,  1.f}},
    {{ 1.f, -1.f, -1.f}}, {{ 1.f,  1.f,  1.f}},
    {{ 1.f,  1.f, -1.f}}, {{-1.f,  1.f,  1.f}},
    {{-1.f,  1.f, -1.f}}, {{-1.f, -1.f,  1.f}},
    {{-1.f, -1.f, -1.f}}, {{ 1.f, -1.f, -1.f}},
    {{-1.f,  1.f, -1.f}}, {{ 1.f,  1.f, -1.f}}
  };
  gl::vertex_array cube_vao = {cube_p, cube_mesh};
  type(int){}
  bool is_transparent() const override {
    return false;
  }
  void render(plugin::renderer_context ctx) override{
      cube_p.bind();
      glUniform2f(cube_p.uniform_loc("size"), ctx.resolution().x, ctx.resolution().y);
      glUniformMatrix4fv(cube_p.uniform_loc("mvp"), 1, false, &ctx.matrix()[0][0]);
      cube_vao.bind();
      glDrawArrays(GL_TRIANGLE_STRIP, 0, cube_mesh.size());

  }
};
template void plugin::renderer<int>::add(int);
