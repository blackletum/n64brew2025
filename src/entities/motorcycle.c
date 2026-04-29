#include "motorcycle.h"

#include "../render/tmesh.h"
#include "../resource/tmesh_cache.h"
#include "../render/render_scene.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"
#include "../time/time.h"
#include "../math/mathf.h"
#include "../config.h"
#include "../scene/scene.h"
#include "../player/inventory.h"
#include "../resource/animation_cache.h"
#include "../math/mathf.h"
#include "../util/input.h"

#define HOVER_SAG_AMOUNT        0.25f
#define HOVER_SPRING_STRENGTH   (-GRAVITY_CONSTANT / HOVER_SAG_AMOUNT)
#define STOPPED_SPEED_THESHOLD  8.0f

#define ACCEL_RATE              20.0f
#define BOOST_ACCEL_RATE        60.0f
#define BASE_DRIVE_SPEED        25.0f
#define UPGRADED_DRIVE_SPEED    35.0f
#define BASE_BOOST_SPEED        50.0f
#define UPGRADED_BOOST_SPEED    60.0f
#define MAX_TURN_RATE           2.0f

#define MAX_SLOPE_CLIMB         0.7f
#define MAX_FAST_SLOPE_CLIMB    0.8f

#define DRIFT_SPEED_REDUNCTION  3.0f
#define MIN_DRIFT_TURN_RATE     1.3f
#define MAX_DRIFT_TURN_RATE     2.6f

#define MAX_BOOST_TURN_ACCEL    50.0f
#define MAX_TURN_ACCEL          30.0f
#define DRIFT_ACCEL             20.0f

#define TURN_BOOST_SLOW_THESHOLD    (MAX_BOOST_TURN_ACCEL / MAX_TURN_RATE)
#define TURN_SLOW_THRESHOLD         (MAX_TURN_ACCEL / MAX_TURN_RATE)

#define STILL_HOVER_HEIGHT      0.25f
#define RIDE_HOVER_HEIGHT       0.5f
#define FAST_HOVER_HEIGHT       1.0f
#define BOB_HEIGHT              0.1f
#define BOB_TIME                12.0f

#define BASE_BOOST_TIME         3.0f
#define UPGRADED_BOOST_TIME     3.5f

#define SELF_BOOST_COOLDOWN     6.0f
#define DRIFT_BOOST_COOLDOWN    2.0f
#define JUMP_BOOST_COOLDOWN     1.2f

#define CAST_CENTER             0.3f

#define COYOTE_TIME             1.5f
#define COYOTE_FADE_TIME        0.5f

struct motorcyle_assets {
    tmesh_t* mesh;
    wav64_t* boost;
    wav64_t* idle;
};

typedef struct motorcyle_assets motorcyle_assets_t;

static motorcyle_assets_t assets;

static dynamic_object_type_t collider_type = {
    BOX_COLLIDER(0.3f, 0.4f, 0.8f),
    .max_stable_slope = 0.0f,
    .surface_type = SURFACE_TYPE_DEFAULT,
    .friction = 0.0f,
    .bounce = 0.0f,
    .center = {0.0f, 0.6f, 0.0f},
};

static vehicle_camera_target_t boost_positions[VEHICLE_CAM_COUNT] = {
    [VEHICLE_CAM_NEUTRAL] = {
        .position = {0.0f, 1.75f, -4.0f},
        .look_at = {0.0f, 1.75f, 2.0f},
    },
    [VEHICLE_CAM_U] = {
        .position = {0.0f, 12.5f, -6.0f},
        .look_at = {0.0f, 1.5f, 2.0f},
    },
    [VEHICLE_CAM_UR] = {
        .position = {-6.0f, 6.5f, -3.0f},
        .look_at = {0.0f, 1.5f, 2.0f},
    },
    [VEHICLE_CAM_R] = {
        .position = {-4.0f, 2.5f, 1.0f},
        .look_at = {0.0f, 2.0f, 1.0f},
    },
    [VEHICLE_CAM_DR] = {
        .position = {-3.0f, 1.0f, -2.0f},
        .look_at = {0.0f, 3.0f, 1.0f},
    },
    [VEHICLE_CAM_D] = {
        .position = {0.0f, 1.0f, -3.0f},
        .look_at = {0.0f, 2.5f, 2.0f},
    },
    [VEHICLE_CAM_DL] = {
        .position = {3.0f, 1.0f, -2.0f},
        .look_at = {0.0f, 3.0f, 1.0f},
    },
    [VEHICLE_CAM_L] = {
        .position = {4.0f, 2.5f, 1.0f},
        .look_at = {0.0f, 2.0f, 1.0f},
    },
    [VEHICLE_CAM_UL] = {
        .position = {6.0f, 6.5f, -3.0f},
        .look_at = {0.0f, 1.5f, 2.0f},
    },
};

