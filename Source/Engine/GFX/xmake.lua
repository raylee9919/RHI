-- Copyright Seong Woo Lee. All Rights Reserved.

target("GFX")
    set_kind("object")
    set_group("Engine")

    add_includedirs(".", { public = true })
    add_headerfiles("*.h")
    add_files("*.cpp")
    
    add_deps("RHI")
