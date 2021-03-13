#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 uv;

uniform mat4 matrix;
uniform vec3 camera;
uniform bool ortho;
uniform float fog_distance;

out vec2  frag_uv;
out float frag_ao;
out float frag_light;
out float diffuse;
out float fog_height;
out float fog_factor;

const float pi = 3.14159265;
const vec3 light_dir = normalize(vec3(-1.0, 1.0, -1.0));

void main()
{
    gl_Position = matrix * vec4(position, 1.0);
    frag_uv = uv.xy;
    frag_ao = 0.3 + (1.0 - uv.z) * 0.7;
    frag_light = uv.w;
    diffuse = max(0.0, dot(normal, light_dir));
    if(ortho) {
	fog_factor = 0.0;
	fog_height = 0.0;
    }
    else {
	// float camera_distance = distance(camera, position);
	// fog_factor = pow(clamp(camera_distance / fog_distance, 0.0, 1.0), 4.0);
	// float dy = position.y - camera.y;
	// float dx = distance(position.xz, camera.xz);
	// fog_height = (atan(dy, dx) + pi / 2) / pi;
    }
}
