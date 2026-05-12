-- Copyright Seong Woo Lee. All Rights Reserved.

target("DearIMGUI")
    set_kind("object")
    set_group("ThirdParty")

    add_headerfiles("*.h")
    add_files("*.cpp")

    add_deps("SDL3")
