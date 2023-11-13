#pragma once
#include<GL/glew.h>
#include<glm/glm.hpp>
#include<string>
#include<string_view>
#include<stdexcept>
#include<concepts>
#include<ranges>

#include "boost/pfr.hpp"

namespace gl{
namespace detail{
template<class T>
constexpr size_t component_count = 1;
template<class T, auto P>
constexpr size_t component_count<glm::tvec1<T, P>> = 1;
template<class T, auto P>
constexpr size_t component_count<glm::tvec2<T, P>> = 2;
template<class T, auto P>
constexpr size_t component_count<glm::tvec3<T, P>> = 3;
template<class T, auto P>
constexpr size_t component_count<glm::tvec4<T, P>> = 4;
template<class T>         struct component_type_;
template<class T, auto P> struct component_type_<glm::tvec1<T, P>>{using type = T;};
template<class T, auto P> struct component_type_<glm::tvec2<T, P>>{using type = T;};
template<class T, auto P> struct component_type_<glm::tvec3<T, P>>{using type = T;};
template<class T, auto P> struct component_type_<glm::tvec4<T, P>>{using type = T;};
template<std::integral T> struct component_type_<T>{using type = T;};
template<std::floating_point T> struct component_type_<T>{using type = T;};
template<class T>
using component_type = typename component_type_<T>::type;
template<class T>
inline constexpr unsigned gl_type_id = -1;
template<> inline constexpr unsigned gl_type_id<   GLint> = GL_INT;
template<> inline constexpr unsigned gl_type_id<  GLuint> = GL_UNSIGNED_INT;
template<> inline constexpr unsigned gl_type_id< GLshort> = GL_SHORT;
template<> inline constexpr unsigned gl_type_id<GLushort> = GL_UNSIGNED_SHORT;
template<> inline constexpr unsigned gl_type_id<  GLbyte> = GL_BYTE;
template<> inline constexpr unsigned gl_type_id<    char> = GL_BYTE;
template<> inline constexpr unsigned gl_type_id< GLubyte> = GL_UNSIGNED_BYTE;
template<> inline constexpr unsigned gl_type_id< GLfloat> = GL_FLOAT;
template<> inline constexpr unsigned gl_type_id<GLdouble> = GL_DOUBLE;
}

enum class shader_type:unsigned{
  compute = GL_COMPUTE_SHADER,
  vertex = GL_VERTEX_SHADER,
  tess_control = GL_TESS_CONTROL_SHADER,
  tess_evaluation = GL_TESS_EVALUATION_SHADER,
  geometry = GL_GEOMETRY_SHADER,
  fragment = GL_FRAGMENT_SHADER
};

template<class T>
struct tbox2{
  glm::tvec2<T> min;
  glm::tvec2<T> max;
};
using ubox2 = tbox2<unsigned>;
using ibox2 = tbox2<int>;
using bbox2 = tbox2<char>;
using dbox2 = tbox2<double>;
using box2 = tbox2<float>;

template<shader_type type>
struct shader{
  shader():handle{}{}
  shader(shader&& other):handle(std::exchange(other.handle, 0)){}
  shader(const shader&) = delete;
  operator bool() const{
    return handle;
  }
  friend struct program;
  shader(std::string_view text):
    handle{glCreateShader((unsigned)type)}{
    int size = text.size();
    const char * data = text.data();
    glShaderSource(handle, 1, &data, &size);
    glCompileShader(handle);
    int ok = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &ok);
    if(!ok)
    {
      int len = 0;
      glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &len);
      std::string log(len, '\0');
      glGetShaderInfoLog(handle, len, &len, log.data());
      log.resize(len + 1);
      glDeleteShader(handle);
      throw std::runtime_error(log);
    }
  }
  ~shader(){
    glDeleteShader(handle);
  }
private:
  unsigned handle;
};

struct program{
  program():handle{}{}
  program(program&& other):handle(std::exchange(other.handle, 0)){}
  program(const program&) = delete;
  operator bool() const{
    return handle;
  }
  template<auto... type>
  program(shader<type>&&... shaders):handle{glCreateProgram()}{
    (glAttachShader(handle, shaders.handle), ...);
    glLinkProgram(handle);
    int ok = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &ok);
    if(!ok){
      int len = 0;
      glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &len);
      std::string log(len, '\0');
      glGetProgramInfoLog(handle, len, &len, log.data()); 
      log.resize(len + 1);
      glDeleteProgram(handle);
      throw std::runtime_error(log);
    }
  }
  auto attrib_loc(const char * name){
    return glGetAttribLocation(handle, name);
  }
  auto uniform_loc(const char * name){
    return glGetUniformLocation(handle, name);
  }
  void bind(){
    glUseProgram(handle);
  }
private:
  unsigned handle;
};

