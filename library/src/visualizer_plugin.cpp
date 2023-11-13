#include <atomic>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#define GLM_FORCE_SWIZZLE

#include <GL/glew.h>
#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <dlfcn.h>

#include "visualizer-plugin/abstraction/gl.hpp"
#include "visualizer-plugin/abstraction/glfw.hpp"
#include "plane_renderer.hpp"
#include "main_framebuffer.hpp"
#include "visualizer-plugin/visualizer-plugin.hpp"

namespace asio = boost::asio;
using asio::awaitable;
using asio::use_awaitable;
using namespace asio::experimental;
using namespace asio::experimental::awaitable_operators;

namespace plugin {

struct renderer_context::pimpl {
  glm::uvec2 res;
  glm::vec2 dir = {};
  glm::vec3 lookat = {};
  glm::vec3 camera_pos = {};
  glm::vec3 cursor_pos = {};
  float zoom = 1;
  float logzoom = 1;
  glm::ivec2 mouse_pos = res / 2u;
  struct mouse_button_bits {
    bool left :1;
    bool middle :1;
    bool right :1;
  } mouse_buttons;
  
  auto calculate_view() const  {
    return glm::lookAt(camera_pos, lookat, {0., 1., 0.});
  }
  
  auto calculate_proj() const  {
    return glm::perspective(
      glm::radians(90.f),
      (float) res.x / (float) res.y,
      0.01f * zoom,
      1000.f * zoom
    );
  }
  
  auto calculate_matrix() const  {
    auto project = glm::perspective(
      glm::radians(90.f),
      (float) res.x / (float) res.y,
      0.01f * zoom,
      1000.f * zoom
    );
    auto view = glm::lookAt(camera_pos, lookat, {0., 1., 0.});
    return project * view;
  }
  
  void calculate_zoom() { zoom = std::exp(logzoom); }
  
  void calculate_camera_pos() {
    camera_pos = lookat + zoom * glm::vec3{
      4. * std::sin(dir.x) * std::cos(dir.y),
      -4. * std::sin(dir.y),
      4. * std::cos(dir.x) * std::cos(dir.y)
    };
  }
};

glm::uvec2 renderer_context::resolution() const { return impl.res; }

glm::vec3 renderer_context::position() const { return impl.camera_pos; }

glm::vec3 renderer_context::focus() const { return impl.lookat; }

float renderer_context::camera_scale() const { return impl.zoom; }

glm::mat4 renderer_context::matrix() const { return impl.calculate_matrix(); }

namespace impl {
struct plane_type;
}

struct render_core {
  std::vector<std::unique_ptr<renderer_base>> opaque_renderers;
  std::vector<std::unique_ptr<renderer_base>> transparent_renderers;
  static boost::lockfree::spsc_queue<std::function<renderer_base *()>>
    constructor_queue;
  
  render_core(render_core &&) = delete;
  
  render_core(const render_core &) = delete;
  
  boost::asio::io_context ctx;
  // struct frame_data {
  //   gl::buffer<glm::tvec3<char>> memory;
  //   gl::buffer<float> depth;
  //   glm::uvec2 size{};
  // };
  plugin::renderer_context::pimpl render_data;
  // concurrent_channel<void(boost::system::error_code, frame_data)>
  //   render_to_sender;
  // concurrent_channel<void(boost::system::error_code, frame_data)>
  //   sender_to_render;
  concurrent_channel<void(boost::system::error_code, impl::main_framebuffer::client_memory)>
    render_to_sender;
  concurrent_channel<void(boost::system::error_code, impl::main_framebuffer::client_memory)>
    sender_to_render;
  glfw::window window;
  std::string ip;
  uint32_t port;
  
  render_core(const char *ip, uint32_t port) :
    window{
      glfw::window_builder{}
        .size(
          {100, 100}
        )
        .title("hello")
        .api(glfw::window_api::gl)
          //.version(4,6)
        .visible(false)
        .build()
    },
    render_data{
      {500, 500},
      {1,   0.5}
    },
    render_to_sender(ctx, 2),
    sender_to_render(ctx, 1),
    ip(ip),
    port(port) {}
  
