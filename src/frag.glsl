#version 400 compatibility
#extension GL_ARB_gpu_shader_fp64 : enable
#pragma optionNV(fastmath on)
#pragma optionNV(fastprecision on)

#define FLOAT float
#define VEC2 vec2
#define VEC3 vec3
#define AA_LEVEL 1
#define ESCAPE 1000.0
#define PI 3.141592653

#define FLAG_DRAW_MSET ((iFlags & 0x01) == 0x01)
#define FLAG_DRAW_JSET ((iFlags & 0x02) == 0x02)
#define FLAG_USE_COLOR ((iFlags & 0x04) == 0x04)

uniform vec2 iResolution;
uniform vec2 iCam;
uniform vec2 iJulia;
uniform float iZoom;
uniform int iType;
uniform int iIters;
uniform int iFlags;
uniform int iTime;

#define cx_one VEC2(1.0, 0.0)
VEC2 cx_mul(VEC2 a, VEC2 b) {
  return VEC2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}
VEC2 cx_sqr(VEC2 a) {
  FLOAT x2 = a.x*a.x;
  FLOAT y2 = a.y*a.y;
  FLOAT xy = a.x*a.y;
  return VEC2(x2 - y2, xy + xy);
}
VEC2 cx_cube(VEC2 a) {
  FLOAT x2 = a.x*a.x;
  FLOAT y2 = a.y*a.y;
  FLOAT d = x2 - y2;
  return VEC2(a.x*(d - y2 - y2), a.y*(x2 + x2 + d));
}
VEC2 cx_div(VEC2 a, VEC2 b) {
  FLOAT denom = 1.0 / (b.x*b.x + b.y*b.y);
  return VEC2(a.x*b.x + a.y*b.y, a.y*b.x - a.x*b.y) * denom;
}
VEC2 cx_sin(VEC2 a) {
  return VEC2(sin(a.x) * cosh(a.y), cos(a.x) * sinh(a.y));
}
VEC2 cx_cos(VEC2 a) {
  return VEC2(cos(a.x) * cosh(a.y), -sin(a.x) * sinh(a.y));
}
VEC2 cx_exp(VEC2 a) {
  return exp(a.x) * VEC2(cos(a.y), sin(a.y));
}

//Fractal equations
VEC2 mandelbrot(VEC2 z, VEC2 c) {
  return cx_sqr(z) + c;
}
VEC2 burning_ship(VEC2 z, VEC2 c) {
  return VEC2(z.x*z.x - z.y*z.y, 2.0*abs(z.x * z.y)) + c;
}
VEC2 feather(VEC2 z, VEC2 c) {
  return cx_div(cx_cube(z), cx_one + z*z) + c;
}
VEC2 sfx(VEC2 z, VEC2 c) {
  return z * dot(z,z) - cx_mul(z, c*c);
}
VEC2 henon(VEC2 z, VEC2 c) {
  return VEC2(1.0 - c.x*z.x*z.x + z.y, c.y * z.x);
}
VEC2 duffing(VEC2 z, VEC2 c) {
  return VEC2(z.y, -c.y*z.x + c.x*z.y - z.y*z.y*z.y);
}
VEC2 ikeda(VEC2 z, VEC2 c) {
  FLOAT t = 0.4 - 6.0/(1.0 + dot(z,z));
  FLOAT st = sin(t);
  FLOAT ct = cos(t);
  return VEC2(1.0 + c.x*(z.x*ct - z.y*st), c.y*(z.x*st + z.y*ct));
}
VEC2 chirikov(VEC2 z, VEC2 c) {
  z.y += c.y*sin(z.x);
  z.x += c.x*z.y;
  return z;
}

#if 1
#define DO_LOOP(name) \
  for (i = 0; i < iIters; ++i) { \
    VEC2 ppz = pz; \
    pz = z; \
    z = name(z, c); \
    if (dot(z, z) > ESCAPE) { break; } \
    sumz.x += dot(z - pz, pz - ppz); \
    sumz.y += dot(z - pz, z - pz); \
    sumz.z += dot(z - ppz, z - ppz); \
  }
#else
#define DO_LOOP(name) \
  for (i = 0; i < iIters; ++i) { \
    z = name(z, c); \
    if (dot(z, z) > ESCAPE) { break; } \
  }
#endif

