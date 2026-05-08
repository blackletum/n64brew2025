#include "main_menu.h"

#include <libdragon.h>
#include "../render/material.h"
#include "../resource/material_cache.h"
#include "menu_rendering.h"
#include "menu_common.h"
#include "../time/time.h"
#include "../fonts/fonts.h"
#include "../scene/scene.h"
#include "../render/defs.h"
#include "../effects/fade_effect.h"
#include "../savefile/savefile.h"
#include "../menu/map_menu.h"

enum main_menu_sound_effects {
    MAIN_MENU_SOUND_CURSOR,
    MAIN_MENU_SOUND_SELECT,

    MAIN_MENU_SOUND_COUNT,
};

struct main_menu {
    bool is_showing;
    sprite_t* title;
    sprite_t* title_band;
    material_t* title_material;
    
    wav64_t* sounds[MAIN_MENU_SOUND_COUNT];
    
    int selected_item;
    float go_timer;
    float intro_timer;
};

static struct main_menu main_menu;

#define TITLE_Y         24
#define BAND_OFFSET     24
#define TITLE_X         40

#define TEXT_X              120
#define TEXT_LINE_0_Y       130
#define TEXT_LINE_SPACING   20

#define SLIDE_IN_TIME       0.25f
#define FLASH_TIME          0.5f

#define INTRO_TIME          (SLIDE_IN_TIME + FLASH_TIME)

bool main_menu_enable_scene_saving;

void main_menu_render(void* data) {
    material_apply(main_menu.title_material);

    int flash_alpha = 0;

    if (main_menu.intro_timer > FLASH_TIME) {
        rdpq_set_prim_color((color_t){255, 255, 255, 255});
    } else if (main_menu.intro_timer > 0.0f) {
        flash_alpha = ((int)((255.0f / FLASH_TIME) * main_menu.intro_timer));
        rdpq_set_prim_color((color_t){flash_alpha, flash_alpha, flash_alpha, 255});
    }

    float slide_amount = 0.0f;

    if (main_menu.intro_timer > FLASH_TIME) {
        slide_amount = (main_menu.intro_timer - FLASH_TIME) * (1.0f / SLIDE_IN_TIME);
    }

    rdpq_blitparms_t band_params = (rdpq_blitparms_t){
        .scale_y = main_menu.intro_timer > FLASH_TIME ? (1.0f - slide_amount) : 1.0f,
    };
    if (slide_amount < 0.99f) {
        rdpq_sprite_blit(main_menu.title_band, 0, TITLE_Y + BAND_OFFSET, &band_params);
    }
    rdpq_sprite_blit(main_menu.title, TITLE_X + slide_amount * SCREEN_WD, TITLE_Y, NULL);

    if (main_menu.intro_timer > FLASH_TIME) {
        return;
    }

    if (flash_alpha) {
        material_apply(solid_primitive_material);
        rdpq_set_prim_color((color_t){255, 255, 255, flash_alpha});
        
        rdpq_texture_rectangle(TILE0, 0, 0, SCREEN_WD, SCREEN_HT, 0, 0);
    }

    menu_common_render_background(100, 120, 120, 60);

    if (savefile_has_save()) {
        rdpq_text_printn(&(rdpq_textparms_t){
                .align = ALIGN_LEFT,
                .valign = VALIGN_TOP,
                .width = 128,
                .height = 40,
                .wrap = WRAP_NONE,
            }, 
            FONT_DIALOG, 
            TEXT_X, TEXT_LINE_0_Y, 
            "Continue",
            strlen("Continue")
        );
    }
    
    rdpq_text_printn(&(rdpq_textparms_t){
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .width = 128,
            .height = 40,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        TEXT_X, TEXT_LINE_0_Y + (savefile_has_save() ? TEXT_LINE_SPACING : 0), 
        "New game",
        strlen("New game")
    );


    material_apply(menu_icons_material);

    rdpq_texture_rectangle(
        TILE0,
        TEXT_X - 16, TEXT_LINE_0_Y + main_menu.selected_item * TEXT_LINE_SPACING,
        TEXT_X, TEXT_LINE_0_Y + main_menu.selected_item * TEXT_LINE_SPACING + 16,
        32, 0
    );
}

void main_menu_update(void *data) {
    static int prev_y;

    joypad_inputs_t inputs = joypad_get_inputs(0);

    if (savefile_has_save()) {
        if ((inputs.stick_y > 40 && prev_y <= 40) || (inputs.stick_y < -40 && prev_y >= -40)) {
            main_menu.selected_item = 1 - main_menu.selected_item;
            audio_play_2d(main_menu.sounds[MAIN_MENU_SOUND_CURSOR], 1.0f, 0.0f, 1.0f, 1);
        }
    }

    if (inputs.btn.l && inputs.btn.z && get_tv_type() == TV_PAL) {
        vi_set_timing_preset(&VI_TIMING_PAL60);
        update_set_fixed_time_step(DEFAULT_TIME_STEP);
    } else if (inputs.btn.r && inputs.btn.z && get_tv_type() == TV_PAL) {
        vi_set_timing_preset(&VI_TIMING_PAL);
        update_set_fixed_time_step(DEFAULT_PAL_TIME_STEP);
    }

    prev_y = inputs.stick_y;

    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.a) {
        fade_effect_set((color_t){0, 0, 0, 255}, 1.0f);
        audio_play_2d(main_menu.sounds[MAIN_MENU_SOUND_SELECT], 1.0f, 0.0f, 1.0f, 1);
        main_menu.go_timer = 1.0f;
    }

    if (main_menu.go_timer > 0.0f) {
        main_menu.go_timer -= fixed_time_step;

        if (main_menu.go_timer < 0.0f) {
            main_menu_hide();
            update_unpause_layers(UPDATE_LAYER_WORLD);
            if (main_menu.selected_item == 0 && savefile_has_save()) {
                scene_queue_next(savefile_get_last_scene());
            } else {
                savefile_new();
                scene_queue_next("rom:/scenes/intro.scene#default");
            }
            map_menu_update_has_prev();
            main_menu_enable_scene_saving = true;
        }
    }

    if (main_menu.intro_timer > 0.0f) {
        main_menu.intro_timer -= fixed_time_step;
    }
}


