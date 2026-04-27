#include "robot_whistle.h"    

#include "../audio/audio.h"

static const char* robot_whistle_sounds[] = {
    [ROBOT_WHISTLE_ROBOT] = "rom:/sounds/music/store_theme_whistle.wav64",
    [ROBOT_WHISTLE_WATERFALL] = "rom:/sounds/objects/waterfall.wav64",
};
    
void robot_whistle_init(robot_whistle_t* robot_whistle, struct robot_whistle_definition* definition, entity_id entity_id) {
    robot_whistle->position = definition->position;
    robot_whistle->sound = wav64_load(robot_whistle_sounds[definition->sound], NULL);

    audio_id id = audio_play_3d(robot_whistle->sound, definition->volume, &robot_whistle->position, &gZeroVec, 1.0f, 1);

    if (definition->disable_doppler) {
        audio_disable_doppler(id);
    }
}

void robot_whistle_destroy(robot_whistle_t* robot_whistle, struct robot_whistle_definition* definition) {
    wav64_close(robot_whistle->sound);
}

void robot_whistle_common_init() {

}

void robot_whistle_common_destroy() {

}