static vehicle_definiton_t vehicle_def = {
    .local_player_position = {
        .x = 0.0f,
        .y = 0.0f,
        .z = 0.0f,
    },
    .local_player_rotation = {1.0f, 0.0f},
    .exit_position = {-1.0f, 0.0f, 0.0f},
    .camera_positions = {
        [VEHICLE_CAM_NEUTRAL] = {
            .position = {0.0f, 2.0f, -4.0f},
            .look_at = {0.0f, 2.0f, 2.0f},
        },
        [VEHICLE_CAM_U] = {
            .position = {0.0f, 12.5f, -6.0f},
            .look_at = {0.0f, 1.5f, 2.0f},
        },
        [VEHICLE_CAM_UR] = {
            .position = {-6.0f, 6.5f, -3.0f},
            .look_at = {0.0f, 1.5f, 2.0f},
        },
        [VEHICLE_CAM_R] = {
            .position = {-4.0f, 2.5f, 1.0f},
            .look_at = {0.0f, 2.0f, 1.0f},
        },
        [VEHICLE_CAM_DR] = {
            .position = {-3.0f, 1.0f, -2.0f},
            .look_at = {0.0f, 3.0f, 1.0f},
        },
        [VEHICLE_CAM_D] = {
            .position = {0.0f, 1.0f, -3.0f},
            .look_at = {0.0f, 2.5f, 2.0f},
        },
        [VEHICLE_CAM_DL] = {
            .position = {3.0f, 1.0f, -2.0f},
            .look_at = {0.0f, 3.0f, 1.0f},
        },
        [VEHICLE_CAM_L] = {
            .position = {4.0f, 2.5f, 1.0f},
            .look_at = {0.0f, 2.0f, 1.0f},
        },
        [VEHICLE_CAM_UL] = {
            .position = {6.0f, 6.5f, -3.0f},
            .look_at = {0.0f, 1.5f, 2.0f},
        },
    },
    .boost_camera_positions = boost_positions,
};

static vector3_t local_cast_points[CAST_POINT_COUNT] = {
    {0.0f, CAST_CENTER, 0.0f},
    {0.31f, CAST_CENTER, 0.7f},
    {0.31f, CAST_CENTER, -0.7f},
    {-0.31f, CAST_CENTER, 0.7f},
    {-0.31f, CAST_CENTER, -0.7f},
};

void motorcycle_common_init() {
    assets.mesh = tmesh_cache_load("rom:/meshes/vehicles/bike.tmesh");
    assets.boost = wav64_load("rom:/sounds/vehicle/boost_pad.wav64", NULL);
    assets.idle = wav64_load("rom:/sounds/vehicle/cycle_idle.wav64", NULL);
}

void motorcycle_common_destroy() {
    tmesh_cache_release(assets.mesh);
    wav64_close(assets.boost);
    wav64_close(assets.idle);
    assets.mesh = NULL;
    assets.boost = NULL;
    assets.idle = NULL;
}

void motorcycle_ride(struct interactable* interactable, entity_id from) {

}

float motorcycle_hover_height(motorcycle_t* motorcycle, float speed) {
    float bob_height = BOB_HEIGHT * cosf(game_time * 2.0f * PI_F * (1.0f / BOB_TIME));

    if (!motorcycle->vehicle.driver) {
        return STILL_HOVER_HEIGHT + bob_height;
    }
    
    float lerp = speed * (1.0f / UPGRADED_DRIVE_SPEED);

    if (lerp > 1.0f) {
        return FAST_HOVER_HEIGHT;
    }

    return mathfLerp(RIDE_HOVER_HEIGHT + bob_height, FAST_HOVER_HEIGHT, lerp);
}

