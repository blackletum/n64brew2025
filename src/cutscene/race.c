#include "race.h"

#include <string.h>
#include "../scene/scene.h"
#include "cutscene.h"
#include "cutscene_runner.h"
#include "../player/inventory.h"
#include "expression_evaluate.h"
#include "cutscene_stopwatch.h"
#include "../render/render_scene.h"
#include "../scene/scene.h"
#include "../util/cleanup.h"

static char g_completion_script[64];
static int race_progress;
static enum race_state current_state;
static int laps_left = 0;
static vector3_t next_checkpoint_position;
static bool has_next_checkpoint;
static tmesh_t next_checkpoint_arrow;
static vector3_t relative_checkpoint_arrow_pos = {0.0f, 1.2f, -3.0f};

static const char* completion_messages[] = {
    [RACE_STATE_NOT_STARTED] = "",
    [RACE_STATE_STARTED] = "",
    [RACE_STATE_FINISH] = "Finish!",
    [RACE_STATE_MISS_CHECKPOINT] = "Missed checkpoint",
    [RACE_STATE_ABANDON] = "Race abandoned",
};

void race_render_next_checkpoint(void* data, struct render_batch* batch) {
    if (!has_next_checkpoint) {
        return;
    }

    vector4_t pos_4;
    matrixVec3Mul(batch->camera_matrix, &relative_checkpoint_arrow_pos, &pos_4);
    
    transform_sa_t transform;
    transform.position = (vector3_t){pos_4.x, pos_4.y, pos_4.z};

    vector3_t next_offset;
    vector3Sub(&next_checkpoint_position, &transform.position, &next_offset);
    vector2LookDir(&transform.rotation, &next_offset);

    transform.scale = 1.0f;

    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &transform);

    if (!mtx) {
        return;
    }

    render_batch_add_tmesh(batch, &next_checkpoint_arrow, mtx, NULL, NULL, NULL);
}

void race_setup_laps(void *data) {
    cutscene_stop_watch_set_lap_count(laps_left);
}

void race_start(const char* completion_script, int lap_count) {
    race_progress = 0;
    current_state = RACE_STATE_STARTED;
    laps_left = lap_count;
    strcpy(g_completion_script, completion_script);
    render_scene_add(NULL, 0.0f, race_render_next_checkpoint, &next_checkpoint_position);
    tmesh_load_filename(&next_checkpoint_arrow, "rom:/meshes/effects/next_checkpoint.tmesh");

    cutscene_builder_t cutscene;
    cutscene_builder_init(&cutscene);

    cutscene_builder_fade(&cutscene, FADE_COLOR_BLACK, 0.5f);
    cutscene_builder_delay(&cutscene, 0.5f);
    cutscene_builder_set_boolean(&cutscene, inventory_get_item_ref(ITEM_RIDING_MOTORCYCLE), true);
    cutscene_builder_load_scene(&cutscene, completion_script);
    cutscene_builder_callback(&cutscene, race_setup_laps, NULL);
    cutscene_builder_delay(&cutscene, 0.5f);
    cutscene_builder_camera_follow_vehicle(&cutscene);
    cutscene_builder_fade(&cutscene, FADE_COLOR_NONE, 0.5f);

    cutscene_runner_run(cutscene_builder_finish(&cutscene), 0, cutscene_runner_free_on_finish(), NULL, 0);
}

void race_trigger_end(enum race_state state) {
    if (current_state != RACE_STATE_STARTED) {
        return;
    }
    
    cutscene_stopwatch_set_running(false);
    render_scene_remove(&next_checkpoint_position);
    cleanup_safe((cleanup_callback)tmesh_release, &next_checkpoint_arrow);
    
    current_state = state;
    laps_left = 0;
    has_next_checkpoint = false;
    cutscene_builder_t cutscene;
    cutscene_builder_init(&cutscene);

    cutscene_builder_dialog(&cutscene, completion_messages[state]);
    cutscene_builder_fade(&cutscene, FADE_COLOR_BLACK, 0.5f);
    cutscene_builder_delay(&cutscene, 0.5f);
    cutscene_builder_set_boolean(&cutscene, inventory_get_item_ref(ITEM_RIDING_MOTORCYCLE), false);
    cutscene_builder_load_scene(&cutscene, g_completion_script);
    cutscene_builder_callback(&cutscene, race_setup_laps, NULL);
    cutscene_builder_delay(&cutscene, 0.5f);
    cutscene_builder_fade(&cutscene, FADE_COLOR_NONE, 0.5f);

    cutscene_runner_run(cutscene_builder_finish(&cutscene), 0, cutscene_runner_free_on_finish(), NULL, 0);
}

bool race_trigger_checkpoint(int index, bool is_finish) {
    if (current_state != RACE_STATE_STARTED || index == race_progress || (is_finish && race_progress == 0)) {
        return false;
    }

    if (race_progress + 1 == index) {
        race_progress = index;

        if (is_finish) {
            laps_left -= 1;
            cutscene_stop_watch_next_lap();

            if (laps_left) {
                race_progress = 0;
            } else {
                race_trigger_end(RACE_STATE_FINISH);
            }
        }

        return true;
    }

    race_trigger_end(RACE_STATE_MISS_CHECKPOINT);

    return false;
}

enum race_state race_get_state() {
    return current_state;
}

void race_set_next_checkpoint(vector3_t* pos) {
    next_checkpoint_position = *pos;
    has_next_checkpoint = true;
}