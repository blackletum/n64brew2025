#include "overworld_music.h"

#include "../audio/audio.h"
#include "../cutscene/race.h"
#include "../time/time.h"
#include "../savefile/savefile.h"
#include "../util/text.h"

static vector2_t settlement_loc = {
    179.4, 113.6,
};

#define SETTLEMENT_RADIUS   140.0f

static const char* music_filenames[OVERWORLD_SONG_COUNT] = {
    [OVERWORLD_SONG_AWAKENGING_TO_SILENCE] = "rom:/sounds/music/awakening_to_silence.wav64",
    [OVERWORLD_SONG_DESERT_DAYDREAMS] = "rom:/sounds/music/desert_daydreams.wav64",
    [OVERWORLD_SONG_DESERT_STRING] = "rom:/sounds/music/desert_strings.wav64",
    [OVERWORLD_SONG_RACE] = "rom:/sounds/music/race.wav64",
    [OVERWORLD_SONG_MEMORIES_INDOOR] = "rom:/sounds/music/memories_of_the_oldtimes_indoors.wav64",
    [OVERWORLD_SONG_MEMORIES_OUTDOOR] = "rom:/sounds/music/memories_of_the_oldtimes_outdoors.wav64",
    [OVERWORLD_SONG_INDOOR_AMBIENCE] = "rom:/sounds/music/science_lab_ambience.wav64",
    [OVERWORLD_SONG_STORE_THEME] = "rom:/sounds/music/store_theme_radio.wav64",
    [OVERWORLD_SONG_MAIN_MENU] = "rom:/sounds/music/menu_music.wav64",
    [OVERWORLD_SONG_GARAGE] = "rom:/sounds/music/garage.wav64",
};

void overworld_music_init(overworld_music_t* music, const char* scene_filename) {
    for (int i = 0; i < OVERWORLD_SONG_COUNT; i += 1) {
        music->songs[i] = wav64_load(music_filenames[i], NULL);
    }

    music->is_racing = race_get_state() == RACE_STATE_STARTED;
    music->did_race_start = false;

    music->single_scene_song = OVERWORLD_SONG_COUNT;
    
    if (str_startswith(scene_filename, "rom:/scenes/inside_house.scene")) {
        music->single_scene_song = OVERWORLD_SONG_MEMORIES_INDOOR;
    }

    if (str_startswith(scene_filename, "rom:/scenes/inside_lab.scene")) {
        music->single_scene_song = OVERWORLD_SONG_INDOOR_AMBIENCE;
    }
    
    if (str_startswith(scene_filename, "rom:/scenes/inside_boat.scene")) {
        music->single_scene_song = OVERWORLD_SONG_INDOOR_AMBIENCE;
    }
    
    if (str_startswith(scene_filename, "rom:/scenes/store.scene")) {
        music->single_scene_song = OVERWORLD_SONG_STORE_THEME;
    }
    
    if (str_startswith(scene_filename, "rom:/scenes/garage.scene")) {
        music->single_scene_song = OVERWORLD_SONG_GARAGE;
    }
    
    if (str_startswith(scene_filename, "rom:/scenes/overworld.scene#main_menu")) {
        music->single_scene_song = OVERWORLD_SONG_MAIN_MENU;
    }
}

wav64_t* overworld_music_determine_song(overworld_music_t* music, vector3_t* player_pos) {
    if (music->single_scene_song != OVERWORLD_SONG_COUNT) {
        return music->songs[music->single_scene_song];
    }

    if (music->is_racing) {
        if (update_has_layer(UPDATE_LAYER_WORLD)) {
            music->did_race_start = true;
        }

        return music->did_race_start ? music->songs[OVERWORLD_SONG_RACE] : NULL;
    }

    vector2_t offset = {
        player_pos->x - settlement_loc.x,
        player_pos->z - settlement_loc.y,
    };
    
    if (vector2MagSqr(&offset) < SETTLEMENT_RADIUS * SETTLEMENT_RADIUS) {
        return music->songs[OVERWORLD_SONG_MEMORIES_OUTDOOR];
    }

    if (offset.y < 0.0f) {
        return music->songs[OVERWORLD_SONG_DESERT_DAYDREAMS];
    }

    if (offset.x < 0.0f) {
        return music->songs[OVERWORLD_SONG_AWAKENGING_TO_SILENCE];
    }

    return music->songs[OVERWORLD_SONG_DESERT_STRING];
}

void overworld_music_update(overworld_music_t* music, vector3_t* player_pos) {
    audio_play_music(overworld_music_determine_song(music, player_pos));
}

void overworld_music_destroy(overworld_music_t* music) {
    audio_play_music(NULL);
     for (int i = 0; i < OVERWORLD_SONG_COUNT; i += 1) {
        wav64_close(music->songs[i]);
     }
}