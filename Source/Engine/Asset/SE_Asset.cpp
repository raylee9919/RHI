// Copyright Seong Woo Lee. All Rights Reserved.

#include "SE_Asset.h"

#include "Core/SE_Basics.h"
#include "Core/SE_Log.h"
#include "File/FileSystem.h"

#include "ThirdParty/cgltf/Include/cgltf.h"
#include "ThirdParty/MikkTSpace/Include/mikktspace.h"
#include "ThirdParty/stb/Include/stb_image.h"

namespace Engine
{
    INTERNAL cgltf_accessor* cgltf_get_accessor_from_type(cgltf_primitive* prim, cgltf_attribute_type type)
    {
        for (cgltf_size i = 0; i < prim->attributes_count; ++i) {
            if (prim->attributes[i].type == type) {
                return prim->attributes[i].data;
            }
        }
        return nullptr;
    }

    INTERNAL vec2 cgltf_read_vec2_from_accessor(cgltf_accessor* acc, uint index)
    {
        vec2 v;
        float f[2];
        cgltf_accessor_read_float(acc, index, f, 2);
        v.x = f[0];
        v.y = f[1];
        return v;
    }

    INTERNAL vec3 cgltf_read_vec3_from_accessor(cgltf_accessor* acc, uint index)
    {
        vec3 v;
        float f[3];
        cgltf_accessor_read_float(acc, index, f, 3);
        v.x = f[0];
        v.y = f[1];
        v.z = f[2];
        return v;
    }

    INTERNAL vec4 cgltf_read_vec4_from_accessor(cgltf_accessor* acc, uint index)
    {
        vec4 v;
        float f[4];
        cgltf_accessor_read_float(acc, index, f, 4);
        v = vec4(f[0], f[1], f[2], f[3]);
        return v;
    }

    INTERNAL int mikkt_get_num_faces(const SMikkTSpaceContext* ctx)
    {
        Mesh_Slice* mesh = (Mesh_Slice *)ctx->m_pUserData;
        return mesh->indices.size() / 3;
    }

    INTERNAL int mikkt_get_num_vertices_of_face(const SMikkTSpaceContext* ctx, const int face)
    {
        return 3;
    }

    INTERNAL void mikkt_get_position(const SMikkTSpaceContext* ctx, float out[], const int face, const int vert)
    {
        Mesh_Slice* mesh = (Mesh_Slice *)ctx->m_pUserData;
        u32 index = mesh->indices[face * 3 + vert];
        vec3 pos = mesh->vertices[index].position;
        out[0] = pos.x;
        out[1] = pos.y;
        out[2] = pos.z;
    }

    INTERNAL void mikkt_get_normal(const SMikkTSpaceContext* ctx, float out[], const int face, const int vert)
    {
        Mesh_Slice* mesh = (Mesh_Slice *)ctx->m_pUserData;
        u32 index = mesh->indices[face * 3 + vert];
        vec3 normal = mesh->vertices[index].normal;
        out[0] = normal.x;
        out[1] = normal.y;
        out[2] = normal.z;
    }

    INTERNAL void mikkt_get_tex_coord(const SMikkTSpaceContext* ctx, float out[], const int face, const int vert)
    {
        Mesh_Slice* mesh = (Mesh_Slice *)ctx->m_pUserData;
        u32 index = mesh->indices[face * 3 + vert];
        vec2 uv = mesh->vertices[index].uv;
        out[0] = uv.x;
        out[1] = uv.y;
    }

    INTERNAL void mikkt_set_tspace_basic(const SMikkTSpaceContext* ctx, const float in[], const float sign, const int face, const int vert)
    {
        Mesh_Slice* mesh = (Mesh_Slice *)ctx->m_pUserData;
        u32 index = mesh->indices[face * 3 + vert];
        vec4* tangent = &mesh->vertices[index].tangent;
        tangent->x = in[0];
        tangent->y = in[1];
        tangent->z = in[2];
        tangent->w = sign;
    }

