/*
logo_t3d under the same licensing as the rest of the code
logo_libdragon and logo_n64brew sourced from the N64brew-GameJam2024 repository

Copyright (c) 2024 N64brew

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmodel.h>

#define SPINSPEED 0.9f

// Easing function for quadratic ease-out
//  t = current time
//  b = start value
//  c = change in value
//  d = duration
static float ease_out_quad(float t, float b, float c, float d) {
  t /= d;
  if (t>1) t = 1;
  return -c * t*(t-2) + b;
}

void logo_libdragon() {
  const color_t RED = RGBA32(221, 46, 26, 255);
  const color_t WHITE = RGBA32(255, 255, 255, 255);

  sprite_t *d1 = sprite_load("rom:/images/intro/dragon1.sprite");
  sprite_t *d2 = sprite_load("rom:/images/intro/dragon2.sprite");
  sprite_t *d3 = sprite_load("rom:/images/intro/dragon3.sprite");
  sprite_t *d4 = sprite_load("rom:/images/intro/dragon4.sprite");
  wav64_t music;
  wav64_open(&music, "rom:/sounds/menu/dragon.wav64");

  display_init(RESOLUTION_640x480,
      DEPTH_16_BPP,
      2,
      GAMMA_NONE,
      ANTIALIAS_RESAMPLE);

  float angle1=0, angle2=0, angle3=0;
  float scale1=0, scale2=0, scale3=0, scroll4=0;
  uint32_t ms0=0;
  int anim_part=0;
  // translation offset of the animation (simplify centering)
  const int X0 = 10, Y0 = 30;

  void reset() {
    ms0 = get_ticks_ms();
    anim_part = 0;

    angle1 = 3.2f;
    angle2 = 1.9f;
    angle3 = 0.9f;
    scale1 = 0.0f;
    scale2 = 0.4f;
    scale3 = 0.8f;
    scroll4 = 400;
    wav64_play(&music, 0);
  }

  reset();
  while (1) {
    mixer_try_play();

    // Calculate animation part:
    // 0: rotate dragon head
    // 1: rotate dragon body and tail, scale up
    // 2: scroll dragon logo
    // 3: fade out
    uint32_t tt = get_ticks_ms() - ms0;
    if (tt < 1000) anim_part = 0;
    else if (tt < 1500) anim_part = 1;
    else if (tt < 4000) anim_part = 2;
    else if (tt < 5000) anim_part = 3;
    else break;

    // Update animation parameters using quadratic ease-out
    angle1 -= angle1 * 0.04f; if (angle1 < 0.010f) angle1 = 0;
    if (anim_part >= 1) {
      angle2 -= angle2 * 0.06f; if (angle2 < 0.01f) angle2 = 0;
      angle3 -= angle3 * 0.06f; if (angle3 < 0.01f) angle3 = 0;
      scale2 -= scale2 * 0.06f; if (scale2 < 0.01f) scale2 = 0;
      scale3 -= scale3 * 0.06f; if (scale3 < 0.01f) scale3 = 0;
    }
    if (anim_part >= 2) {
      scroll4 -= scroll4 * 0.08f;
    }

    // Update colors for fade out effect
    color_t red = RED;
    color_t white = WHITE;
    if (anim_part >= 3) {
      red.a = 255 - (tt-4000) * 255 / 1000;
      white.a = 255 - (tt-4000) * 255 / 1000;
    }

    surface_t *fb = display_get();
    rdpq_attach_clear(fb, NULL);

    // To simulate the dragon jumping out, we scissor the head so that
    // it appears as it moves.
    if (angle1 > 1.0f) {
      // Initially, also scissor horizontally, 
      // so that the head tail is not visible on the right.
      rdpq_set_scissor(0, 0, X0+300, Y0+240);  
    } else {
      rdpq_set_scissor(0, 0, 640, Y0+240);
    }

    // Draw dragon head
    rdpq_set_mode_standard();
    rdpq_mode_alphacompare(1);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(TEX0,0,PRIM,0)));
    rdpq_set_prim_color(red);
    rdpq_sprite_blit(d1, X0+216, Y0+205, &(rdpq_blitparms_t){
      .theta = angle1, .scale_x = scale1+1, .scale_y = scale1+1,
      .cx = 176, .cy = 171,
    });

    // Restore scissor to standard
    rdpq_set_scissor(0, 0, 640, 480);

    // Draw a black rectangle with alpha gradient, to cover the head tail
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rdpq_mode_dithering(DITHER_NOISE_NOISE);
    float vtx[4][6] = {
      //  x,    y,  r,g,b,a
      { X0+0,   Y0+180, 0,0,0,0 },
      { X0+200, Y0+180, 0,0,0,0 },
      { X0+200, Y0+240, 0,0,0,1 },
      { X0+0,   Y0+240, 0,0,0,1 },
    };
    rdpq_triangle(&TRIFMT_SHADE, vtx[0], vtx[1], vtx[2]);
    rdpq_triangle(&TRIFMT_SHADE, vtx[0], vtx[2], vtx[3]);

    if (anim_part >= 1) {
      // Draw dragon body and tail
      rdpq_set_mode_standard();
      rdpq_mode_alphacompare(1);
      rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
      rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(TEX0,0,PRIM,0)));

      // Fade them in
      color_t color = red;
      color.r *= 1-scale3; color.g *= 1-scale3; color.b *= 1-scale3;
      rdpq_set_prim_color(color);

      rdpq_sprite_blit(d2, X0+246, Y0+230, &(rdpq_blitparms_t){ 
        .theta = angle2, .scale_x = 1-scale2, .scale_y = 1-scale2,
        .cx = 145, .cy = 113,
      });

      rdpq_sprite_blit(d3, X0+266, Y0+256, &(rdpq_blitparms_t){ 
        .theta = -angle3, .scale_x = 1-scale3, .scale_y = 1-scale3,
        .cx = 91, .cy = 24,
      });
    }

    // Draw scrolling logo
    if (anim_part >= 2) {
      rdpq_set_prim_color(white);
      rdpq_sprite_blit(d4, X0 + 161 + (int)scroll4, Y0 + 182, NULL);
    }

    rdpq_detach_show();
  }

  wait_ms(500); // avoid immediate switch to next screen
  rspq_wait();
  sprite_free(d1);
  sprite_free(d2);
  sprite_free(d3);
  sprite_free(d4);
  wav64_close(&music);
  display_close();
}

void logo_t3d() {
  display_init(RESOLUTION_640x480,
      DEPTH_16_BPP,
      2,
      GAMMA_NONE,
      FILTERS_DISABLED);
  sprite_t *logo = sprite_load("rom:/images/intro/t3d-logo.sprite");

  rdpq_set_mode_standard();
  rdpq_attach_clear(display_get(), NULL);
  rdpq_sprite_blit(logo, 55, 137, NULL);
  rdpq_detach_show();

  wait_ms(3000);
  rspq_wait();
  sprite_free(logo);
  display_close();
}