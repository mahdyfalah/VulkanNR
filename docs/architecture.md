# Architecture

Same-resolution neural rendering (not upscaling). Model structure follows the OpenDLSS-NR documentation; the DLSS 5 report describes the concept only.

## 1. Frame pipeline

```mermaid
flowchart TD
    subgraph IN[Renderer inputs]
        C[HDR colour]
        MV[Motion vectors]
        AC[Artistic controls]
    end

    C --> PRE
    MV --> PRE
    AC --> PRE
    HIST[(History: previous output)] --> PRE

    PRE["Preprocess + reprojection<br/>proxy, reprojected history, 3 noise lanes,<br/>style/tone/conditioning, padded to field<br/>16 f32 lanes per pixel (motion is not a lane)"]
    PRE --> NET

    subgraph NET[Neural network - Vulkan compute]
        direction TB
        ENC["Encoder<br/>shifted-window transformer blocks<br/>pool across levels"]
        BOT["Bottleneck<br/>global ViT attention"]
        DEC["Decoder<br/>upsample + skip merges<br/>shifted-window blocks"]
        HEAD["Head: 4 ch/pixel<br/>RGB residual + blend logit"]
        ENC --> BOT --> DEC --> HEAD
        ENC -. skip connections .-> DEC
    end

    HEAD --> COMP["Composite<br/>proxy + residual, blended with reprojected history"]
    COMP --> OUT[Enhanced frame]
    COMP --> HIST
```

## 2. Implementation layers

```mermaid
flowchart TD
    subgraph HOST[Host - C++ / Vulkan]
        L["Load manifest + weights<br/>validate shapes and hashes"]
        R["Create buffers, descriptors,<br/>pipelines, activation arena"]
        G["Record model graph<br/>barriers between dispatches"]
        L --> R --> G
    end

    subgraph GPU[Compute kernels - GLSL to SPIR-V, no CUDA/PTX]
        K1[Preprocess / format conversion]
        K2[GEMM / projections]
        K3[QKV + window attention + softmax]
        K4[MLP + SiLU]
        K5[Pool / upsample / skip merge]
        K6[Global attention]
        K7[Head + temporal composite]
    end

    subgraph MEM[GPU memory]
        W[(Weights)]
        A[(Activation arena)]
        H[(Frame history)]
    end

    G --> GPU
    GPU <--> MEM
```

## 3. Mapping to project tracks

| Piece | Track A (own kernels, synthetic weights) | Track B (external weights) | Track C (own small model) |
|---|---|---|---|
| Kernels in section 2 | implement and verify vs CPU | reuse | reuse |
| Weight loader | simple format | OpenDLSS-NR manifest | own format |
| Output quality | not evaluated | compare to reference | expected poor |

## Notes

- Precision: the reference uses FP8 E4M3 activations with FP16 accumulation and needs cooperative-matrix and float8 extensions. Portable FP16/FP32 kernels come first; FP8 is a stretch goal.
- A single-frame run is not the full temporal loop; history and reprojection must be implemented for temporal evaluation.
- Claim only what is tested: no cross-vendor support without running on other GPUs.