    INTERNAL Entity* make_entity_from_cgltf_node(Scene* world,
                                                 const Array<Mesh*>& meshes, const Array<cgltf_mesh*>& cgltf_meshes,
                                                 cgltf_node* node, Entity* parent)
    {
        String name = node->name ? node->name : "Entity";
        Entity* entity = world->alloc_entity(name);

        if (parent) { parent->children.push_back(entity->id); }

        // Bind mesh name if has one.
        if (node->mesh) {
            u32 index = 0;
            for (auto* cmesh : cgltf_meshes) {
                if (cmesh == node->mesh) {
                    entity->mesh_name = meshes[index]->name;
                    break;
                }
                index++;
            }
            CORE_ASSERT(entity->mesh_name != "");
        }

        // transform
        if (node->has_translation) {
            entity->position = vec3(node->translation[0], node->translation[1], node->translation[2]);
        }
        if (node->has_rotation) {
            entity->orientation = Quaternion(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
        }
        if (node->has_scale) {
            entity->scaling = vec3(node->scale[0], node->scale[1], node->scale[2]);
        }

        for (cgltf_size chi = 0; chi < node->children_count; ++chi) {
            auto* child = node->children[chi];
            make_entity_from_cgltf_node(world, meshes, cgltf_meshes, child, entity);
        }

        return entity;
    }

    ENGINE_API GLTF_Load_Result load_gltf(Scene* world, Asset_State* state, const String& path, bool flip_uv)
    {
        GLTF_Load_Result result = {};
        result.materials = nlohmann::json::array();

        String dir = file_sys::path(path).parent_path().string();
        cgltf_options opts = {};
        cgltf_data* data = nullptr;

        if (cgltf_parse_file(&opts, path.c_str(), &data) != cgltf_result_success) {
            CORE_ASSERT(!"failed to parse gltf file.");
        }

        if (cgltf_load_buffers(&opts, data, path.c_str()) != cgltf_result_success) {
            CORE_ASSERT(!"failed to load buffers from gltf file.");
        }


        // Materials
        //
        Array <String> materials;
        const String base_name = file_sys::path(path).stem().filename().string();

        for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
            cgltf_material* mat = &data->materials[mi];

            // fetch image paths.
            String albedo_path, orm_path, normal_path, emissive_path;

            auto get_texture_path = [&](cgltf_texture_view& view) ->String {
                if (view.texture && view.texture->image && view.texture->image->uri) {
                    return (file_sys::path(dir) / view.texture->image->uri).lexically_normal().string();
                }
                return "";
            };

            if (mat->has_pbr_metallic_roughness) {
                albedo_path = get_texture_path(mat->pbr_metallic_roughness.base_color_texture);
                orm_path    = get_texture_path(mat->pbr_metallic_roughness.metallic_roughness_texture);
            }
            normal_path   = get_texture_path(mat->normal_texture);
            emissive_path = get_texture_path(mat->emissive_texture);

            // increment suffix number until it founds unique name.
            String name = state->make_name(base_name + "_material_");

            // make json metadata and push to array.
            nlohmann::json json;
            {
                json["name"]     = name;
                json["albedo"]   = albedo_path;
                json["orm"]      = orm_path;
                json["normal"]   = normal_path;
                json["emissive"] = emissive_path;
            }
            result.materials.push_back(json);

            // push material name in a same order as it is in gltf file so 
            // each mesh can reference it correctly.
            materials.push_back(name);
        }
        state->reset_suffix();


        // Meshes
        //
        Array <cgltf_mesh*> cgltf_meshes;
        u32 num_meshes = (u32)data->meshes_count;

        for (u32 mi = 0; mi < num_meshes; ++mi) {
            Mesh* mesh = new Mesh;
            cgltf_mesh* cmesh = &data->meshes[mi];
            cgltf_meshes.push_back(cmesh);
            String name = state->make_name(base_name + "_mesh_");

            // Primitive isn't a triangle/line in here. It's a mesh with its own material.
            // a.k.a., mesh slice/submesh.
            u32 num_prim = (u32)cmesh->primitives_count;
            for (u32 pi = 0; pi < num_prim; ++pi) {
                Mesh_Slice* mesh_slice = new Mesh_Slice;

                cgltf_primitive* prim = &cmesh->primitives[pi];
                cgltf_primitive_type type = prim->type;
                CORE_ASSERT(type == cgltf_primitive_type_triangles);

                cgltf_accessor* pos_acc    = cgltf_get_accessor_from_type(prim, cgltf_attribute_type_position);
                cgltf_accessor* normal_acc = cgltf_get_accessor_from_type(prim, cgltf_attribute_type_normal);
                cgltf_accessor* uv_acc     = cgltf_get_accessor_from_type(prim, cgltf_attribute_type_texcoord);
                cgltf_accessor* tan_acc    = cgltf_get_accessor_from_type(prim, cgltf_attribute_type_tangent);
                CORE_ASSERT(pos_acc);

                bool should_generate_tangent = false;
                u32 num_verts = (u32)pos_acc->count;
                Array <Vertex> vertices(num_verts);
                Array <u32> indices;
                String material_name;

                for (u32 vi = 0; vi < num_verts; ++vi) {
                    auto& vert = vertices[vi];

                    vert.position = cgltf_read_vec3_from_accessor(pos_acc, vi);

                    if (normal_acc) {
                        vert.normal = cgltf_read_vec3_from_accessor(normal_acc, vi);
                    } else {
                        vert.normal = vec3(0.f, 0.f, 1.f);
                    }

                    if (uv_acc) {
                        vert.uv = cgltf_read_vec2_from_accessor(uv_acc, vi);
                        if (flip_uv) { 
                            vert.uv.y = 1.0f - vert.uv.y;
                        }
                    } else {
                        vert.uv = vec2(0.f, 0.f);
                    }

                    if (tan_acc) {
                        vec4 tangent = cgltf_read_vec4_from_accessor(tan_acc, vi);
                        vert.tangent = tangent;
                    } else {
                        should_generate_tangent = true;
                    }
                }

                // Fetch indices.
                if (prim->indices) {
                    for (cgltf_size i = 0; i < prim->indices->count; ++i) {
                        indices.push_back((uint32)cgltf_accessor_read_index(prim->indices, i));
                    }
                } else {
                    // Make an identity index buffer, that is, identity buffer's always there.
                    CORE_ASSERT(num_verts % 3 == 0);
                    indices.resize(num_verts);
                    for (uint32 i = 0; i < num_verts; ++i) {
                        indices[i] = i;
                    }
                }

                if (prim->material) {
                    for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
                        if (&data->materials[mi] == prim->material) {
                            material_name = materials[mi];
                        }
                    }
                }

                mesh_slice->vertices      = std::move(vertices);
                mesh_slice->indices       = std::move(indices);
                mesh_slice->material_name = material_name;

                // Generate tangents if needed.
                if (should_generate_tangent) {
                    SMikkTSpaceInterface hooks = {
                        .m_getNumFaces          = mikkt_get_num_faces,
                        .m_getNumVerticesOfFace = mikkt_get_num_vertices_of_face,
                        .m_getPosition          = mikkt_get_position,
                        .m_getNormal            = mikkt_get_normal,
                        .m_getTexCoord          = mikkt_get_tex_coord,
                        .m_setTSpaceBasic       = mikkt_set_tspace_basic
                    };

                    SMikkTSpaceContext ctx = {
                        .m_pInterface = &hooks,
                        .m_pUserData  = mesh_slice
                    };

                    CORE_ASSERT(genTangSpaceDefault(&ctx));
                }

                mesh->name = name;
                mesh->meshes.push_back(mesh_slice);
            }

            result.meshes.push_back(mesh);
        }


        // Scene
        //
        {
            u32 num_scenes = (cgltf_size)data->scenes_count;
            CORE_ASSERT(num_scenes == 1);

            for (u32 sci = 0; sci < num_scenes; ++sci) {
                auto* scene = &data->scenes[sci];
                u32 num_nodes = (cgltf_size)scene->nodes_count;

                // DFS
                for (u32 ni = 0; ni < num_nodes; ++ni) {
                    auto* root_node = scene->nodes[ni];
                    Entity* entity = make_entity_from_cgltf_node(world, result.meshes, cgltf_meshes, root_node, nullptr);
                    result.entities.push_back(entity);
                }
            }
        }

        cgltf_free(data);

        return result;
    }
}
