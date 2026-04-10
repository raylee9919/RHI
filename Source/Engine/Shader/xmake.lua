-- Copyright Seong Woo Lee. All Rights Reserved.

includes("DXIL")

target("Shader")
    set_kind("object")
    set_group("Engine")

    add_headerfiles("*.h")
    add_files("*.cpp")

    add_deps("DXIL")