  auto setup_gl() {
    window.make_current();
    
    GLenum err = glewInit();
    if(err != GLEW_OK)
      throw std::runtime_error{(const char *) glewGetErrorString(err)};
    
    using ec = boost::system::error_code;
    sender_to_render.try_send(
      ec{},
      impl::main_framebuffer::client_memory{.color_image = {{}}, .depth_image = {{}}, .size = {}}
    );
    sender_to_render.try_send(
      ec{},
      impl::main_framebuffer::client_memory{.color_image = {{}}, .depth_image = {{}}, .size = {}}
    );
    transparent_renderers
      .emplace_back((renderer_base *) new renderer<impl::plane_type>);
  }
  
  auto gl_settings() {
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glClearColor(0.1, 0.1, 0.1, 1.0);
  }
  
  auto run() {
    setup_gl();
    asio::co_spawn(
      ctx,
      [&] -> awaitable<void> {
        asio::ip::tcp::endpoint ep(asio::ip::address::from_string(ip), port);
        asio::ip::tcp::socket s(ctx);
        std::cerr << "connecting\n";
        co_await s.async_connect(ep, use_awaitable);
        std::cerr << "connected\n";
        try {
          co_await asio::experimental::make_parallel_group(
            asio::co_spawn(ctx, render(), asio::deferred),
            asio::co_spawn(ctx, sender(s), asio::deferred),
            asio::co_spawn(ctx, handle_updates(s), asio::deferred)
          )
            .async_wait(
              asio::experimental::wait_for_one_error(),
              use_awaitable
            );
        }
        catch(std::exception &x) { std::cerr << x.what() << "\n"; }
      },
      asio::detached
    );
    ctx.run();
  }
  
  struct vertex {
    glm::vec3 pos;
  };
  
  awaitable<void> render() try {
    using type = gl::shader_type;
    auto render_state = render_data;
    auto &res = render_state.res;
    impl::main_framebuffer fb(res);
    
    for(;;) {
      for(std::function<renderer_base *()> elem;
        constructor_queue.pop(&elem, 1);) {
        try {
          auto r = elem();
          (r->is_transparent() ? transparent_renderers : opaque_renderers)
            .emplace_back(r);
        }
        catch(...){
        }
      }
      render_data.calculate_zoom();
      render_data.calculate_camera_pos();
      render_state = render_data;
      
      fb.resize(res);
      auto data = co_await sender_to_render.async_receive(use_awaitable);
      fb.initiate_transfer(data);
      fb.bind();
      gl_settings();
      
      for(auto &r:opaque_renderers) {
        r->render({render_state});
        co_await asio::this_coro::executor;
      }
      glDepthMask(false);
      for(auto &r:transparent_renderers) {
        r->render({render_state});
        co_await asio::this_coro::executor;
      }
      glDepthMask(true);
      
      fb.swap();
      window.swap();
      
      co_await render_to_sender.async_send({}, std::move(data), use_awaitable);
    }
  }
  catch(std::exception &e) {
    std::cerr << e.what();
    throw;
  }
  
