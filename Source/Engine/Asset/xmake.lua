-- Copyright Seong Woo Lee. All Rights Reserved.

target("Asset")
    set_kind("object")
    set_group("Engine")

    add_includedirs(".")
    add_headerfiles("*.h")
    add_files("*.cpp")

    add_deps("cgltf");