vec3 fractal(VEC2 z, VEC2 c) {
  VEC2 pz = z;
  VEC3 sumz = VEC3(0.0, 0.0, 0.0);
  int i;
  switch (iType) {
    case 0: DO_LOOP(mandelbrot); break;
    case 1: DO_LOOP(burning_ship); break;
    case 2: DO_LOOP(feather); break;
    case 3: DO_LOOP(sfx); break;
    case 4: DO_LOOP(henon); break;
    case 5: DO_LOOP(duffing); break;
    case 6: DO_LOOP(ikeda); break;
    case 7: DO_LOOP(chirikov); break;
    case 8:

        FLOAT t = 0.0;
        FLOAT dt = 0.01;

        FLOAT g = 9.81;

        FLOAT _a = 1.0;
        FLOAT _b = 1.0;
        FLOAT _Ma = 1.0;
        FLOAT _Mb = 1.0;
        FLOAT _Mp = 1.0;

        FLOAT _A = ((_Ma / 3.0) + _Mp + _Mb) * _a * _a;
        FLOAT _B = (_Mb * _b * _b) / 3.0;
        FLOAT _C = (_a * _b * _Mb) / 2.0;
        FLOAT _D = ((_Ma / 2.0) + _Mp + _Mb) * g * _a;
        FLOAT _E = (_Mb * g * _b) / 2.0;

        //z = name(z, c);
        FLOAT th1 = c.y * 1.0;
        FLOAT th2 = c.x * 1.0;

        FLOAT th1d = c.y - z.y;
        FLOAT th2d = c.x - z.x;
        FLOAT th1dd = 0;
        FLOAT th2dd = 0;

        FLOAT dth = 0;
        FLOAT cosdth = 0;
        FLOAT sindth = 0;
        FLOAT Ccosdth2 = 0;
        //FLOAT k1, k2, k3, k4;

        FLOAT T = iIters * dt;
        i = 0;

        while (t < T) {
            dth = th1 - th2;
            cosdth = cos(dth);
            sindth = sin(dth);
            Ccosdth2 = _C * _C * cosdth * cosdth;
            th1dd = (-1 * _C * sindth * (_B * th2d * th2d + _C * th1d * th1d * cosdth) - _B * _D * sin(th1) + _C * _E * sin(th2) * cosdth) / (_A * _B - Ccosdth2);
            th2dd = (_C * sindth * (_A * th1d * th1d + _C * th2d * th2d * cosdth) - _A * _E * sin(th2) + _C * _D * sin(th1) * cosdth) / (_A * _B - Ccosdth2);
            // euler integration
            th1d += th1dd * dt;
            th2d += th2dd * dt;
            th1 += th1d * dt;
            th2 += th2d * dt;
            // determine if bottom pendulum has flipped
            if (abs(th2) > PI) {
                break;
            }
            t += dt;
            ++i;

            sumz.x += th1d * th1dd + th2d * th2dd; \
                sumz.y += th1d * th1d + th2d * th2d; \
                sumz.z += th1dd * th1dd + th2dd * th2dd; \
        }

        z = VEC2(th1, th2);
  }

  if (i != iIters) {
    float n1 = sin(float(i) * 0.1) * 0.5 + 0.5;
    float n2 = cos(float(i) * 0.1) * 0.5 + 0.5;
    return vec3(n1, n2, 1.0) * (1.0 - float(FLAG_USE_COLOR)*0.85);
  } else if (FLAG_USE_COLOR) {
    sumz = abs(sumz) / iIters;
    vec3 n1 = sin(abs(sumz * 5.0)) * 0.45 + 0.5;
    return n1;
  } else {
    return vec3(0.0, 0.0, 0.0);
  }
}

float rand(float s) {
  return fract(sin(s*12.9898) * 43758.5453);
}

void main() {
	//Get normalized screen coordinate
	vec2 screen_pos = gl_FragCoord.xy - (iResolution.xy * 0.5);

  vec3 col = vec3(0.0, 0.0, 0.0);
  for (int i = 0; i < AA_LEVEL; ++i) {
    vec2 dxy = vec2(rand(i*0.54321 + iTime), rand(i*0.12345 + iTime));
    VEC2 c = VEC2((screen_pos + dxy) * vec2(1.0, -1.0) / iZoom - iCam);

    if (FLAG_DRAW_MSET) {
      col += fractal(c, c);
    }
    if (FLAG_DRAW_JSET) {
      col += fractal(c, iJulia);
    }
  }

  col /= AA_LEVEL;
  if (FLAG_DRAW_MSET && FLAG_DRAW_JSET) {
    col *= 0.5;
  }
  col = clamp(col, 0.0, 1.0);
  col.x = col.x;
  col.y = 0.9 * col.y;
  col.z = 0.9 * col.z;
  gl_FragColor = vec4(col, 1.0 / (iTime + 1.0));
}