float motorcycle_target_speed(motorcycle_t* motorcycle, joypad_inputs_t input) {
    if (input.btn.a || motorcycle->vehicle.is_boosting) {
        if (motorcycle->boost_timer > 0.0f) {
            if (inventory_has_item(ITEM_UPGRADE_FASTER)) {
                return UPGRADED_BOOST_SPEED;
            }

            return BASE_BOOST_SPEED;
        }

        if (inventory_has_item(ITEM_UPGRADE_FASTER)) {
            return UPGRADED_DRIVE_SPEED;
        }

        return BASE_DRIVE_SPEED;
    } else if (input.btn.b) {
        return 0.0f;
    } else {
        return 0.99f * sqrtf(vector3MagSqrd2D(&motorcycle->collider.velocity));
    }
}

float motorycle_get_ground_height(motorcycle_t* motorcycle, float target_height, vector3_t* ground_normal) {
    float min_height_offset = target_height;

    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        cast_point_t* cast_point = &motorcycle->cast_points[i];

        if (cast_point->surface_type != SURFACE_TYPE_NONE) {
            float actual_height = cast_point->pos.y - cast_point->y - CAST_CENTER;

            if (actual_height < min_height_offset) {
                min_height_offset = actual_height;
                *ground_normal = cast_point->normal;
            }
        }

        vector3_t pos;
        transformSaTransformPoint(&motorcycle->transform, &local_cast_points[i], &pos);
        cast_point_set_pos(cast_point, &pos);
    }

    return min_height_offset;
}

