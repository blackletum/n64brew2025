#include "checkpoint.h"    
    
#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../cutscene/expression_evaluate.h"
#include "../time/time.h"
#include "../audio/audio.h"
#include "../cutscene/race.h"
#include "../cutscene/cutscene_stopwatch.h"

struct checkpoint_assets
{
    wav64_t* checkpoint_sound;
};

static struct checkpoint_assets assets;

static spatial_trigger_type_t trigger_type = {
    .type = SPATIAL_TRIGGER_BOX,
    .data = {
        .box = {
            .half_size = {5.75f, 3.0f, 1.5f},
        }
    },
    .center = {0.0f, 2.0f, 0.0f},
};

static const char* checkpoint_meshes[CHECKPOINT_TYPE_COUNT] = {
    [CHECKPOINT_FINISH] = "rom:/meshes/objects/race_start+finish.tmesh",
    [CHECKPOINT_MIDDLE] = "rom:/meshes/objects/race_checkpoint.tmesh",
};

static const char* checkpoint_meshes_off[CHECKPOINT_TYPE_COUNT] = {
    [CHECKPOINT_FINISH] = "rom:/meshes/objects/race_start+finish.tmesh",
    [CHECKPOINT_MIDDLE] = "rom:/meshes/objects/race_checkpoint_off.tmesh",
};

void checkpoint_update(void* data) {
    checkpoint_t* checkpoint = (checkpoint_t*)data;

    if (race_get_state() != RACE_STATE_STARTED) {
        return;
    }

    if (contacts_are_touching(checkpoint->trigger.active_contacts, ENTITY_ID_MOTORCYLE)) {
        if (race_trigger_checkpoint(checkpoint->checkpoint_index, checkpoint->is_finish)) {
            audio_play_2d(assets.checkpoint_sound, 1.0f, 0.0f, 1.0f, 1);
            race_set_next_checkpoint(&checkpoint->next_checkpoint);
        }
    }
}

void checkpoint_init(checkpoint_t* checkpoint, struct checkpoint_definition* definition, entity_id entity_id) {
    transformSaInit(&checkpoint->transform, &definition->position, &definition->rotation, 1.0f);
    
    spatial_trigger_init(&checkpoint->trigger, &checkpoint->transform, &trigger_type, COLLISION_LAYER_TANGIBLE, entity_id);
    collision_scene_add_trigger(&checkpoint->trigger);

    renderable_single_axis_init(&checkpoint->renderable, &checkpoint->transform, race_get_state() == RACE_STATE_STARTED ? checkpoint_meshes[definition->checkpoint_type] : checkpoint_meshes_off[definition->checkpoint_type]);
    render_scene_add_renderable(&checkpoint->renderable, 7.0f);
    update_add(checkpoint, checkpoint_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    checkpoint->checkpoint_index = definition->checkpoint_index;
    checkpoint->is_finish = definition->checkpoint_type == CHECKPOINT_FINISH;
    checkpoint->next_checkpoint = definition->next_checkpoint_position;

    if (race_get_state() == RACE_STATE_STARTED && 
        definition->checkpoint_type == CHECKPOINT_FINISH &&
        cutscene_last_stopwatch_time() == 0.0f
    ) {
        race_set_next_checkpoint(&checkpoint->next_checkpoint);
    }
}

void checkpoint_destroy(checkpoint_t* checkpoint, struct checkpoint_definition* definition) {
    collision_scene_remove_trigger(&checkpoint->trigger);

    render_scene_remove(&checkpoint->renderable);
    renderable_destroy(&checkpoint->renderable);
    update_remove(checkpoint);
}

void checkpoint_common_init() {
    assets.checkpoint_sound = wav64_load("rom:/sounds/race/checkpoint.wav64", NULL);
}

void checkpoint_common_destroy() {
    wav64_close(assets.checkpoint_sound);
}
