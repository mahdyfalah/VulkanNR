# Vulkan Neural Rendering --- Research Plan

**Status:** Initial investigation\
**Target:** End of semester\
**Reference:** [OpenDLSS-NR](https://github.com/maanHimself/OpenDLSS-NR)

## 1. Project goal

The goal is to learn how neural rendering works and recreate a small,
testable part of OpenDLSS-NR using **Vulkan compute shaders**, without
depending on CUDA.

Rather than promising a complete reproduction immediately, this project
will first build a working Vulkan prototype, then investigate how
selected neural-network operations can be implemented and tested.

## 2. Current status

-   Reviewed the OpenDLSS-NR repository and its build and weight
    documentation.
-   Confirmed Windows 11 detects an **NVIDIA RTX 4070 Ti with 12 GB
    VRAM** using `nvidia-smi`.
-   The Vulkan extension support has not yet been checked.
-   The project has not yet been built or run.
-   Compatible pretrained weights have not been obtained. The reference
    repository does not include them.

**Important limitation:** Without the correct weights, the original
trained network cannot be reproduced end-to-end. The project can still
investigate Vulkan compute and test individual operations using small
synthetic inputs.

## 3. Research questions

1.  Can I create a reliable Vulkan compute pipeline on my hardware?
2.  How do GPU results compare with a simple CPU implementation of the
    same operation?
3.  How do different workgroup sizes affect execution time?
4.  Which parts of the reference implementation can realistically be
    recreated during the semester?

## 4. Planned experiments

  -----------------------------------------------------------------------
  Experiment              Method                  What I will measure
  ----------------------- ----------------------- -----------------------
  1\. Vulkan setup        Run a basic compute     Correct output and
                          shader, such as vector  successful execution.
                          addition.               

  2\. CPU vs. GPU         Run the same operation  Output differences and
                          on CPU and GPU with     execution time.
                          several input sizes.    

  3\. Workgroup sizes     Repeat the GPU test     Execution time and any
                          with different          correctness issues.
                          workgroup sizes.        

  4\. Neural-network      Implement one           Correctness against a
  operation               manageable operation    CPU reference and
                          from the reference      performance.
                          code, if feasible.      
  -----------------------------------------------------------------------

These experiments use synthetic data and do **not** claim to reproduce
the full neural renderer.

## 5. Next steps

1.  Check the Vulkan extensions supported by the RTX 4070 Ti and
    installed driver.
2.  Build a minimal Vulkan compute program.
3.  Validate its results against a CPU version.
4.  Study the reference implementation and choose one operation to
    recreate.
5.  Record timings, results, screenshots, and problems in this README.

The original project build instructions are available in the [upstream
README](https://github.com/maanHimself/OpenDLSS-NR#readme). The full
demo also depends on a compatible model and weights; see the [weight
documentation](https://github.com/maanHimself/OpenDLSS-NR/blob/main/docs/weights.md).

## 6. Success criteria

By the end of the semester, I aim to have:

-   A working Vulkan compute prototype.
-   At least one operation tested against a CPU reference.
-   A small comparison of execution times across input sizes or
    workgroup sizes.
-   A documented explanation of what could and could not be reproduced,
    including the impact of missing model weights.

## 7. Progress log

  -----------------------------------------------------------------------
  Date                    Progress                Next step
  ----------------------- ----------------------- -----------------------
  2026-10-26              Reviewed the            Check Vulkan support
                          repository; confirmed   and run the first
                          the GPU is detected.    compute test.

  -----------------------------------------------------------------------

## References

-   [OpenDLSS-NR repository](https://github.com/maanHimself/OpenDLSS-NR)
-   [Weight
    format](https://github.com/maanHimself/OpenDLSS-NR/blob/main/docs/weights.md)
-   [Vulkan documentation](https://docs.vulkan.org/)
-   [Vulkan Tutorial](https://vulkan-tutorial.com/)
