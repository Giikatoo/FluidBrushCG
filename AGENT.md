# Agent Instructions

## 1. Project Overview

- **Project:** Resolution-Independent Marble Fluid Brush on Vulkan GPU
- **Application:** Interactive 2D fluid brush for a procedural marble texture
- **Language/API/shaders:** C++, Vulkan 1.3+, GLSL compiled to SPIR-V
- **Schedule:** Individual five-week computer-graphics term project
- **Research anchor:** *Mixwell: Sharp 2D Fluid Brushes for Progressive
  Physics-Based Mixing*
- **Official project page:** https://dougjam.github.io/mixwell-2026/

Read these instructions and inspect the existing project before changing
code. Treat the repository's actual implementation and build setup as the
source of truth; do not assume it is empty or replace work without review.

## 2. Goal and Scope

Build a small, reproducible application where a user paints on a 2D
canvas and a GPU-executed fluid-brush operation mixes a procedural
marble texture. The texture should be evaluable at user-selected,
practical output resolutions rather than tied to one pre-baked image.

“Resolution-independent” means evaluating the method at a requested
resolution. It does not mean infinite detail, memory, or performance.
Keep the project correct, understandable, measurable, and achievable
within the schedule; do not expand it into a general painting program.

The project should include:

1. A working C++/Vulkan application with interactive input and display.
2. A meaningful brush-mixing operation executed on the GPU.
3. Procedural marble texture and brush processing that can be evaluated
   separately.
4. A fair baseline, repeatable measurements, and at least two useful
   ablation variables.
5. Build/run instructions and honest reporting of results, limitations,
   sources, licenses, and AI assistance.

Do not add by default: a full fluid simulation, layers, complex
undo/redo, multiple brush families, cloud features, machine learning,
unneeded dependencies, or a complete reproduction of the research paper.

## 3. Research and Attribution

Use Mixwell as the research anchor for a limited, accurately described
fluid-brush or mixing technique. Verify details against the paper,
official supplementary material, or official code before attributing
them to the authors. The project page describes sharp 2D fluid brushes,
progressive physics-based mixing, and resolution-independent evaluation.
Do not invent methods or claim to reproduce the full paper.

The project owner identifies Mixwell as the SIGGRAPH 2026 Best Paper.
Verify the official award record and publication citation before using
that claim in a report or presentation.

Keep these contributions distinct:

- **Research method:** the specific, verified subset adapted from
  Mixwell.
- **Application:** procedural marble texture and interactive painting.
- **Project-specific work:** integration, controls, evaluation, and any
  original texture-generation choices.

If the paper method is not feasible, tell the user before substituting a
different method. Label simplifications and original choices honestly.
Cite research and external assets; follow their licenses. Do not use
unlicensed artwork or imply that generated marble is a paper contribution.

## 4. Working Rules

Before a coding task:

1. Restate the requested outcome briefly when the task is non-trivial.
2. Inspect the repository, relevant source/shader files, documentation,
   and build configuration.
3. Identify the smallest coherent set of changes.
4. Implement the request without unrelated refactors or dependencies.
5. Build, test, or compile affected shaders when possible.
6. Review the change and report what was verified and what remains
   uncertain.

Follow the user's latest explicit instruction unless it conflicts with
correctness, safety, or data integrity. Ask one concise question if an
unresolved choice could cause significant rework; otherwise use a
reasonable, reversible default and state it.

Preserve existing behavior, naming, formatting, and architecture unless
the requested change requires otherwise. Do not silently change the
project direction, algorithm, language, API, or scope. Do not claim
completion or successful tests without evidence.

Inspect the environment before choosing commands. Do not assume the
compiler, operating system, GPU, SDK, driver, or optional Vulkan feature.
If build or runtime verification is unavailable, say so and identify
what was not tested.

## 5. Architecture and Responsibilities

Follow the existing architecture. A suitable conceptual pipeline is:

1. Input: mouse position/state, brush radius, and texture parameters.
2. CPU: validate and pack small parameter data; manage resources and
   command submission.
3. GPU: evaluate/update the canvas and execute the meaningful brush
   mixing operation.
4. Render/present the result.
5. Measure relevant GPU passes and export repeatable experiment data.

The CPU may handle application setup, input, resource lifetime,
synchronization, and required readback. Do not move the entire core
operation to the CPU and use Vulkan only to display its output.

Possible organization, only when it fits the repository:

- `src/app/`: application, window, and input
- `src/vulkan/`: Vulkan setup, resources, submission, and timing
- `src/brush/`: brush state and mixing coordination
- `src/texture/`: procedural marble parameters or generation
- `shaders/`: GLSL shaders
- `tests/`, `scripts/`, `docs/`: only when useful

Keep the structure small. Do not create suggested directories
pre-emptively.

## 6. Vulkan and Shader Requirements

- Target Vulkan 1.3+ and GLSL/SPIR-V as required by the project.
- Document a reproducible shader compilation process.
- Query device support before enabling optional features or extensions.
- Keep descriptor layouts, push constants, shader declarations, and host
  data structures consistent; verify sizes and alignment.
