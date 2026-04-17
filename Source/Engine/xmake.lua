-- Copyright Seong Woo Lee. All Rights Reserved.

includes("OS")
includes("Core")
includes("IO")
includes("Window")
includes("Input")
includes("RHI")
includes("Renderer")
includes("Shader")
includes("Asset")

add_includedirs("../ThirdParty/", { public = true })
add_includedirs(".", { public = true })
target("Engine")
    set_kind("object")
    set_group("Engine")

    add_includedirs(".", { public = true })
    add_deps("OS", "Core", "IO", "Window", "Input", "RHI", "Renderer", "Shader", "Asset")