enum class bind_point{
  array = GL_ARRAY_BUFFER,
  atomic_counter = GL_ATOMIC_COUNTER_BUFFER,
  copy_read = GL_COPY_READ_BUFFER,
  copy_write = GL_COPY_WRITE_BUFFER,
  dispatch_indirect = GL_DISPATCH_INDIRECT_BUFFER,
  draw_indirect = GL_DRAW_INDIRECT_BUFFER,
  element_array = GL_ELEMENT_ARRAY_BUFFER,
  pixel_pack = GL_PIXEL_PACK_BUFFER,
  pixel_unpack = GL_PIXEL_UNPACK_BUFFER,
  query = GL_QUERY_BUFFER,
  shader_storage = GL_SHADER_STORAGE_BUFFER,
  texture = GL_TEXTURE_BUFFER,
  transform_feedback = GL_TRANSFORM_FEEDBACK_BUFFER,
  uniform = GL_UNIFORM_BUFFER
};
template<class T>
struct buffer{
  using value_type = T;
  friend class vertex_array;
  buffer():handle{}{}
  buffer(buffer&& other):
    handle(std::exchange(other.handle, {})),
    buffer_size(std::exchange(other.buffer_size, {})),
    mapped_address(std::exchange(other.mapped_address, {}))
  {}
  buffer(const buffer& other) = delete;
  buffer& operator=(const buffer& other) = delete;
  buffer& operator=(buffer&& other){
    handle = std::exchange(other.handle, handle);
    buffer_size = std::exchange(other.buffer_size, buffer_size);
    mapped_address = std::exchange(other.mapped_address, mapped_address);
    return *this;
  }
  operator bool() const{
    return handle;
  }
  buffer(std::initializer_list<T> list):handle{genbuffer()}, buffer_size{list.size()}{
    glNamedBufferData(handle, list.size() * sizeof(T), (void*)std::data(list), GL_STATIC_DRAW);
  }
  ~buffer(){
    glDeleteBuffers(1, &handle);
  }
  T* map(int access){
    if(!mapped_address)
      mapped_address = (T*)glMapNamedBuffer(handle, access);
    return mapped_address;
  }
  void unmap(){
    if(mapped_address)
      glUnmapNamedBuffer(handle);
    mapped_address = nullptr;
  }
  void bind(bind_point p){
    glBindBuffer((int)p, handle);
  }
  void bind(bind_point p, int index){
    if(p == bind_point::array)
      glBindVertexBuffer(index, handle, 0, sizeof(T));
    glBindBufferBase((int)p, index, handle);
  }
  size_t size(){
    return buffer_size;
  }
  void resize(size_t s){
    glNamedBufferData(handle, s * sizeof(T), nullptr, GL_STATIC_DRAW);
    buffer_size = s;
  }
private:
  unsigned handle;
  size_t buffer_size = 0;
  T* mapped_address = nullptr;
  static auto genbuffer(){
    unsigned h;
    glCreateBuffers(1,&h);
    return h;
  }
};
template<class T>
buffer(std::initializer_list<T>)->buffer<T>;