- Give each resource a clear owner and destruction path. Destroy
  resources only after their GPU use is complete.
- Use correct image layouts, barriers, access/stage masks, and
  queue-family ownership where applicable.
- Ensure compute writes are visible to later compute, graphics, transfer,
  or host accesses as needed.
- Check Vulkan results and report failures with useful context.
- Handle resize/swapchain recreation when supported. Avoid device-wide
  idle waits in the normal frame loop unless justified.
- Keep validation enabled during development. Investigate validation
  errors and relevant warnings; do not suppress them to hide problems.

For shaders and algorithms:

- Use clear names and explain non-obvious math or synchronization.
- Define coordinate spaces, units, normalization, boundaries, and color
  representation.
- Guard against invalid values, out-of-range access, and canvas-edge
  errors.
- Separate marble-field evaluation from brush mixing so each can be
  tested independently.
- Optimize only after measuring.

For the marble texture:

- Prefer a procedural field or licensed input, not a single fixed-size
  raster as the core representation.
- Use explicit, deterministic parameters for experiments unless
  randomness is under test.
- Define the field's coordinate and sampling behavior across output
  resolutions.
- Treat resolution as a resource/performance variable, not a claim of
  unlimited physical detail.

For image handling, document color space, texture format, and alpha
convention. Use consistent settings in processing, rendering, and
comparisons.

## 7. Evaluation and Performance

Define a fair baseline before claiming improvement. Compare methods
using the same hardware, input sequence, initial state, and settings
where feasible. State exactly what differs.

Use at least two ablation variables that affect the implemented method.
Candidate variables include output resolution and stroke/workload
complexity. When changing resolution, keep the procedural domain, brush
path, and physical feature scale consistent; record dimensions and
resource costs.

Measure GPU work with Vulkan timestamps or another justified GPU timing
method. Report per-pass GPU time where applicable; CPU frame time/FPS is
not a substitute. Warm up, repeat measurements, report an appropriate
summary (for example median and spread), and record GPU, driver,
resolution, workload, and relevant settings. Avoid reading unavailable
query results or adding accidental stalls.

Choose quality metrics for the actual output. PSNR/SSIM require a defined
reference and consistent preprocessing/color space. Task-specific
metrics are acceptable when their calculation is precise and
reproducible. For resolution comparisons, map samples to a common domain
or reference. Visual examples do not replace required quantitative
evaluation.

Record observed failures, artifacts, instability, lost detail, and
resolution-dependent behavior. Separate measured evidence from
hypotheses; do not promise results before measurement.

## 8. Build, Tests, and Documentation

Preserve the existing build system. Use its CMake or other established
workflow. Keep dependencies minimal, avoid machine-specific absolute
paths, and do not commit generated binaries, build directories, captures,
or large data unless the project requires them.

Run the smallest relevant checks available:

- Build after meaningful C++ changes and compile modified shaders.
- Run relevant tests and Vulkan validation when available.
- Test initialization, normal rendering, brush interaction, supported
  resize behavior, and clean shutdown.
- Check useful edge cases: zero-length strokes, fast movement, canvas
  boundaries, and extreme supported parameters.
- Use deterministic inputs for regression and performance experiments.
- Compare with a CPU reference for mathematical subproblems when useful.

Report tests only as actually run. If verification was unavailable,
label it **Not run** and explain why.

Keep README and related documentation accurate. The README should
eventually state supported systems, compiler/SDK requirements,
dependencies, build/run commands, shader compilation, controls,
limitations, and research references.

Expected final course materials may include a 4–6 page ACM `acmart`
report, results and ablations, a demo, and a project page. Include the
implemented subset, experimental setup/hardware, limitations,
attribution, and AI-use statement. Prioritize the working application
and reliable evaluation over presentation polish.

For substantive AI-assisted work, record files/components changed, the
purpose, checks performed, assumptions, and unresolved issues. The
student should be able to explain the code and technical decisions.
Never fabricate citations, licenses, performance, test results, or
hardware capabilities.

## 9. Code Style and Completion

- Use descriptive names, focused functions, and existing conventions.
- Prefer RAII for C++ resource ownership where appropriate.
- Handle errors explicitly; name constants instead of unexplained magic
  numbers.
- Keep comments for reasoning, invariants, synchronization, and
  non-obvious mathematics.
- Avoid dead code, duplicate implementations, unexplained warning
  suppression, and abstractions that obscure Vulkan behavior.

A task is complete when the requested behavior is implemented, changes
stay in scope, relevant checks have run where possible, documentation is
updated when needed, and known risks are reported. The overall project
also needs a working GPU demo, baseline, per-pass measurements, two
meaningful ablations, quality evaluation, reproducible build steps, and
honest final materials.

If asked to report a coding task, use:

### Summary
What changed.

### Files changed
Files and purpose.

### Technical decisions
Important algorithm, shader, resource, or synchronization choices.

### Verification
Commands/tests actually run and outcomes; label anything not run.

### Remaining risks
Known limitations or assumptions.

If the user requests another format, follow that format.
