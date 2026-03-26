## Plan: MATLAB Simulink to C Feasibility

Yes, migration is feasible. Recommended path is staged: convert the standalone MATLAB propagation script to hand-written C, and convert Simulink modulation chains using Simulink Coder for speed and lower risk. This prioritizes runtime improvement while preserving model fidelity.

Steps
1. Phase 1 - Baseline and scope lock: Confirm current slowdown points (simulation time per scenario for 8-PSK, 64-APSK, 256-APSK) and freeze acceptance metrics.
2. Phase 2 - Propagation script C port: Rewrite the computations in propagation_losses.m as pure C functions (FSPL + atmospheric + rain components) and remove plotting from compute path.
3. Phase 3 - Simulink C code generation path: Prepare each .slx model for code generation by excluding visualization-only blocks (Scopes/Spectrum analyzers) and generate reusable C functions with Simulink Coder.
4. Phase 4 - C integration harness: Build a small C executable that calls generated model entry points and propagation functions, emits CSV outputs for BER/loss curves, and supports timing benchmarks.
5. Phase 5 - Numerical equivalence validation: Compare C outputs against MATLAB/Simulink references on fixed test vectors and SNR sweeps; require tolerance gates before optimization.
6. Phase 6 - Performance optimization: Profile hotspots (filters, noise generation, symbol mapping), then optimize in C (compiler flags, memory layout, optional SIMD) only after equivalence is met.

Relevant files
- propagation_losses.m - direct source for manual C rewrite of loss equations.
- 8 PSK/simulation_8_psk.slx - 8-PSK chain to generate C from after pruning visualization blocks.
- 64 apsk/simulation_64_apsk.slx - 64-APSK chain for equivalent generation/validation flow.
- 256 apsk/simulation_256_apsk.slx - 256-APSK chain; likely highest runtime cost and validation sensitivity.
- 8 PSK/simulation.slx.original.zip - backup model archive useful if recovery/comparison is needed during preparation.

Verification
1. Reproduce MATLAB baseline runtime and output datasets for all three modulation scenarios and propagation sweep.
2. For propagation, compare per-frequency loss curves between MATLAB and C over 1-60 GHz; enforce max absolute error threshold.
3. For each modulation model, compare BER-vs-SNR curves between Simulink and generated C; enforce agreed tolerance.
4. Confirm generated C builds cleanly with strict warnings enabled and runs without NaN/Inf failures.
5. Benchmark end-to-end runtime and verify measurable speedup over MATLAB/Simulink baseline.

Decisions
- Chosen approach: Fast path using Simulink Coder for modulation models plus manual C for propagation script.
- Primary objective: Runtime speedup over MATLAB/Simulink.
- Included scope: Feasibility, migration sequencing, validation strategy.
- Excluded scope: Full manual rewrite of all modulation DSP blocks from scratch in this first iteration.

Further considerations
1. Toolbox parity risk: gaspl/rainpl are toolbox-backed; if exact equations are needed in pure C, use ITU-R references and validate with independent calculators.
2. Visualization parity: spectrum/scope behavior should be replaced with exported numeric traces and external plotting, not in-core C rendering.
3. If deployment target is embedded, add an extra pass for float32/fixed-point feasibility after correctness is established.