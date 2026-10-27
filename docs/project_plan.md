# Project plan: DLSS 5-style neural rendering in Vulkan compute (no CUDA)

Draft for discussion with the supervisor. Big picture only; details come after approval.

## 1. What DLSS 5 is (from the NVIDIA report)

DLSS 5 is not an upscaler. It is a **generative rendering stage** that runs after the renderer and re-renders the
finished frame so it looks more photographic, while keeping the authored scene intact ("3D-guided neural
rendering"). Key points of the report:

- **One-step, pixel-space diffusion**: one deterministic network evaluation per frame, no VAE/latent space.
- **Conditioning**: the rendered RGB frame, engine motion vectors, carried temporal state (previous output), and
  artist controls (appearance model, structure strength, tone strength, per-pixel masks, auto skin mask).
- **Causal streaming**: one output per input frame, constant memory, no future frames.
- **Backbone**: a transformer of about 154M parameters, with FP8 for most GEMMs and attention and FP16 for the
  sensitive ones, tuned for tensor cores.
- **Training-only supervision**: albedo, normal and lighting consistency keep the output faithful to the scene.
- **Evaluation in the report**: blinded user study (realism), DINOv2 and LPIPS (content preservation), albedo,
  normal and depth comparison (structure), plus speed and VRAM.

The OpenDLSS-NR reference repository documents a concrete 71-block version of this backbone (a U-net of
shifted-window attention blocks with a global ViT at the bottom). It is a good map of the algorithms involved, but
it needs NVIDIA weights that are not available, so it cannot be reproduced end to end.

## 2. Scope: three tracks

| Track | What | Depends on weights? |
| --- | --- | --- |
| A. Core (committed) | Self-written Vulkan compute kernels and blocks, checked against a CPU reference with synthetic weights | No |
| B. Reference model (optional) | Load a model directory in the OpenDLSS-NR format and compare against its published behaviour | Yes, NVIDIA's weights |
| C. Own small model (optional) | Train a small network in PyTorch, run it through the Vulkan kernels, compare with Track B if available | No |

Track A is the thesis. B and C only add comparison. Quality from C is expected to be far below the original; the point
is a documented comparison, not matching NVIDIA.

**Weights and licensing.** The weights are NVIDIA's proprietary model data. OpenDLSS-NR ships none and states that
the user is responsible for the licenses of whatever model data is used. Whether and how they may be obtained and used
for coursework must be cleared with the supervisor before Track B starts. The project must not fail if they are
unavailable.

**Out of scope:** bit-exact parity with NVIDIA unless Track B is approved, and any CUDA/PTX path.

## 3. Pipeline to build

Full diagrams (frame pipeline, implementation layers, track mapping): [architecture.md](architecture.md).

```
rendered frame ─┐
motion vectors ─┼─► preprocess ─► network (small transformer U-net) ─► composite ─► output
previous output ┘   (tone proxy,      window attention + MLP blocks      (residual +
 controls/masks      noise, reproject)  pooling / upsampling / skips       temporal blend)
```

## 4. Algorithms and components

| # | Component | Notes |
| --- | --- | --- |
| 1 | Elementwise ops (residual add, SiLU, scale) | warm-up kernels |
| 2 | Preprocess: tone proxy, noise channels, conditioning channels | one fused kernel |
| 3 | Reprojection of the previous output with motion vectors | bilinear sampling, history validity |
| 4 | Tiled GEMM (fp32, then fp16) | the core of every block |
| 5 | Normalisation and softmax | numerically careful reductions |
| 6 | Window attention (8x8 windows, shifted windows) | window partition, QK^T, softmax, V |
| 7 | MLP block with residual | two GEMMs plus activation |
| 8 | Pool and upsample with skip connections | U-net structure |
| 9 | Optional global attention at the lowest resolution | ViT stage |
| 10 | Composite: residual plus temporal blend, tone and structure strength | final image |
| 11 | Stretch: cooperative matrices (`VK_KHR_cooperative_matrix`), then FP8 | tensor-core speed without CUDA |

## 5. Milestones

| Phase | Deliverable | Status |
| --- | --- | --- |
| 0. Research and setup | Read the DLSS 5 report and OpenDLSS-NR docs; project repo and template (CMake, GLSL, minimal Vulkan host); architecture diagrams ([architecture.md](architecture.md)); plan | Done |
| A. Setup | Vulkan device and extension check; vector-add compute test (template in this repo) |
| B. Basics | Elementwise and preprocess kernels, CPU vs GPU comparison, workgroup-size study |
| C. GEMM | Tiled GEMM, then fp16; compare with CPU and report GFLOPS |
| D. Attention | Softmax, window attention, one full transformer block checked against a CPU reference |
| E. Network | Small U-net with random or self-trained weights running on an image |
| F. Stretch | Cooperative-matrix GEMM, FP8 experiments, optional temporal feedback demo |
| G. Report | Results, limitations, comparison with OpenDLSS-NR's published numbers |

Phases A to D are the commitment. E to F are stretch goals, depending on time.

## 6. Evaluation

- **Correctness**: max and mean error against a CPU reference per kernel and per block, using fixed random inputs.
- **Performance**: milliseconds per kernel and per block across input sizes and workgroup sizes, on the RTX 4070 Ti.
- **Image-level (only if weights exist)**: LPIPS and PSNR/SSIM between input and output, and a simple structure
  check, in the spirit of the report's content-preservation metrics. A user study is out of scope.
- **Honest limits**: without trained weights, output quality is not meaningful; the results are about correct
  and fast Vulkan compute, not image quality.

## 7. Risks

| Risk | Mitigation |
| --- | --- |
| NVIDIA weights unavailable or not licensed for this use | Track A needs none; Track C trains a tiny model |
| Extension support (cooperative matrix, FP8) is driver dependent | Check early (phase A); core path uses plain compute |
| Scope too large | Phases A to D are the minimum; the rest is optional |
| Reference code is MIT but should not be copied | Write kernels independently; cite the repo where it guides the design |

## 8. Questions for the supervisor

1. Is a smaller, self-designed network acceptable instead of reproducing the 71-block model?
2. May I use the NVIDIA weights for coursework (licensing), and is a small PyTorch-trained model acceptable for comparison?
3. Does the stretch use of NVIDIA cooperative-matrix extensions count as "without CUDA"?
