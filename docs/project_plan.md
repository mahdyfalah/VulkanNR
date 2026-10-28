# Project plan

EC293: recreate a DLSS 5-style neural rendering pipeline with Vulkan compute shaders, without CUDA.
Draft for discussion with the supervisor. Background and diagrams are in [architecture.md](architecture.md).

## 1. Goal

Learn how a neural rendering network works on the GPU by writing the main parts myself in Vulkan compute, and check
each part against a simple CPU version. I am not trying to match NVIDIA's output exactly.

## 2. What I will do

Main work:

- Write my own Vulkan kernels and a small transformer block.
- Test them against a CPU version using random weights.
- Load the OpenDLSS-NR model format and compare with it (needs NVIDIA's weights).
- Train a small model in PyTorch, run it with my kernels and compare. It will look much worse than NVIDIA's, and
  that is expected.

The NVIDIA weights are proprietary and OpenDLSS-NR does not include them, so using them needs the supervisor's
approval. The project works without them.

## 3. Milestones

| # | Milestone | Status |
| --- | --- | --- |
| 1 | Research and project setup (repo, template, plan) | Done |
| 2 | Vulkan basics (GPU check, first compute shader) | Template written, not run yet |
| 3 | In-depth architecture and study of the NVIDIA report | To do |
| 4 | Recreate the OpenDLSS-NR results locally | Built and run with dummy weights; real output needs weights |
| 5 | Start implementation (my own kernels, GPU vs CPU) | To do |
| 6 | Final report | To do |

Later stretch goals, if there is time: a small U-net, cooperative matrices and FP8.

## 4. How I will check it

- **Correct:** the error against the CPU version for each kernel and block, using fixed random inputs.
- **Fast:** milliseconds per kernel and per block for different sizes and workgroup sizes on the RTX 4070 Ti.
- **Image quality:** only if I have trained weights (NVIDIA's or my own), using simple metrics such as PSNR and LPIPS.
  With random weights the image is meaningless, so the results are about correct and fast compute.

## 5. Risks

| Risk | What I do about it |
| --- | --- |
| No access to NVIDIA weights | The main work does not need them; I can train a small model instead |
| Cooperative matrix / FP8 not supported by the driver | Check early; the main path uses plain compute |
| Too much work | Keep to milestones 1 to 5; the rest is optional |
| OpenDLSS-NR code should not be copied | Write my own kernels and cite the repo where it guided the design |

## 6. Questions

1. May I use the NVIDIA weights for coursework (licensing), and is a small PyTorch-trained model acceptable for comparison?
   OpenDLSS-NR is an independent reimplementation that was matched against NVIDIA's own runtime. Its README says it
   is not affiliated with NVIDIA, contains no NVIDIA software or weights, grants no rights under NVIDIA's
   intellectual property, and leaves licensing of the model data to the user. The code itself is MIT. Even if such
   work may be legal, I want to discuss it before building on it.
