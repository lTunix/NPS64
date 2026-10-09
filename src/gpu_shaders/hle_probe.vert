#version 450
layout(push_constant) uniform Draw { vec4 color; float depth; uint textured; uint wrap; uint padding; vec4 rect; vec4 prim; vec4 env; uvec4 mux; vec4 sampling; } draw;
layout(location=0) in vec4 position;
layout(location=1) in vec4 vertex_color;
layout(location=2) in vec2 vertex_uv;
layout(location=0) noperspective out vec4 color;
layout(location=1) noperspective out vec3 uv_over_w;
layout(location=2) flat out uint textured;
layout(location=3) flat out vec4 rect;
layout(location=4) flat out uint wrap;
void main() {
    /* RDP vertices already carry authoritative screen coordinates and depth.
     * Rasterize in that domain; keep reciprocal W only for texture gradients. */
    float inverse_w=1.0/position.w;
    gl_Position = vec4(position.xyz*inverse_w,1.0);
    gl_Position.z += draw.depth;
    color = vertex_color * draw.color;
    uv_over_w = vec3(vertex_uv*inverse_w,inverse_w);
    textured = draw.textured;
    rect = draw.rect; wrap = draw.wrap;
}
