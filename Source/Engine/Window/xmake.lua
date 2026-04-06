-- Copyright Seong Woo Lee. All Rights Reserved.

target("Window")
    set_kind("object")
    set_group("Engine")

    add_headerfiles("*.h")
    add_files("*.cpp")

    add_deps("SDL3")
