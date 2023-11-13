#version 400
uniform vec2 size;
uniform float scale;
in vec3 world_pos;
in vec3 grid_local_pos;
in vec3 camera_local_pos;
out vec4 frag_color;
float sample_grid(vec2 pos, float line_width){
  vec2 line_color = abs(fract(pos+0.5)-0.5);
  vec2 line_alpha = clamp(line_width - line_color / fwidth(pos), 0, 1);
  return max(line_alpha.x, line_alpha.y);
}
void main(){
  float logscale = log(scale)/log(10);
  float sweep = pow(fract(logscale), log(10));
  float big_grid = sample_grid(grid_local_pos.xz*0.01, 3);
  float mid_grid = sample_grid(grid_local_pos.xz*0.1, 3*sweep+(1-sweep));
  float small_grid = sweep*sample_grid(grid_local_pos.xz, 1);
  float total_grid = max(max(big_grid, mid_grid), small_grid);

  if(total_grid < 0.001)
    discard;
  float bright = min(1, 40/(dot(camera_local_pos, camera_local_pos)));
  frag_color = vec4(vec3(1), bright*total_grid);
}
