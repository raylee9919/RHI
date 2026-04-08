-- Copyright Seong Woo Lee. All Rights Reserved.

target("DX12")
    set_kind("object")
    set_group("Engine")

    add_includedirs(".", { public = true })
    add_headerfiles("*.h")
    add_files("*.cpp")

    if is_plat("windows") then
        add_syslinks("d3d12", "dxgi", "dxguid", { public = true })
    end

    add_deps("DirectX")
