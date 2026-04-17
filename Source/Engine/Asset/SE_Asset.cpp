// Copyright Seong Woo Lee. All Rights Reserved.

#include "SE_Asset.h"

#include "Core/Core_Log.h"

#include "ThirdParty/cgltf/Include/cgltf.h"

namespace Engine
{
    INTERNAL cgltf_accessor* cgltfGetAccessorFromType(cgltf_primitive* prim, cgltf_attribute_type type)
    {
        for (cgltf_size i = 0; i < prim->attributes_count; ++i) 
        {
            if (prim->attributes[i].type == type)
            {
                return prim->attributes[i].data;
            }
        }
        return nullptr;
    }

    INTERNAL vec2 cgltfReadVec2FromAccessor(cgltf_accessor* acc, uint index)
    {
        vec2 v;
        float f[2];
        cgltf_accessor_read_float(acc, index, f, 2);
        v.x = f[0];
        v.y = f[1];
        return v;
    }

    INTERNAL vec3 cgltfReadVec3FromAccessor(cgltf_accessor* acc, uint index)
    {
        vec3 v;
        float f[3];
        cgltf_accessor_read_float(acc, index, f, 3);
        v.x = f[0];
        v.y = f[1];
        v.z = f[2];
        return v;
    }

    INTERNAL vec4 cgltfReadVec4FromAccessor(cgltf_accessor* acc, uint index)
    {
        vec4 v;
        float f[4];
        cgltf_accessor_read_float(acc, index, f, 4);
        v = vec4(f[0], f[1], f[2], f[3]);
        return v;
    }

    INTERNAL SceneComponent* cgltfParseNode(cgltf_node* node)
    {
        cgltf_mesh* mesh = node->mesh;
        if (!mesh) {
            return nullptr;
        }

        SceneComponent* result = new SceneComponent;

        uint num_primitives = (uint)mesh->primitives_count;
        for (uint pi = 0; pi < num_primitives; ++pi) 
        {
            Mesh* M = new Mesh;

            AABB aabb = AABB::RevInf();

            cgltf_primitive* prim = &mesh->primitives[pi];
            cgltf_primitive_type type = prim->type;

            CORE_ASSERT(type == cgltf_primitive_type_triangles);

            cgltf_accessor* pos_acc    = cgltfGetAccessorFromType(prim, cgltf_attribute_type_position);
            cgltf_accessor* normal_acc = cgltfGetAccessorFromType(prim, cgltf_attribute_type_normal);
            cgltf_accessor* uv_acc     = cgltfGetAccessorFromType(prim, cgltf_attribute_type_texcoord);
            cgltf_accessor* tan_acc    = cgltfGetAccessorFromType(prim, cgltf_attribute_type_tangent);

            CORE_ASSERT(pos_acc);

            uint32 num_verts = (uint32)pos_acc->count;

            Array<Vertex> verts(num_verts);

            // Process vertex attributes.
            //
            for (uint32 vi = 0; vi < num_verts; ++vi)
            {
                auto& vert = verts[vi];

                vert.position = cgltfReadVec3FromAccessor(pos_acc, vi);

                if (normal_acc) {
                    vert.normal = cgltfReadVec3FromAccessor(normal_acc, vi);
                } else {
                    vert.normal = vec3(0.f, 0.f, 1.f);
                }

                if (uv_acc) {
                    vert.uv = cgltfReadVec2FromAccessor(uv_acc, vi);
                } else {
                    vert.uv = vec2(0.f, 0.f);
                }

                if (tan_acc) {
                    vec4 tangent = cgltfReadVec4FromAccessor(tan_acc, vi);
                    vert.tangent = tangent;
                } else {
                    // @Todo
                    vert.tangent = vec4(1.f, 0.f, 0.f, 0.f);
                }

                aabb.Expand(vert.position);
            }

            // Process indices.
            //
            Array<uint32> indices;
            if (prim->indices)
            {
                for (cgltf_size i = 0; i < prim->indices->count; ++i)
                {
                    indices.push_back((uint32)cgltf_accessor_read_index(prim->indices, i));
                }
            }
            else
            {
                // Make identity index buffer
                CORE_ASSERT(num_verts % 3 == 0);
                indices.resize(num_verts);
                for (uint32 i = 0; i < num_verts; ++i) 
                {
                    indices[i] = i;
                }
            }

            M->vertices = std::move(verts);
            M->indices  = std::move(indices);
            M->material = nullptr;
            M->aabb     = aabb;

            result->meshes.push_back(std::move(M));
        }

        if (node->has_matrix)
        {
            m4x4 m;
            {
                m._11 = node->matrix[0];
                m._21 = node->matrix[1];
                m._31 = node->matrix[2];
                m._41 = node->matrix[3];

                m._12 = node->matrix[4];
                m._22 = node->matrix[5];
                m._32 = node->matrix[6];
                m._42 = node->matrix[7];

                m._13 = node->matrix[8];
                m._23 = node->matrix[9];
                m._33 = node->matrix[10];
                m._43 = node->matrix[11];

                m._14 = node->matrix[12];
                m._24 = node->matrix[13];
                m._34 = node->matrix[14];
                m._44 = node->matrix[15];
            }
            result->local_transform = m;
        }
        else
        {
            vec3 translation = vec3(0, 0, 0);
            if (node->has_translation) {
                translation = vec3(node->translation[0], node->translation[1], node->translation[2]);
            }

            Quaternion rotation = Quaternion(1, 0, 0, 0);
            if (node->has_rotation) {
                rotation.x = node->rotation[0];
                rotation.y = node->rotation[1];
                rotation.z = node->rotation[2];
                rotation.w = node->rotation[3];
            }

            vec3 scale = vec3(1, 1, 1);
            if (node->has_scale) {
                scale = vec3(node->scale[0], node->scale[1], node->scale[2]);
            }

            result->local_transform = m4x4(Xform(translation, rotation, scale));
        }

        for (cgltf_size chi = 0; chi < node->children_count; ++chi)
        {
            cgltf_node* child_node =  node->children[chi];
            SceneComponent* child = cgltfParseNode(child_node);
            result->children.push_back(child);
        }

        return result;
    }

    ENGINE_API SceneComponent* LoadGLTF(const String& path)
    {
        cgltf_options opt = {};
        cgltf_data* data = nullptr;

        if (cgltf_parse_file(&opt, path.c_str(), &data) != cgltf_result_success) {
            Log("Failed to load gltf.");
            return nullptr;
        }

        if (cgltf_load_buffers(&opt, data, path.c_str()) != cgltf_result_success) {
            Log("Failed to load gltf buffer.");
            return nullptr;
        }

        uint num_scenes = (uint)data->scenes_count;
        if (num_scenes == 0) {
            return nullptr;
        }

        SceneComponent* result = new SceneComponent;

        for (uint sci = 0; sci < num_scenes; ++sci) 
        {
            cgltf_scene* scene = &data->scenes[sci];

            uint num_nodes = (uint)scene->nodes_count;
            for (uint ni = 0; ni < num_nodes; ++ni) 
            {
                cgltf_node* node = scene->nodes[ni];
                SceneComponent* child = cgltfParseNode(node);
                result->children.push_back(child);
            }
        }

        cgltf_free(data);

        return result;
    }
}
