# hyper
Making myself a little Vulkan Engine using C++. <br>
I named it hyper because I want to make a fast-ish engine (probably not how it'll turn out haha).

# Progress
I want to completely understand how the whole vulkan graphics pipeline works, so I will not be using any tools such as [vk-bootstrap]. <br>
| <ul><li>- [ ] Vulkan Initialisation    | <ul><li>- [ ] The Interesting Stuff    | <ul><li>- [ ] Extras                    |
|----------------------------------------|----------------------------------------|-----------------------------------------|
| <ul><li>- [x] Window Creation          | <ul><li>- [x] ShaderEXT Creation       | <ul><li>- [ ] Deferred Rendering        |
| <ul><li>- [x] Instance Creation        | <ul><li>- [x] Eradication of Pipelines | <ul><li>- [ ] Asset System              |
| <ul><li>- [x] Extension Setup          | <ul><li>- [x] Buffer Class             | <ul><li>- [ ] Multiple Shader Setup     |
| <ul><li>- [x] Device Handling          | <ul><li>- [x] Image Class              | <ul><li>- [ ] Post-Processing           | 
| <ul><li>- [x] Queues                   | <ul><li>- [ ] Mesh Class               | <ul><li>- [ ] Material System           |
| <ul><li>- [x] Swapchain                | <ul><li>- [ ] Compute Shaders          | <ul><li>- [ ] Raytracing (maybe)        |
| <ul><li>- [x] Buffers                  | <ul><li>- [x] ImGUI Implementation     | <ul><li>- [ ] Meshlet Rendering (maybe) |
| <ul><li>- [x] Textures                 | <ul><li>- [ ] Instancing               |
| <ul><li>- [ ] GLTF Loading             | <ul><li>- [ ] Multithreading           |
|                                        | <ul><li>- [ ] Mipmaps                  |
# Tools used
I am using: <br>
* [GLFW] for window creation
* [Vulkan-Hpp] as a C++ wrapper for the vulkan API
* [stb_image] from stb for image loading
* [VulkanMemoryAllocator] for memory management
* [ImGui] for a simple GUI

# Learning Resources used
* [Vulkan Tutorial] for in-depth Vulkan setup
* [Vulkan Guide] for a more high-level look into Vulkan
* [Vulkan Registry] for API usage/spec
* [Vulkan Docs] for more API use/spec
* [VMA Docs]  for API use/spec for VulkanMemoryAllocator
* [GIGD Vulkan Project] for a more in-depth look into Vulkan
* [Sascha Willems' Vulkan Examples] for examples of specific Vulkan features
* [Vulkan Minimal Example] for an example of unique handles
# Building
You can build this on Windows using the included Visual Studio 2022 solution. <br>
I plan to one day use [CMake] or [Premake], but until then, VS2022 FTW <br>
Builds may be found [here] sometimes, but I probably won't upload them too often until I am way later in development.
# Licenses from the tools used
It's probably a good idea to put the licenses of the tools used in this project here. <br>
All code produced is under the GPL-3.0 License, except for the projects listed below: <br>
* [GLFW] is licensed under the [zlib/libpng] license, and the license file is in [vendor/GLFW]. <br>
* [stb_image] is both in the public domain ([Unlicense]) and underneath the [MIT License], and the license file is in [vendor/stb]. <br>
* [ImGui] is licensed under the [MIT License], and the license file is in [vendor/imgui]. <br>
* [Vulkan-Hpp] is licensed under the [Apache License 2.0], and the license is found online [here](https://github.com/KhronosGroup/Vulkan-Hpp/blob/main/LICENSE.txt). <br>
* [VulkanMemoryAllocator] is licensed under the [MIT License], and the license is found online [here](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/blob/master/LICENSE.txt). <br>
# Good things to know/bookmark
* [VkResult spec] - Good to remember/check when finding errors
* [VK_KHR_dynamic_rendering tutorial] - Used this extension early on, good stuff
* [VK_EXT_shader_object tutorial] - Good, but only a 11% implementation rate on gpus
* [Buffer device addresses in Vulkan and VMA] - Another good extension
* [Managing bindless descriptors in Vulkan] - Not used yet
# Known Problems
* Some Intel GPUs do not support nearest sampling, unknown why
* ImGui not rendering on devices that rely on the ShaderEXT emulation fallback
* If anyone finds more problems or bugs, please create an issue.

[vk-bootstrap]: https://github.com/charles-lunarg/vk-bootstrap/
[CMake]: https://cmake.org/
[Premake]: https://premake.github.io/
[here]: https://github.com/fl2mex/hyper/releases/

[GLFW]: https://github.com/glfw/glfw/
[Vulkan-Hpp]: https://github.com/KhronosGroup/Vulkan-Hpp/
[stb_image]: https://github.com/nothings/stb/blob/master/stb_image.h
[VulkanMemoryAllocator]: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
[ImGui]: https://github.com/ocornut/imgui

[Vulkan Tutorial]: https://vulkan-tutorial.com/
[Vulkan Guide]: https://vkguide.dev/
[Vulkan Registry]: https://registry.khronos.org/vulkan/specs/latest/registry.html
[Vulkan Docs]: https://docs.vulkan.org/spec/latest/
[VMA Docs]: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html
[GIGD Vulkan Project]: https://github.com/amengede/getIntoGameDev/tree/main/vulkan
[Sascha Willems' Vulkan Examples]: https://github.com/SaschaWillems/Vulkan
[Vulkan Minimal Example]: https://github.com/dokipen3d/vulkanHppMinimalExample

[zlib/libpng]: https://zlib.net/zlib_license.html
[MIT License]: https://opensource.org/license/MIT
[Unlicense]: https://unlicense.org/#the-unlicense
[Apache License 2.0]: https://www.apache.org/licenses/LICENSE-2.0
[vendor/GLFW]: vendor/GLFW/LICENSE.md
[vendor/stb]: vendor/stb/LICENSE.txt
[vendor/imgui]: vendor/imgui/LICENSE.txt

[VkResult spec]: https://registry.khronos.org/vulkan/specs/latest/man/html/VkResult.html
[VK_KHR_dynamic_rendering tutorial]: https://lesleylai.info/en/vk-khr-dynamic-rendering/
[VK_EXT_shader_object tutorial]: https://docs.vulkan.org/samples/latest/samples/extensions/shader_object/README.html
[Managing bindless descriptors in Vulkan]: https://dev.to/gasim/implementing-bindless-design-in-vulkan-34no
[Buffer device addersses in Vulkan and VMA]: https://dev.to/gasim/buffer-device-addresses-in-vulkan-and-vma-7ne