struct renderbuffer{
  renderbuffer():handle{}, buffer_size{}{}
  renderbuffer(const renderbuffer& other) = delete;
  renderbuffer(renderbuffer&& other):
    handle(std::exchange(other.handle, 0)),
    buffer_size(std::exchange(other.buffer_size, {})),
    buffer_format(std::exchange(other.buffer_format, buffer_format)),
    buffer_samples(std::exchange(other.buffer_samples, buffer_samples))
  {}
  renderbuffer& operator=(const renderbuffer& other) = delete;
  renderbuffer& operator=(renderbuffer&& other){
    handle = std::exchange(other.handle, handle);
    buffer_size = std::exchange(other.buffer_size, buffer_size);
    buffer_format = std::exchange(other.buffer_format, buffer_format);
    buffer_samples = std::exchange(other.buffer_samples, buffer_samples);
    return *this;
  }
  operator bool() const{
    return handle;
  }
  renderbuffer(glm::uvec2 size, int format, int samples = 1):
    handle{genbuffer()},
    buffer_size{size},
    buffer_format{format},
    buffer_samples{samples}{
    glNamedRenderbufferStorageMultisample(handle, samples, format, size.x, size.y);
  }
  glm::uvec2 size() const{
    return buffer_size;
  }
  void resize(glm::uvec2 size){
    glNamedRenderbufferStorageMultisample(handle, buffer_samples, buffer_format, size.x, size.y);
    buffer_size = size;
  }
private:
  friend struct framebuffer;
  static unsigned genbuffer(){
    unsigned h;
    glCreateRenderbuffers(1,&h);
    return h;
  }
  unsigned handle;
  glm::uvec2 buffer_size;
  int buffer_format;
  int buffer_samples;
};
struct framebuffer{
  framebuffer():handle{genbuffer()}{}
  framebuffer(const framebuffer& other) = delete;
  framebuffer(framebuffer&& other):
    handle(std::exchange(other.handle, 0))
  {}
  framebuffer& operator=(const framebuffer& other) = delete;
  framebuffer& operator=(framebuffer&& other){
    handle = std::exchange(other.handle, handle);
    return *this;
  }
  operator bool() const{
    return handle;
  }
  void attach(renderbuffer& b, int attachment){
    glNamedFramebufferRenderbuffer(handle, attachment, GL_RENDERBUFFER, b.handle);
  }
  void bind(bool draw, bool read){
    int attachment[]{0, GL_READ_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER};
    glBindFramebuffer(attachment[draw*2+read], handle);
  }
  void draw_on(std::initializer_list<unsigned> attachments){
    glNamedFramebufferDrawBuffers(handle, attachments.size(), data(attachments));
  }
  void read_on(unsigned attachment){
    glNamedFramebufferReadBuffer(handle, attachment);
    read_attachment = attachment;
  }
  auto native()const{
    return handle;
  }
  friend void blit(
    framebuffer& dst,
    ubox2 dst_size,
    framebuffer& src,
    ubox2 src_size,
    bool enable_depth){
    //GLint preserve_draw_fb = 0, preserve_read_fb = 0;
    //glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &preserve_draw_fb);
    //glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &preserve_read_fb);
    
    dst.bind(1, 0);
    src.bind(0, 1);
    //int preserve_read = 0;
    //glGetIntegerv(GL_READ_BUFFER, &preserve_read);
    //int preserve_draw_buffers_size = 0;
    //glGetIntegerv(GL_MAX_DRAW_BUFFERS, &preserve_draw_buffers_size);
    //unsigned preserve_draw_buffers[preserve_draw_buffers_size];
    //for(size_t i = 0; i < preserve_draw_buffers_size; ++i){
    //  glGetIntegerv(GL_DRAW_BUFFER0+i, (int*)(preserve_draw_buffers+i));
    //}
    
    glBlitFramebuffer(
      src_size.min.x,
      src_size.min.y,
      src_size.max.x,
      src_size.max.y,
      dst_size.min.x,
      dst_size.min.y,
      dst_size.max.x,
      dst_size.max.y,
      GL_COLOR_BUFFER_BIT | (enable_depth ? GL_DEPTH_BUFFER_BIT : 0),
      GL_NEAREST
    );
    
    //glReadBuffer(preserve_read);
    //glDrawBuffers(preserve_draw_buffers_size, preserve_draw_buffers);
    //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, preserve_draw_fb);
    //glBindFramebuffer(GL_READ_FRAMEBUFFER, preserve_read_fb);
  }
  template<class buffer_type>
  auto read_pixels(buffer<buffer_type>& buf, ubox2 size, int attachment, int format){
    int prev_buf, prev_fbo;
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prev_buf);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_fbo);
    auto prev_attachment = read_attachment;
    read_on(attachment);
    bind(0,1);
    buf.bind(bind_point::pixel_pack);
    glReadPixels(
        size.min.x,
        size.min.y,
        size.max.x - size.min.x,
        size.max.y - size.min.y,
        format,
        //GL_BYTE,
        detail::gl_type_id<detail::component_type<buffer_type>>,
        0);
    read_on(prev_attachment);
    glBindBuffer((int)bind_point::pixel_pack, prev_buf);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prev_fbo);
  }
  static unsigned genbuffer(){
    unsigned h;
    glCreateFramebuffers(1,&h);
    return h;
  }
  unsigned read_attachment;
  unsigned handle;
};


struct vertex_array{
  vertex_array():handle{}{}
  vertex_array(vertex_array&& other):handle(std::exchange(other.handle, 0)){}
  vertex_array(const vertex_array& other) = delete;
  vertex_array& operator=(const vertex_array& other) = delete;
  vertex_array& operator=(vertex_array&& other){
    handle = std::exchange(other.handle, handle);
    return *this;
  }
  operator bool() const{
    return handle;
  }
  template<class... Ts>
  vertex_array(program& p, const buffer<Ts>&... bufs):handle(genarray()){
    static_assert(((int)std::integral<Ts> + ... + 0) <= 1);
    auto process_buffer = [&, buffer_id=0]<class T>(const buffer<T>& buf){
      using namespace boost::pfr;
      if constexpr(std::is_aggregate_v<T>){
        [&]<size_t... i>(std::index_sequence<i...>){
          auto current_size = 0;
          ([&]{
            using type = tuple_element_t<i, T>;
            constexpr std::string_view name_ = get_name<i, T>();
            const std::string name{name_};
           
            current_size = ((current_size - 1) / alignof(type) + 1) * alignof(type);
            auto loc = p.attrib_loc(name.c_str());
            if(loc == -1) return;
            glEnableVertexArrayAttrib(handle,loc);
            glVertexArrayVertexBuffer(handle, loc, buf.handle, current_size, sizeof(T));
            glVertexArrayAttribFormat(handle, loc, detail::component_count<type>, detail::gl_type_id<detail::component_type<type>>, GL_FALSE, current_size);
          }(), ...);
        }(std::make_index_sequence<tuple_size_v<T>>{});
      }else if constexpr(std::integral<T>){
        glVertexArrayElementBuffer(handle, buf.handle);
      }
    };
    (process_buffer(bufs),...);
  }
  void bind(){
    glBindVertexArray(handle);
  }
  ~vertex_array(){
    glDeleteVertexArrays(1, &handle);
  }
private:
  unsigned handle;
  static unsigned genarray(){
    unsigned h;
    glCreateVertexArrays(1,&h);
    return h;
  }
};

}
