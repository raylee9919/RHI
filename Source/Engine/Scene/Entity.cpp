// Copyright Seong Woo Lee. All Rights Reserved.

#include "Entity.h"

namespace Engine
{
    Scene::Scene()
    {
        root = new Entity;
    }

    Scene::~Scene()
    {
    }

    Entity* Scene::get_entity(Entity::ID id)
    {
        if (entity_table.contains(id)) {
            return entity_table[id];
        } else {
            return nullptr;
        }
    }

    ENGINE_API Entity* Scene::alloc_entity()
    {
        Entity* result = nullptr;

        if (free_entities.empty()) {
            result = new Entity;
        } else {
            result = free_entities.front();
            free_entities.pop_front();
        }

        result->id = next_id++;
        entity_table[result->id] = result;

        return result;
    }
}
