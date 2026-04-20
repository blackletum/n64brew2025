#ifndef __CUTSCENE_RACE_H__
#define __CUTSCENE_RACE_H__

#include "../scene/scene_definition.h"
#include <stdbool.h>

void race_start(const char* completion_script, int lap_count);
void race_trigger_end(enum race_state state);
bool race_trigger_checkpoint(int index, bool is_finish);
enum race_state race_get_state();

void race_set_next_checkpoint(vector3_t* pos);

#endif