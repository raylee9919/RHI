-- Copyright Seong Woo Lee. All Rights Reserved.

target("SDL3")
    set_kind("headeronly")
    set_group("ThirdParty")

    add_headerfiles("Include/**.h")
    add_includedirs("Include", { public = true})
    add_linkdirs("Lib", { public = true })

    if is_plat("windows") then
        add_syslinks("SDL3.lib", { public = true })
    end
