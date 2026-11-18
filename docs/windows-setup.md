# Set up Acorn on Windows with Rider

Last updated: **2026-11-18**.

## Prerequisites

Install Git, xmake, Rider, and Visual Studio or its Build Tools with the **Desktop development with C++** workload. The development setup uses Visual Studio 2026. The generated solution directory can vary with the installed toolchain/xmake version; use the path reported by generation rather than assuming its year.

Obtain the full project, including `stb_image.h`, shaders, textures, and models. The graphics driver must support the requested OpenGL 4.6 context.

## Build and generate the solution

1. Open PowerShell in the folder containing `xmake.lua`.
2. Build:

   ```powershell
   xmake
   ```

   Allow installation of the declared dependencies when prompted. A first build may download/build packages. Check for a successful build before treating setup as complete.

3. Generate the solution:

   ```powershell
   xmake project -k vsxmake
   ```

   Generation may need packages for additional build configurations. Read the output for errors and the generated solution path.

4. In Rider, open that `.sln`, for example `vsxmake2026\Acorn.sln`. To find generated solutions without relying on that folder name:

   ```powershell
   Get-ChildItem -Path . -Filter Acorn.sln -Recurse |
       Select-Object -ExpandProperty FullName
   ```

5. Build/run the `Acorn` target. Set the run configuration's working directory to the project root so relative asset paths resolve. Alternatively, from the project root:

   ```powershell
   xmake run Acorn
   ```

## Recovering a missing solution

Regenerate with `xmake project -k vsxmake` and reopen the reported `.sln`. Generated files are not a substitute for the source `xmake.lua` configuration.

After changing package requirements, regenerate if needed. The project also enables the `plugin.vsxmake.autoupdate` rule.

## Source discovery and dependencies

`src/**.cpp` recursively includes implementation files. Header search roots are `src` and `inc`; cross-folder includes use paths such as `geometry/CpuMeshData.h`. A neighbouring header can still be included relative to its source file.

`add_requires` declares packages and their configuration. `add_packages` applies them to the `Acorn` target. Keep GLM's `1.0.3` requirement and GLAD's configured `v0.1.36` requirement when reproducing this setup. Do not add a second unversioned GLAD requirement.

## Project documentation

Keep `README.md` beside `xmake.lua`, and keep `architecture.md` and this `windows-setup.md` in a `docs` folder beside `src`. The relative links assume this layout. These are plain text files, not compiled sources; opening or editing them needs no build-system change.

## Diagnosing common problems

| Symptom | Check |
| --- | --- |
| `xmake` is not recognized | Installation/PATH; reopen PowerShell after changing PATH |
| Compiler not found | Visual Studio C++ workload and xmake's detected toolchain |
| `stb_image.h` missing | Header exists in an included directory such as `inc` |
| Asset copy step fails | Source `shaders`, `textures`, and `models` folders exist |
| Shader/model/image cannot be opened | Process working directory and relative path; upload wrappers print resolved model paths |
| Window/context creation fails | GLFW diagnostic output and OpenGL 4.6 driver support |
| Rider points to missing `.sln` | Regenerate and reopen the newly reported path |

Official references: [xmake package setup](https://xmake.io/guide/project-configuration/add-packages.html), [project generators](https://xmake.io/guide/extensions/builtin-plugins.html), [toolchain selection](https://xmake.io/guide/basic-commands/switch-toolchains.html).
