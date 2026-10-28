# Architecture

## What DLSS 5 is

DLSS 5 is not an upscaler like older DLSS. It runs after the game has drawn a frame and re-renders that frame so it
looks more photographic, while keeping the scene as the artists made it. The output has the same resolution as the
input.

What the NVIDIA report says, in short:

- One neural network pass per frame (a one-step diffusion model working directly on pixels).
- Inputs: the rendered frame, motion vectors, the previous output frame, and artist controls (style, structure and
  tone strength, masks).
- It only uses past frames, so it can run live in a game.
- The network is a transformer of about 154M parameters, using FP8 math on tensor cores.
- The report checks quality with a user study and image metrics (LPIPS, DINOv2).

OpenDLSS-NR is an open Vulkan version of this network. Its docs describe a concrete layout (71 blocks); I use it as a
guide for the structure, not as code to copy.

## 1. Frame pipeline

```mermaid
flowchart TD
    C[Rendered frame] --> PRE
    MV[Motion vectors] --> PRE
    AC[Artist controls] --> PRE
    HIST[(Previous output)] --> PRE

    PRE["Preprocess<br/>tone-map the frame, add noise,<br/>reproject previous output"]
    PRE --> NET

    subgraph NET[Network - Vulkan compute]
        direction TB
        ENC["Encoder<br/>window attention blocks, shrink the image"]
        BOT["Bottom<br/>global attention"]
        DEC["Decoder<br/>grow the image again, merge with encoder"]
        HEAD["Output: 3 colour values + 1 blend value per pixel"]
        ENC --> BOT --> DEC --> HEAD
        ENC -. skip connections .-> DEC
    end

    HEAD --> COMP["Composite<br/>add colour change, mix with previous output"]
    COMP --> OUT[Final frame]
    COMP --> HIST
```

Motion vectors are only used to reproject the previous output; they are not an input of the network itself.

## 2. How I will build it

```mermaid
flowchart TD
    subgraph HOST[C++ host]
        L["Load weights"]
        R["Create buffers and pipelines"]
        G["Record the kernels in order with barriers"]
        L --> R --> G
    end

    subgraph GPU[GLSL compute shaders - no CUDA]
        K1[Preprocess]
        K2[Matrix multiply]
        K3[Window attention + softmax]
        K4[MLP]
        K5[Pool / upsample / skip]
        K6[Global attention]
        K7[Output + composite]
    end

    MEM[(Weights, activations, previous frame)]

    G --> GPU
    GPU <--> MEM
```

## 3. Weights

- My own kernels are tested with random weights against the CPU version.
- If allowed, I can load the OpenDLSS-NR weights and compare with the reference.
- If I train a small model myself, I can run it with the same kernels. Its image quality is expected to be poor.

