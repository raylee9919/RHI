-- Copyright Seong Woo Lee. All Rights Reserved.

target("Core")
    set_kind("object")
    set_group("Engine")

    add_includedirs(".")
    add_headerfiles("Core_Module.h")
    add_files("Core_Module.cpp")

    add_deps("stb");
