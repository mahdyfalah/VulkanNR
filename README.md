# VulkanNR

EC293 - Recreate DLSS: a DLSS 5-style neural rendering pipeline in **Vulkan compute, without CUDA**.

Reference: [OpenDLSS-NR](https://github.com/maanHimself/OpenDLSS-NR) and the
[NVIDIA DLSS 5 report](https://research.nvidia.com/labs/adlr/files/DLSS5_Report.pdf).

**Status:** planning, awaiting supervisor approval. See [docs/project_plan.md](docs/project_plan.md).

## Goal

Learn how a real-time generative rendering network is built, and recreate a small, testable version of it with Vulkan
compute shaders, checking every piece against a CPU reference.

Three tracks: **A** own kernels with synthetic weights (the committed core), **B** optionally load the OpenDLSS-NR
model format if suitably licensed weights are available, **C** optionally train a small model myself for comparison
(expected to be far below the original). The weights are NVIDIA's proprietary data, so B needs the supervisor's
approval and the project does not depend on it.

## What DLSS 5 does (short)

It is a one-step, pixel-space diffusion transformer that re-renders a finished game frame to look more photographic.
It takes the rendered frame, motion vectors, the previous output and artist controls, and runs causally, one frame at
a time, with FP8 tensor-core math.

## Architecture

Frame pipeline, implementation layers and the mapping to tracks are in [docs/architecture.md](docs/architecture.md).

## Plan

| Phase | Content |
| --- | --- |
| 0 (done) | Research, repo and template, architecture diagrams, plan |
| A | Vulkan setup and a vector-add compute test (this template) |
| B | Elementwise, preprocess and reprojection kernels; CPU vs GPU; workgroup sizes |
| C | Tiled GEMM (fp32, fp16) |
| D | Softmax, window attention, one transformer block |
| E | Small U-net on an image (stretch) |
| F | Cooperative matrices / FP8 (stretch) |

## Success criteria

- A working Vulkan compute prototype.
- At least one transformer-style block checked against a CPU reference.
- Timing results across sizes and workgroup sizes.
- A written account of what could and could not be reproduced, including the effect of missing weights.

## Build and run

Needs the Vulkan SDK (for `glslc`), CMake 3.19+ and Visual Studio 2022.

```
cmake -S . -B build
cmake --build build --config Release
build\Release\vulkannr.exe
```

It prints the GPU, runs `vector_add` on the GPU and checks it against the CPU.

## Layout

```
docs/project_plan.md   plan, algorithms, evaluation, risks
docs/architecture.md   pipeline and implementation diagrams
shaders/               GLSL compute shaders
src/main.cpp           minimal Vulkan compute host
```

## Progress log

| Date | Progress | Next step |
| --- | --- | --- |
| 2026-10-26 | Reviewed the reference repo; GPU detected | Check Vulkan support, run the first compute test |
| 2026-10-27 | Read the DLSS 5 report; wrote plan and template | Supervisor approval; run the template |
