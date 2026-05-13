// Copyright Seong Woo Lee. All Rights Reserved.

#include "Entity.h"

namespace Engine
{
    Scene::Scene()
    {
        root = alloc_entity("SceneRoot");
    }

    Scene::~Scene()
    {
    }

    Entity::Entity()
    {
        position = vec3(0.f, 0.f, 0.f);
        orientation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
        scaling = vec3(1.0f, 1.0f, 1.0f);
    }

    Entity::~Entity()
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

    Entity* Scene::alloc_entity(const String& base_name)
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

        result->name = alloc_name(base_name);

        return result;
    }

    String Scene::alloc_name(const String& base_name)
    {
        String result = base_name;
        int prefix = 0;
        while (using_names.contains(result)) {
            result = base_name + std::to_string(prefix++);
        }
        using_names.insert(result);
        return result;
    }
}
