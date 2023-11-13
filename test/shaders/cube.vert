#version 330
attribute vec3 pos;
uniform mat4 mvp;
out vec3 pos2;
void main(){
  gl_Position = mvp * vec4(pos, 1.0);
  pos2 = pos;
}
