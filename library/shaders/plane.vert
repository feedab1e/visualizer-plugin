#version 400
uniform mat4 mvp;
uniform vec3 offset;
uniform float scale;
out vec3 world_pos;
out vec3 grid_local_pos;
out vec3 camera_local_pos;

const vec3 vertices[4] = vec3[4](
  vec3(-100, 0, -100),
  vec3( 100, 0, -100),
  vec3(-100, 0,  100),
  vec3( 100, 0,  100)
);

void main(){
  float logscale = log(scale)/log(10);
  float step = pow(10, floor(logscale));
  float rem = pow(10, fract(logscale));
  float sweep = pow(fract(logscale), log(10));

  vec3 true_pos = vertices[gl_VertexID]/scale + offset;
  world_pos = true_pos;
  camera_local_pos = vertices[gl_VertexID];

  grid_local_pos = vertices[gl_VertexID]/rem*10 + 100*fract(offset/10*step);
  gl_Position = mvp * vec4(true_pos, 1.0);
}
