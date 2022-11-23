set_languages("c++20", "c17")

add_rules("mode.debug")

add_requires("fmt")
add_requires("spdlog", {configs = {fmt_external = true, debug = true}})
add_requires("vulkan-hpp", {configs = {debug = true}})
add_requires("glfw", {configs = {values = {"vulkan"}, debug = true}})
add_requires("vulkan-loader", {configs = {debug = true}})
add_requires("vulkan-memory-allocator", {configs = {debug = true}})
add_requires("glm")

target("bs_engine_cpp")
    set_kind("static")
    add_files("src/**.cpp")
    add_packages("vulkan-hpp", "vulkan-loader", "vulkan-memory-allocator", "glfw", "fmt", "spdlog", "glm")
    add_includedirs("./include/", "./external/vkfw/include/", "./external/VulkanMemoryAllocator-Hpp/include/", {public = true})
    set_pcxxheader("./external/vkfw/include/vkfw/vkfw.hpp")
    add_defines("VULKAN_HPP_NO_CONSTRUCTORS", "VULKAN_HPP_NO_SPACESHIP_OPERATOR")
    add_cxxflags("-g")
    set_symbols("debug")

includes("./examples/")
