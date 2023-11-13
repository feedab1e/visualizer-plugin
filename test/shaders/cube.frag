#version 330
uniform vec2 size;
in vec3 pos2;
out vec4 frag_color;
void main()
{
  frag_color = vec4((pos2 + vec3(1))/2, 1.0);
}
