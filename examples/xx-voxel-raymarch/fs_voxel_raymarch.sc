$input v_texcoord0

/*
 * Copyright 2016 Rasmus Christian Pedersen. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

SAMPLER3D(u_volume, 0);

uniform vec4 u_isovalue;
uniform vec4 u_eye;
uniform vec4 u_lightPos;
uniform vec4 u_boxMin;
uniform vec4 u_boxMax;
uniform vec4 u_texSize;
uniform mat4 u_texMatrix;

//-----------------------------------------------------------------------------
// Ray
//-----------------------------------------------------------------------------
struct Ray {
    vec3 org;
    vec3 dir;
};

//-----------------------------------------------------------------------------
// Box
//-----------------------------------------------------------------------------
struct Box {
    vec3 min;
    vec3 max;
};


bool intersect(in Ray ray, in Box box, out vec2 t)
{
    t.x = 0.0;
    t.y = 65536.0;

    vec3 invr = vec3(1.0) / ray.dir;

    vec3 tbot = invr * (box.min - ray.org);
    vec3 ttop = invr * (box.max - ray.org);

    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);

    vec2 s = max(tmin.xx, tmin.yz);
    t.x    = max(s.x, s.y);

    s      = min(tmax.xx, tmax.yz);
    t.y    = min(s.x, s.y);

    return t.x < t.y;
}

float rand(vec2 p)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(p.xy, vec2(a, b));
    float sn = mod(dt, 3.14);

    return fract(sin(sn) * c);
}

void main()
{
    Ray ray;
    ray.org = vec3(u_eye);
    ray.dir = normalize(v_texcoord0.xyz - ray.org);

    Box box;
    box.min = u_boxMin.xyz;
    box.max = u_boxMax.xyz;

    vec2 t;
    bool isects = intersect(ray, box, t);

    vec3 front = ray.org + t.x * ray.dir;
    vec3 back = ray.org + t.y * ray.dir;

    front += rand(gl_FragCoord.xy) * ray.dir;

    const float spv = 2.0f;

    int noSamples = int(float(t.y - t.x) * spv);
    vec3 rayStep = ray.dir * (1.0 / spv);
    int maxSamples = noSamples > 1024 ? 1024 : noSamples;

    // pre-compute ray front and ray step transformation to texture-space
    vec3 rayStepTex = mul(vec4(rayStep, 1.0), u_texMatrix).xyz;
    vec3 frontTex = mul(vec4(front, 0.0), u_texMatrix).xyz;

    const float slope = 1.0;
    const vec3 skin = vec3(255.0/255.0, 195.0/255.0, 170.0/255.0);

    vec4 invTexSize = vec4(vec3(1.0)/u_texSize.xyz, 0.0);

    vec4 Cdst = vec4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < maxSamples; ++i)
    {
        vec3 samplePos = front + float(i) * rayStep;
        vec3 texPos = mul(vec4(samplePos, 0.0), u_texMatrix).xyz;

        vec3 scalar = slope * texture3D(u_volume, texPos.xyz).xxx;

        if (scalar.x >= u_isovalue.x)
        {
            vec3 sample;
            sample.x = slope * texture3D(u_volume, texPos.xyz + invTexSize.xww).x;
            sample.y = slope * texture3D(u_volume, texPos.xyz + invTexSize.wyw).x;
            sample.z = slope * texture3D(u_volume, texPos.xyz + invTexSize.wwz).x;

            vec3 gradient = normalize(sample - scalar);

            vec3 L =  mul(vec4(samplePos - u_lightPos.xyz, 1.0), u_texMatrix).xyz;
            float ld = abs(dot(gradient, L));

            Cdst.rgb = ld * skin;
            break;
        }

    }

    gl_FragColor = Cdst;
}
