-- Copyright Seong Woo Lee. All Rights Reserved.

includes("DX12", "VK")

target("RHI")
    set_kind("object")
    set_group("Engine")

    add_includedirs(".", { public = true })
    add_headerfiles("*.h")
    --add_files("*.cpp")

    add_deps("DX12", "VK")
