#version 330 core

in vec2  frag_uv;
in float frag_ao;
in float frag_light;
in float diffuse;
in float fog_height;
in float fog_factor;

uniform sampler2D sampler;
uniform sampler2D sky_sampler;
uniform float timer;
uniform float daylight;
uniform bool ortho;

out vec4 FragColor;

const float pi = 3.14159265;

void main()
{
    vec3 color = texture(sampler, frag_uv).rgb;
    if(color == vec3(1.0, 0.0, 1.0)) {
	discard;
    }
    bool cloud = color == vec3(1.0);
    if(cloud && ortho) {
	discard;
    }
    float df = cloud ? 1.0 - diffuse * 0.2 : diffuse;
    float ao = cloud ? 1.0 - (1.0 - frag_ao) * 0.2 : frag_ao;
    df = min(1.0, df + frag_light);	    
    ao = min(1.0, ao + frag_light);
    float intensity = min(1.0, daylight + frag_light);
    vec3 ambient = vec3(intensity * 0.3 + 0.2);
    vec3 light_color = vec3(intensity * 0.3 + 0.2);
    vec3 light = ambient + light_color* df;
    color = clamp(color * light * ao, vec3(0.0), vec3(1.0));
    vec3 sky_color = texture(sky_sampler, vec2(timer, fog_height)).rgb;
    color = mix(color, sky_color, fog_factor);
    FragColor = vec4(color, 1.0);
}
