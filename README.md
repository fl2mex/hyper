# hyper
Making myself a little Vulkan Engine using C++. <br>
I named it hyper because I want to make a fast-ish engine (probably not how it'll turn out haha).

# Progress
I want to completely understand how the whole vulkan graphics pipeline works, so I will not be using any tools such as [vk-bootstrap]. <br>
| <ul><li>- [x] Vulkan Initialisation    | <ul><li>- [ ] The Interesting Stuff    | <ul><li>- [x] Buffer Setup                    | <ul><li>- [ ] Extras                |
|----------------------------------------|----------------------------------------|-----------------------------------------------|-------------------------------------|
| <ul><li>- [x] Window Creation          | <ul><li>- [x] ShaderEXT Creation       | <ul><li>- [x] Framebuffer Setup               | <ul><li>- [ ] Asset System          |
| <ul><li>- [x] Instance Creation        | <ul><li>- [x] Vertex + Fragment Shaders| <ul><li>- [x] Command Buffer Setup            | <ul><li>- [ ] Multithreading        |
| <ul><li>- [x] Debug Messenger Creation | <ul><li>- [ ] Compute Shaders          | <ul><li>- [x] Vertex & Index Buffer Creation  | <ul><li>- [ ] Mipmaps               |
| <ul><li>- [x] Surface Creation         | <ul><li>- [x] Eradication of Pipelines | <ul><li>- [x] Descriptor Layout/Pool Creation | <ul><li>- [ ] Multiple Shader Setup | 
| <ul><li>- [x] Physical Device Picked   | <ul><li>- [ ] ImGUI Implementation     |                                               | <ul><li>- [ ] Post-Processing       |
| <ul><li>- [x] Logical Device Creation  | <ul><li>- [x] Texture Loading          |                                               | <ul><li>- [ ] Raytracing (maybe)    |
| <ul><li>- [x] Queue Setup              | <ul><li>- [ ] GLTF Loading             |                                               | <ul><li>- [ ] Mesh Shaders (maybe)  |
| <ul><li>- [x] Swapchain Setup          | <ul><li>- [ ] Mesh System              |                                               | <ul><li>- [ ] Better Synchronisation|
|                                        | <ul><li>- [ ] Instancing
# Tools used
I am using: <br>
* [GLFW] for window creation <br>
* [Vulkan-Hpp] as a C++ wrapper for the vulkan API

# Learning Resources used
* [Vulkan Tutorial] for in-depth Vulkan setup
* [Vulkan Guide] for a more high-level look into Vulkan
* [Vulkan Registry] for API usage/spec
* [Vulkan Docs] for more API use/spec
* [GIGD Vulkan Project] for a more in-depth look into Vulkan
* [Vulkan Minimal Example] for an example of unique handles

# Building
You can build this on Windows using the included Visual Studio 2022 solution. <br>
I plan to one day learn [CMake] or [Premake], but until then, Windows FTW <br>
Builds may be found [here] sometimes, but I probably won't upload them too often until I am way later in development.

[vk-bootstrap]: https://github.com/charles-lunarg/vk-bootstrap/
[GLFW]: https://github.com/glfw/glfw/
[Vulkan-Hpp]: https://github.com/KhronosGroup/Vulkan-Hpp/
[Vulkan Tutorial]: https://vulkan-tutorial.com/
[Vulkan Guide]: https://vkguide.dev/
[Vulkan Registry]: https://registry.khronos.org/vulkan/specs/1.3/html/
[Vulkan Docs]: https://docs.vulkan.org/spec/latest/
[CMake]: https://cmake.org/
[Premake]: https://premake.github.io/
[here]: https://github.com/fl2mex/hyper/releases/
[GIGD Vulkan Project]: https://github.com/amengede/getIntoGameDev
[Vulkan Minimal Example]: https://github.com/dokipen3d/vulkanHppMinimalExample