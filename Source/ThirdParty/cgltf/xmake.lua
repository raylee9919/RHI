-- Copyright Seong Woo Lee. All Rights Reserved.

target("cgltf")
    set_kind("object")
    set_group("ThirdParty")

    add_includedirs("Include")
    add_headerfiles("Include/*.h")
    add_files("cgltf.cpp")