static const char* menu_sound_files[MAIN_MENU_SOUND_COUNT] = {
    [MAIN_MENU_SOUND_CURSOR] = "rom:/sounds/menu/cursor.wav64",
    [MAIN_MENU_SOUND_SELECT] = "rom:/sounds/menu/pause.wav64",
};

void main_menu_show() {
    if (main_menu.is_showing) {
        return;
    }
    main_menu.is_showing = true;
    main_menu.selected_item = 0;
    main_menu.go_timer = 0.0f;
    main_menu.intro_timer = INTRO_TIME;

    menu_add_callback(main_menu_render, &main_menu, MENU_PRIORITY_MENU);
    main_menu.title = sprite_load("rom:/images/menu/game_title.sprite");
    main_menu.title_band = sprite_load("rom:/images/menu/game_title_band.sprite");
    main_menu.title_material = material_cache_load("rom:/materials/menu/map_icon.mat");

    font_type_use(FONT_DIALOG);
    update_add(&main_menu, main_menu_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_CUTSCENE | UPDATE_LAYER_PAUSE_MENU);
    
    for (int i = 0; i < MAIN_MENU_SOUND_COUNT; i += 1) {
        main_menu.sounds[i] = wav64_load(menu_sound_files[i], NULL);
    }
}

void main_menu_hide() {
    if (!main_menu.is_showing) {
        return;
    }
    
    for (int i = 0; i < MAIN_MENU_SOUND_COUNT; i += 1) {
        wav64_close(main_menu.sounds[i]);
    }

    main_menu.is_showing = false;
    menu_remove_callback(&main_menu);

    sprite_free(main_menu.title);
    sprite_free(main_menu.title_band);
    material_cache_release(main_menu.title_material);
    update_remove(&main_menu);

    font_type_release(FONT_DIALOG);
}
