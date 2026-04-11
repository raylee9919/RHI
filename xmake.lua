-- Copyright Seong Woo Lee. All Rights Reserved.

add_rules("mode.debug", "mode.profile", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

set_defaultmode("debug")
set_languages("c++20", { public = true })

set_warnings("all", "extra") 

if is_plat("windows") then
    set_toolset("msvc")
    add_defines("PLATFORM_WINDOWS=1", "_CRT_SECURE_NO_WARNINGS")
end

if is_mode("debug") then
    add_defines("BUILD_DEBUG=1")
    set_symbols("debug", { public = true })
    set_optimize("none", { public = true })
elseif is_mode("profile") then
    set_symbols("debug", { public = true })
    set_optimize("fastest", { public = true })
    set_strip("all", { public = true })
else
    set_symbols("hidden", { public = true })
    set_optimize("fastest", { public = true })
    set_strip("all", { public = true })
end

includes("Source")
