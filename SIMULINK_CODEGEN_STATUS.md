# Simulink Code Generation Status (March 26, 2026)

## What was done
- Added automation script: generate_simulink_c.m
- Attempted batch generation for:
  - 8 PSK/simulation_8_psk.slx
  - 64 apsk/simulation_64_apsk.slx
  - 256 apsk/simulation_256_apsk.slx

## Current blocker
Code generation fails because Simulink Coder is not installed in this MATLAB installation.

Observed error:
"Simulink Coder is required for code generation, however it has not been installed."

Additional warnings indicate missing model libraries tied to Communications Toolbox and Spectrum Analyzer blocks.

## Unblock checklist
1. Install product: Simulink Coder.
2. Install product: Communications Toolbox.
3. Install product (one of): DSP System Toolbox / RF Blockset / Simscape (for Spectrum Analyzer blocks).
4. Re-run:
   matlab -batch "generate_simulink_c"

## Generated output folder
- codegen_out/ (currently empty because build is blocked)

## Notes
- Script already comments visualization blocks where possible.
- Manual C propagation phase is implemented in c_port/ and can be built now.
