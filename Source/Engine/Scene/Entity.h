// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Engine/Core/SE_Basics.h"
#include "Engine/Core/SE_Math.h"

namespace Engine
{
    struct ENGINE_API Entity {
        typedef u64 ID;

        ID id;                  // generational ID
        String name;            // must be unique.
        List <ID> children;

        String mesh_name;

        // Transform
        vec3 position;
        Quaternion orientation;
        vec3 scaling;

        // index in mapped buffer
        u32 current_transform_index;

        Entity();
        ~Entity();

        // @Todo: This is wrong. It may contain invalid ids.
        // The thing I want to do is retain strictly correct pointers.
        FORCE_INLINE bool is_leaf() { return children.empty(); }
    };

    struct ENGINE_API Scene {
        u64 next_id = 1; // next generational ID
        List <Entity*> free_entities;
        Entity* root;
        Hash_Table <Entity::ID, Entity*> entity_table;
        Set <String> using_names;


        Scene();
        ~Scene();
        Entity* alloc_entity(const String& base_name);
        Entity* get_entity(Entity::ID id);
        String alloc_name(const String& base_name);
    };
}
