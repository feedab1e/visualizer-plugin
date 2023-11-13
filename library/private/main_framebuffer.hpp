#pragma once
#include"visualizer-plugin/abstraction/gl.hpp"

namespace plugin::impl {
struct main_framebuffer{
  struct client_memory{
    gl::buffer<glm::tvec3<char>> color_image;
    gl::buffer<float> depth_image;
    glm::uvec2 size{};
  };
  void initiate_transfer(client_memory& memory){
    auto total_res = read_buffer_res.x * read_buffer_res.y;
    memory.size = read_buffer_res;
    if(memory.color_image.size() < total_res)
      memory.color_image.resize(total_res);
    if(memory.depth_image.size() < total_res)
      memory.depth_image.resize(total_res);
    read_fb.read_pixels(memory.color_image, {{},read_buffer_res}, GL_COLOR_ATTACHMENT0, GL_BGR);
    read_fb.read_pixels(memory.depth_image, {{}, read_buffer_res}, GL_COLOR_ATTACHMENT0, GL_DEPTH_COMPONENT);
  }
  void resize(glm::uvec2 size){
    write_buffer_res = size;
    if(write_depth_buffer.size() != size)
      write_depth_buffer.resize(size);
    if(write_color_buffer.size() != size)
      write_color_buffer.resize(size);
  }
  void swap(){
    read_buffer_res = write_buffer_res;
    read_depth_buffer.resize(write_depth_buffer.size());
    read_color_buffer.resize(write_color_buffer.size());
    read_fb.bind(1,0);
    write_fb.bind(0,1);
    blit(read_fb, {{}, read_buffer_res}, write_fb, {{}, write_buffer_res}, false);
    blit(read_fb, {{}, read_buffer_res}, write_fb, {{}, write_buffer_res}, true);
  }
  void bind(){
    write_fb.bind(1,0);
    glViewport(0,0,write_buffer_res.x, write_buffer_res.y);
    glClearColor(0.1,0.1,0.1,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  main_framebuffer(glm::uvec2 res):
    read_buffer_res(res),
    write_buffer_res(res),
    read_color_buffer{res, GL_RGBA8, 0},
    write_color_buffer{res, GL_RGBA8, 16},
    read_depth_buffer{res, GL_DEPTH_COMPONENT, 0},
    write_depth_buffer{res, GL_DEPTH_COMPONENT, 16}
  {
    read_fb.attach(read_color_buffer, GL_COLOR_ATTACHMENT0);
    write_fb.attach(write_color_buffer, GL_COLOR_ATTACHMENT0);
    read_fb.attach(read_depth_buffer, GL_DEPTH_ATTACHMENT);
    write_fb.attach(write_depth_buffer, GL_DEPTH_ATTACHMENT);
    write_fb.read_on(GL_COLOR_ATTACHMENT0);
    read_fb .read_on(GL_COLOR_ATTACHMENT0);
    write_fb.draw_on({GL_COLOR_ATTACHMENT0});
    read_fb .draw_on({GL_COLOR_ATTACHMENT0});
  }
  glm::uvec2 read_buffer_res;
  glm::uvec2 write_buffer_res;
  gl::renderbuffer read_color_buffer, write_color_buffer;
  gl::renderbuffer read_depth_buffer, write_depth_buffer;
  gl::framebuffer read_fb;
  gl::framebuffer write_fb;
};
}
