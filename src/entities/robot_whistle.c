#include "robot_whistle.h"    

#include "../audio/audio.h"
    
void robot_whistle_init(robot_whistle_t* robot_whistle, struct robot_whistle_definition* definition, entity_id entity_id) {
    robot_whistle->position = definition->position;
    robot_whistle->sound = wav64_load("rom:/sounds/music/store_theme_whistle.wav64", NULL);

    audio_play_3d(robot_whistle->sound, 2.0f, &robot_whistle->position, &gZeroVec, 1.0f, 1);
}

void robot_whistle_destroy(robot_whistle_t* robot_whistle, struct robot_whistle_definition* definition) {
    wav64_close(robot_whistle->sound);
}

void robot_whistle_common_init() {

}

void robot_whistle_common_destroy() {

}