  awaitable<void> handle_updates(asio::ip::tcp::socket &s) {
    enum class type : uint32_t {
      resize,
      mouse_click,
      mouse_down,
      mouse_up,
      mouse_wheel,
      scroll,
      mouse_drag,
      mouse_move
    };
    struct {
      glm::ivec3 data;
      type t;
    } msg;
    for(;;) {
      co_await async_read(s, asio::buffer(&msg, sizeof msg), use_awaitable);
      msg.data.x = std::byteswap(msg.data.x);
      msg.data.y = std::byteswap(msg.data.y);
      msg.data.z = std::byteswap(msg.data.z);
      msg.t = (type) std::byteswap(std::to_underlying(msg.t));
      
      switch(msg.t) {
      case type::resize: render_data.res = msg.data.xy();
        break;
      case type::mouse_down:
        switch(msg.data.z) {
        case 1: render_data.mouse_buttons.left = 1;
          break;
        case 2: render_data.mouse_buttons.middle = 1;
          break;
        case 3: render_data.mouse_buttons.right = 1;
          break;
        default: break;
        }
        render_data.mouse_pos = msg.data.xy();
        break;
      case type::mouse_up:
        switch(msg.data.z) {
        case 1: render_data.mouse_buttons.left = 0;
          break;
        case 2: render_data.mouse_buttons.middle = 0;
          break;
        case 3: render_data.mouse_buttons.right = 0;
          break;
        default: break;
        }
        render_data.mouse_pos = msg.data.xy();
        break;
      case type::mouse_drag: {
        auto delta = msg.data.xy() - render_data.mouse_pos;
        namespace num = std::numbers;
        if(render_data.mouse_buttons.left)
          render_data.dir = {
            std::fmod(
              (delta.x) / -400.f + render_data.dir.x,
              (float) num::pi * 2.f
            ),
            std::clamp(
              (delta.y) / 400.f + render_data.dir.y,
              -(float) num::pi / 2.f,
              (float) num::pi / 2.f
            )
          };
        if(render_data.mouse_buttons.right) {
          auto dir = render_data.dir.x;
          auto dx = delta.x / -100.f * glm::vec2{cos(dir), -sin(dir)};
          auto dy = delta.y / -100.f * glm::vec2{sin(dir), cos(dir)};
          render_data.lookat
            += render_data.zoom
            * glm::vec3(dx + dy * (float) sin(render_data.dir.y), 0).xzy();
        }
        if(render_data.mouse_buttons.middle) {
          render_data.zoom += delta.y / 400.f;
        }
        render_data.mouse_pos = msg.data.xy();
      }
        break;
      case type::mouse_move: render_data.mouse_pos = msg.data.xy();
        break;
      case type::scroll: {
        double amt
          = std::bit_cast<double>(
            std::array<int32_t, 2>{msg.data.y, msg.data.x}
          );
        render_data.logzoom += amt * 0.1;
      }
        break;
      default: break;
      }
    }
  }
  
  awaitable<void> sender(asio::ip::tcp::socket &s) {
    for(size_t packetid = 0;; ++packetid) {
      auto data = co_await render_to_sender.async_receive(use_awaitable);
      const void *ptr = data.color_image.map(GL_READ_ONLY);
      const auto *depth = data.depth_image.map(GL_READ_ONLY);
      auto screenspace_xy
        = glm::vec2(render_data.mouse_pos) * 2.f / glm::vec2(render_data.res)
          - glm::vec2(1);
      
      auto clamped_pos = glm::clamp(
        render_data.mouse_pos,
        glm::ivec2{},
        (glm::ivec2) data.size
      );
      
      auto idx = clamped_pos.x + (clamped_pos.y) * data.size.x;
      auto d = depth[idx];
      glm::vec3 pt{screenspace_xy, d * 2 - 1};
      auto mat = glm::inverse(render_data.calculate_matrix());
      auto correct = [](auto &&p) { return p / p.w; };
      [[maybe_unused]] auto pt2 = correct(mat * glm::vec4(pt, 1)).xyz();
      
      size_t size
        = data.size.x * data.size.y * sizeof(decltype(data.color_image)::value_type);
      struct {
        uint16_t magic, w, h;
        uint32_t total;
      } header{
        .magic = 0xADDE,
        .w = std::byteswap((uint16_t) data.size.x),
        .h = std::byteswap((uint16_t) data.size.y),
        .total = std::byteswap((uint32_t) size)
      };
      co_await asio::async_write(
        s,
        asio::const_buffer{(const void *) &header, sizeof header},
        use_awaitable
      );
      co_await asio::async_write(
        s,
        asio::const_buffer{(const void *) ptr, size},
        use_awaitable
      );
      data.depth_image.unmap();
      data.color_image.unmap();
      co_await sender_to_render.async_send({}, std::move(data), use_awaitable);
    }
  }
};

boost::lockfree::spsc_queue<std::function<renderer_base *()>>
  render_core::constructor_queue{1024};
namespace impl {
void add(const std::function<renderer_base *()>& f) {
  render_core::constructor_queue.push(f);
}
} // namespace impl

std::optional<std::thread> thread{};

void open(const char *ip, uint32_t port) {
  static render_core app{ip, port};
  thread = std::thread{[&] { app.run(); }};
}

} // namespace plugin