void motorcycle_render(void* data, render_batch_t* batch) {
    motorcycle_t* motorcycle = (motorcycle_t*)data;

    renderable_t* renderable = &motorcycle->renderable;

    if (renderable->hide) {
        return;
    }
    
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    transform_sa_t rotated_transform = motorcycle->transform;

    vector2ComplexMul(&motorcycle->transform.rotation, &vehicle_def.local_player_rotation, &rotated_transform.rotation);

    mat4x4 mtx;
    transformSAToMatrix(&rotated_transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    if (render_should_cull(mtx)) {
        return;
    }
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    struct render_batch_element* element = render_batch_add_tmesh(
        batch, 
        renderable->mesh_render.mesh, 
        mtxfp, 
        &renderable->mesh_render.armature, 
        renderable->mesh_render.attachments,
        renderable->attrs
    );

    if (element && renderable->mesh_render.force_material) {
        element->material = renderable->mesh_render.force_material;
    }
}

#define MOTORCYCLE_CULL_DISTANCE    70.0f

bool motorcycle_check_active(motorcycle_t* motorcycle) {
    bool is_active = vector3DistSqrd(&motorcycle->transform.position, player_get_position(&current_scene->player)) < MOTORCYCLE_CULL_DISTANCE * MOTORCYCLE_CULL_DISTANCE;

    if (is_active && !motorcycle->is_active) {
        collision_scene_add(&motorcycle->collider);
        motorcycle->drop_shadow.enabled = false;
        motorcycle->renderable.hide = false;
    } else if (!is_active && motorcycle->is_active) {
        motorcycle->renderable.hide = true;
        collision_scene_remove(&motorcycle->collider);
        motorcycle->drop_shadow.enabled = true;
    }

    motorcycle->is_active = is_active;

    return is_active;
}

float motorcycle_get_boost_charge(motorcycle_t* motorcycle) {
    if (!motorcycle) {
        return 0.0f;
    }

    if (motorcycle->self_boost_cooldown <= 0.0f) {
        return 1.0f;
    }

    if (motorcycle->self_boost_cooldown >= SELF_BOOST_COOLDOWN) {
        return 0.0f;
    }

    return 1.0f - motorcycle->self_boost_cooldown * (1.0f / SELF_BOOST_COOLDOWN);
}

void motorcycle_check_for_mount(motorcycle_t* motorcycle) {
    if (inventory_has_item(ITEM_RIDING_MOTORCYCLE) && current_scene->player.state != PLAYER_IN_VEHICLE) {
        player_enter_vehicle(&current_scene->player, ENTITY_ID_MOTORCYLE);
    }
}

void motorcycle_update_sound(motorcycle_t* motorcycle, float speed) {
    if (motorcycle->vehicle.driver && !motorcycle->idle_sound) {
        motorcycle->idle_sound = audio_play_3d(assets.idle, 0.3f, &motorcycle->transform.position, &motorcycle->collider.velocity, 1.0f, 2);
    } else if (!motorcycle->vehicle.driver && motorcycle->idle_sound) {
        audio_stop(motorcycle->idle_sound);
        motorcycle->idle_sound = 0;
    }

    if (motorcycle->idle_sound) {
        float idle_lerp = speed * (1.0f / UPGRADED_DRIVE_SPEED);

        if (idle_lerp > 1.0f) {
            idle_lerp = 1.0f;
        }

        audio_update_position(motorcycle->idle_sound, &motorcycle->transform.position, &motorcycle->collider.velocity);
        audio_update_volume(motorcycle->idle_sound, mathfLerp(0.3f, 0.5f, idle_lerp));
        audio_update_pitch(motorcycle->idle_sound, mathfLerp(0.9f, 1.5f, idle_lerp));
    }
}

void motorcycle_update(void* data) {
    motorcycle_t* motorcycle = (motorcycle_t*)data;

    if (!motorcycle_check_active(motorcycle)) {
        return;
    }
    
    motorcycle->collider.is_jumping = 2;

    bool debug = joypad_get_buttons(0).c_down;

    armature_t* armature = renderable_get_armature(&motorcycle->renderable);
    animator_update(&motorcycle->animator, armature, scaled_time_step);

    vector3_t ground_normal = (vector3_t){};
    float min_height_offset = motorycle_get_ground_height(motorcycle, FAST_HOVER_HEIGHT + HOVER_SAG_AMOUNT, &ground_normal);
    vector3Normalize(&ground_normal, &ground_normal);
    
    vector3_t ground_velocity;
    vector3ProjectPlane(&motorcycle->collider.velocity, &ground_normal, &ground_velocity);

    float current_speed = sqrtf(vector3MagSqrd(&ground_velocity));

    float target_height = motorcycle_hover_height(motorcycle, current_speed) + HOVER_SAG_AMOUNT;
    bool is_grounded = min_height_offset < target_height;
    bool has_control = is_grounded;
    float accel_modifier = 1.0f;

    if (is_grounded) {
        motorcycle->coyote_timer = 0.0f;
    } else if (motorcycle->coyote_timer < COYOTE_TIME) {
        motorcycle->coyote_timer += fixed_time_step;
        has_control = true;
        ground_normal = gUp;
        accel_modifier = (motorcycle->coyote_timer < COYOTE_TIME - COYOTE_FADE_TIME) ? 
            1.0f : 
            (COYOTE_TIME - motorcycle->coyote_timer) * (1.0f / COYOTE_FADE_TIME);
        ground_velocity = motorcycle->collider.velocity;
        ground_velocity.y = 0.0f;
    }

    vector3_t normal_velocity;
    vector3Project(&motorcycle->collider.velocity, &ground_normal, &normal_velocity);

    vector3_t forward;

    vector2ToLookDir(&motorcycle->transform.rotation, &forward);

    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    bool activate_boost = motorcycle->vehicle.hit_boost_pad;

    motorcycle->vehicle.is_boosting = activate_boost;
    
    if (pressed.z && motorcycle->self_boost_cooldown <= 0.0f && inventory_has_item(ITEM_BOOST_ANYWHERE)) {
        motorcycle->self_boost_cooldown = SELF_BOOST_COOLDOWN;
        activate_boost = true;
    }
    
#if ENABLE_CHEATS
    if (pressed.z) {
        activate_boost = true;
    }
#endif

    vector2_t target_rotation_offset = gRight2;

    if (is_grounded && motorcycle->drift_direction) {
        motorcycle->self_boost_cooldown -= fixed_time_step * (SELF_BOOST_COOLDOWN / DRIFT_BOOST_COOLDOWN);

        if (!input.btn.r) {
            motorcycle->drift_direction = 0;

            if (motorcycle->self_boost_cooldown <= 0.0f) {
                activate_boost = true;
                motorcycle->self_boost_cooldown = SELF_BOOST_COOLDOWN;
            }
        } else {
            vector2ComplexFromAngle(motorcycle->drift_direction > 0 ? 0.3f : -0.3f, &target_rotation_offset);
        }
    } else if (!is_grounded) {
        motorcycle->self_boost_cooldown -= fixed_time_step * (SELF_BOOST_COOLDOWN / JUMP_BOOST_COOLDOWN);
    } else if (!motorcycle->was_grounded && motorcycle->self_boost_cooldown <= 0.0f && !inventory_has_item(ITEM_BOOST_ANYWHERE)) {
        activate_boost = true;
        motorcycle->self_boost_cooldown = SELF_BOOST_COOLDOWN;
    }

    vector2_t max_rotation;
    vector2ComplexFromAngle(1.4f * fixed_time_step, &max_rotation);
    vector2RotateTowards(&vehicle_def.local_player_rotation, &target_rotation_offset, &max_rotation, &vehicle_def.local_player_rotation);

    if (activate_boost) {
        if (inventory_has_item(ITEM_LONGER_BOOST)) {
            motorcycle->boost_timer = UPGRADED_BOOST_TIME;
        } else {
            motorcycle->boost_timer = BASE_BOOST_TIME;
        }

        motorcycle->vehicle.hit_boost_pad = false;
        
        if (!motorcycle->boost_sound) {
            motorcycle->boost_sound = audio_play_2d(assets.boost, 0.3f, 0.0f, 1.0f, 1);
        }
    } else if (motorcycle->boost_sound) {
        motorcycle->boost_sound = 0;
    }
    
    if (inventory_has_item(ITEM_BOOST_ANYWHERE)) {
        if (motorcycle->self_boost_cooldown > 0.0f) {
            motorcycle->self_boost_cooldown -= fixed_time_step;
        }
    } else if (!motorcycle->drift_direction && motorcycle->self_boost_cooldown < SELF_BOOST_COOLDOWN && is_grounded) {
        motorcycle->self_boost_cooldown += fixed_time_step * (SELF_BOOST_COOLDOWN / DRIFT_BOOST_COOLDOWN);
    }

    if (motorcycle->boost_timer > 0.0f) {
        motorcycle->boost_timer -= fixed_time_step;
        motorcycle->vehicle.is_boosting = input.btn.a || activate_boost;

        if (!input.btn.a) {
            motorcycle->boost_timer = 0.0f;
        }
    } 

    bool are_brakes_on = true;
    
    float target_speed = current_speed;

    motorcycle_update_sound(motorcycle, current_speed);

    if (motorcycle->vehicle.driver && update_has_layer(UPDATE_LAYER_WORLD)) {
        float accel = motorcycle->vehicle.is_boosting ? BOOST_ACCEL_RATE : ACCEL_RATE;

        float target_max_speed = motorcycle_target_speed(motorcycle, input);
        if (motorcycle->drift_direction != 0) {
            target_max_speed -= DRIFT_SPEED_REDUNCTION;
        }
        are_brakes_on = target_max_speed == 0.0f;
        target_speed = mathfMoveTowards(target_speed, target_max_speed, fixed_time_step * ACCEL_RATE);


        if (target_speed > target_max_speed) {
            target_speed = target_max_speed;
        }

        float turn_rate = MAX_TURN_RATE;

        float slow_threshold = motorcycle->vehicle.is_boosting ? TURN_BOOST_SLOW_THESHOLD : TURN_SLOW_THRESHOLD;
        float turn_accel = motorcycle->vehicle.is_boosting ? MAX_BOOST_TURN_ACCEL : MAX_TURN_ACCEL;

        if (target_speed > slow_threshold) {
            turn_rate = turn_accel / target_speed;
        }

        float rotate_rate;

        float input_normalized = clampf(input_handle_deadzone(input.stick_x) * (1.0f / 80.0f), -1.0f, 1.0f);

        if (motorcycle->drift_direction) {
            rotate_rate = (input_normalized + motorcycle->drift_direction * MIN_DRIFT_TURN_RATE) * turn_rate * (0.5f * MAX_DRIFT_TURN_RATE / MAX_TURN_RATE);
        } else {
            rotate_rate = turn_rate * input_normalized;

            if (input.btn.r) {
                motorcycle->drift_direction = input_normalized > 0.0f ? 1 : -1;
                inventory_set_has_item(ITEM_HAS_DRIFTED, true);
            }
        }

        vector2_t new_rot;
        vector2_t rotation_amount;

        vector2ComplexFromAngle(fixed_time_step * rotate_rate, &rotation_amount);
        vector2ComplexMul(&motorcycle->transform.rotation, &rotation_amount, &new_rot);
        
        vector2ToLookDir(&new_rot, &forward);
        motorcycle->transform.rotation = new_rot;
    } else {
        target_speed = mathfMoveTowards(target_speed, 0.0f, fixed_time_step * ACCEL_RATE);
        motorcycle->drift_direction = 0;
    }

    motorcycle->vehicle.is_stopped = vector3MagSqrd2D(&motorcycle->collider.velocity) < STOPPED_SPEED_THESHOLD * STOPPED_SPEED_THESHOLD;

    if (motorcycle->collider.hit_kill_plane) {
        motorcycle->collider.velocity = gZeroVec;
        motorcycle->transform.position = motorcycle->last_ground_location;
        motorcycle->collider.hit_kill_plane = 0;
        motorcycle_check_for_mount(motorcycle);
    }

    if (has_control) {
        vector3_t* vel = &motorcycle->collider.velocity;
        float prev_y = vel->y;

        if (!are_brakes_on) {
            prev_y = vector3Dot(&ground_normal, vel);
        }

        vector3_t target_vel;
        vector2ToLookDir(&motorcycle->transform.rotation, &target_vel);
        vector3ProjectPlane(&target_vel, &ground_normal, &target_vel);
        vector3Normalize(&target_vel, &target_vel);
        vector3Normalize(&ground_velocity, &ground_velocity);

        float speed_in_target_direction = vector3Dot(&target_vel, &ground_velocity);

        if (speed_in_target_direction > 0 && !motorcycle->was_stopped && !are_brakes_on) {
            float transferred_speed = 0.99f * current_speed * speed_in_target_direction;

            if (transferred_speed > target_speed) {
                target_speed = transferred_speed;
            }
        }

        vector3Scale(&target_vel, &target_vel, target_speed);
        motorcycle->last_ground_location = motorcycle->transform.position;

        motorcycle->last_ground_location.y += 5.0f;

        float max_accel = motorcycle->has_traction && motorcycle->drift_direction == 0 ? MAX_TURN_ACCEL : DRIFT_ACCEL;

        float target_offset = target_height - min_height_offset;
        
        float spring_accel = target_offset * HOVER_SPRING_STRENGTH;

        if (prev_y > 0.0f) {
            float vel_threshold = -target_offset * 2.0f * GRAVITY_CONSTANT;

            if (prev_y * prev_y > vel_threshold) {
                spring_accel *= 0.5f;
            }
        }

        if (is_grounded && ground_normal.y > 0.0f) {
            if (vector3Dot(vel, &ground_normal) < 0.0f) {
                vector3ProjectPlane(vel, &ground_normal, vel);
            }

            float horz_dot = target_vel.x * ground_normal.x + target_vel.z * ground_normal.z;

            if (horz_dot < 0.0f) {
                if (ground_normal.y < MAX_FAST_SLOPE_CLIMB) {
                    vector3_t side;
                    vector3Cross(&target_vel, &ground_normal, &side);
                    vector3_t back;
                    vector3Cross(&side, &ground_normal, &back);

                    vector3_t projected_vel;
                    vector3ProjectPlane(&target_vel, &back, &projected_vel);
                    float lerp = (ground_normal.y - MAX_SLOPE_CLIMB) * (1.0f / (MAX_FAST_SLOPE_CLIMB - MAX_SLOPE_CLIMB));

                    if (lerp <= 0.0f) {
                        target_vel = projected_vel;
                    } else {
                        vector3Lerp(&projected_vel, &target_vel, lerp, &target_vel);
                    }
                }
            }
        }
        
        if (is_grounded) {
            target_vel.y += spring_accel * fixed_time_step;
        } else {
            vector3Scale(&target_vel, &target_vel, 0.99f);
            vector3Add(&target_vel, &normal_velocity, &target_vel);
        }

        motorcycle->has_traction = vector3MoveTowards(vel, &target_vel, 2.0f * accel_modifier * max_accel * scaled_time_step, vel);
    } else {
        vector3_t* vel = &motorcycle->collider.velocity;
        vector3Scale(vel, vel, 0.99f);
    }

    motorcycle->vehicle.is_grounded = is_grounded;
    motorcycle->vehicle.ground_normal_y = ground_normal.y;
    motorcycle->was_grounded = is_grounded;
    motorcycle->was_stopped = target_speed == 0.0f && motorcycle->has_traction;
}

static motorcycle_t* current_instance;

void motorcycle_init(motorcycle_t* motorcycle, struct motorcycle_definition* definition, entity_id entity_id) {
    transformSaInit(&motorcycle->transform, &definition->position, &definition->rotation, 1.0f);
    renderable_single_axis_init_direct(&motorcycle->renderable, &motorcycle->transform, assets.mesh);
    render_scene_add(&motorcycle->transform.position, 2.0f, motorcycle_render, motorcycle);
    dynamic_object_init(entity_id, &motorcycle->collider, &collider_type, COLLISION_LAYER_TANGIBLE, &motorcycle->transform.position, &motorcycle->transform.rotation);
    
    armature_t* armature = renderable_get_armature(&motorcycle->renderable);
    animator_init(&motorcycle->animator, armature->bone_count);
    motorcycle->animations = animation_cache_load("rom:/meshes/vehicles/bike.anim");
    vehicle_init(&motorcycle->vehicle, &motorcycle->transform, &vehicle_def, entity_id);
    collision_scene_add(&motorcycle->collider);
    update_add(motorcycle, motorcycle_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
    
    interactable_init(&motorcycle->interactable, entity_id, INTERACT_TYPE_RIDE, motorcycle_ride, motorcycle);
    
    motorcycle->has_traction = true;
    motorcycle->is_active = true;
    motorcycle->boost_timer = 0.0f;
    motorcycle->last_ground_location = definition->position;
    motorcycle->self_boost_cooldown = SELF_BOOST_COOLDOWN;
    motorcycle->boost_sound = 0;
    motorcycle->idle_sound = 0;
    motorcycle->drift_direction = 0;
    motorcycle->was_stopped = true;
    motorcycle->collider.is_jumping = 2;
    motorcycle->coyote_timer = 0.0f;

    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        vector3_t cast_point;
        transformSaTransformPoint(&motorcycle->transform, &local_cast_points[i], &cast_point);
        collision_scene_add_cast_point(&motorcycle->cast_points[i], &cast_point);
    }

    motorcycle_check_for_mount(motorcycle);
    animator_run_clip(&motorcycle->animator, animation_set_find_clip(motorcycle->animations, "idle"), 0.0f, true);
    drop_shadow_init(&motorcycle->drop_shadow, &motorcycle->collider, "rom:/meshes/effects/drop-shadow.tmesh");

    current_instance = motorcycle;
}

void motorcycle_destroy(motorcycle_t* motorcycle) {
    audio_stop(motorcycle->idle_sound);

    if (motorcycle->is_active) {
        collision_scene_remove(&motorcycle->collider);
    }
    render_scene_remove(motorcycle);
    renderable_destroy_direct(&motorcycle->renderable);
    interactable_destroy(&motorcycle->interactable);
    update_remove(motorcycle);
    vehicle_destroy(&motorcycle->vehicle);
    animator_destroy(&motorcycle->animator);
    animation_cache_release(motorcycle->animations);
    drop_shadow_destroy(&motorcycle->drop_shadow);
    
    for (int i = 0; i < CAST_POINT_COUNT; i += 1) {
        collision_scene_remove_cast_point(&motorcycle->cast_points[i]);
    }

    current_instance = NULL;
}

motorcycle_t* motorcycle_get() {
    return current_instance;
}