-- Copyright Seong Woo Lee. All Rights Reserved.

target("DXC")
    set_kind("headeronly")
    set_group("ThirdParty")

    add_headerfiles("Include/**.h")
    add_includedirs("Include", { public = true})
    add_linkdirs("Lib/x64", { public = true })

    if is_plat("windows") then
        add_syslinks("dxcompiler.lib", "dxil.lib", { public = true })
    end
