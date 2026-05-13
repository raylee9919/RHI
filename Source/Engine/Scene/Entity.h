// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Engine/Core/SE_Basics.h"
#include "Engine/Core/SE_Math.h"

namespace Engine
{
    struct ENGINE_API Entity {
        typedef u64 ID;

        ID id; // generational ID
        String mesh_name;
        List <ID> children;

        // Transform
        vec3 position;
        Quaternion orientation;
        vec3 scaling;

        // index in mapped buffer
        u32 current_transform_index;

        Entity();
        ~Entity();
        FORCE_INLINE bool operator == (const Entity* other) { return id == other->id; }
    };

    struct ENGINE_API Scene {
        u64 next_id = 1; // next generational ID
        List <Entity*> free_entities;
        Entity* root;
        Hash_Table <Entity::ID, Entity*> entity_table;


        Scene();
        ~Scene();
        Entity* alloc_entity();
        Entity* get_entity(Entity::ID id);
    };
}
