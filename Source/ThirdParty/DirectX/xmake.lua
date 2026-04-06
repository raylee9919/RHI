-- Copyright Seong Woo Lee. All Rights Reserved.

target("DirectX")
    set_kind("headeronly")
    set_group("ThirdParty")

    add_headerfiles("Include/**.h")
    add_includedirs("Include", { public = true})
