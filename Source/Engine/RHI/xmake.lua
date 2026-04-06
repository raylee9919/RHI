-- Copyright Seong Woo Lee. All Rights Reserved.

includes("D3D12")

target("RHI")
    set_kind("object")
    set_group("Engine")

    add_includedirs(".", { public = true })
    add_headerfiles("*.h")
    --add_files("*.cpp")
    
    add_deps("D3D12")
