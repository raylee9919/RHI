-- Copyright Seong Woo Lee. All Rights Reserved.

target("Editor")
    set_kind("binary")
    set_group("Editor")

    add_includedirs(".")
    add_headerfiles("*.h")
    add_files("*.cpp")

    add_deps("Engine", "DearIMGUI")
