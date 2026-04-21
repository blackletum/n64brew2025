#ifndef __ENTITIES_ROBOT_WHISTLE_H__
#define __ENTITIES_ROBOT_WHISTLE_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"

struct robot_whistle {
    vector3_t position;
    wav64_t* sound;
};

typedef struct robot_whistle robot_whistle_t;

void robot_whistle_init(robot_whistle_t* robot_whistle, struct robot_whistle_definition* definition, entity_id entity_id);
void robot_whistle_destroy(robot_whistle_t* robot_whistle, struct robot_whistle_definition* definition);
void robot_whistle_common_init();
void robot_whistle_common_destroy();

#endif