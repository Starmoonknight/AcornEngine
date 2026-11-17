add_rules("mode.debug", "mode.release")
add_rules("plugin.vsxmake.autoupdate")


set_project("Acorn")


-- add_requires("libsdl3")   -- More advanced than glfw? look at later
add_requires("glfw")
add_requires("glm 1.0.3")

-- OpenGL Version
local GL_MAJOR_VERSION = 4
local GL_MINOR_VERSION = 6

-- OpenGL Extensions
local glad_extensions = {
}

-- OpenGL Loader
local gl_api = "gl=" .. GL_MAJOR_VERSION .. "." .. GL_MINOR_VERSION

add_requires("glad v0.1.36", {configs = {
    spec = "gl",
    api = gl_api,
    profile = "core",
    extensions = table.concat(glad_extensions, ","),
    generator = "c",
    loader = true,
}})

-- External dependencies
add_requires("glfw", "glm", "assimp")

target("Acorn")
    -- C++ language version
    set_languages("c++20")
    -- Build an executable
    set_kind("binary")
    -- C++ source files to compile
    add_files("src/*.cpp", "src/geometry/*.cpp", "src/rendering/*.cpp", "src/assets/*.cpp")
    -- Headers to show in Ride
    add_headerfiles("src/*.h", "src/geometry/*.h", "src/rendering/*.h", "src/assets/*.h", {install = false})
    add_headerfiles("inc/*.h", {install = false})
    -- Additional header search directory
    add_includedirs("inc", "src/geometry", "src/rendering", "src/assets")
    add_includedirs("src")
    -- External libraries used by Acorn
    add_packages("glfw")
    add_packages("glm")
    add_packages("glad") 
    add_packages("assimp") 
    -- Other files to display in Rider
    add_extrafiles("shaders/**.vert", "shaders/**.frag")
    add_extrafiles("textures/**.jpg", "textures/**.png")
    
 
after_build(function (target)
    local shaderdir = path.join(target:targetdir(), "shaders")
    os.mkdir(shaderdir)
    os.cp(path.join(os.projectdir(), "shaders", "*"), shaderdir)
    
    local texturedir = path.join(target:targetdir(), "textures")
    os.mkdir(texturedir)
    os.cp(path.join(os.projectdir(), "textures", "*"), texturedir)
    
    local modeldir = path.join(target:targetdir(), "models")
    os.mkdir(modeldir)
    os.cp(path.join(os.projectdir(), "models", "*"), modeldir)
end)

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--

