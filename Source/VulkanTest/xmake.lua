-- Copyright Seong Woo Lee. All Rights Reserved.

target("VulkanTest")
    set_kind("binary")
    set_group("VulkanTest")

    add_includedirs(".")
    add_headerfiles("*.h")
    add_files("*.cpp")

    add_deps("Engine")
