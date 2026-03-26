% MATLAB Script: Earth-to-GEO Path Loss Frequency Sweep (1 GHz to 60 GHz)
% Required Toolboxes: Communications Toolbox, Phased Array System Toolbox
clear; clc;

% Parameters 
dist_m = 35786000;           % GEO Altitude (m) - Approx 35,786 km
freq = (1:0.1:60) * 1e9;     % Frequency Sweep (Hz) - 1 GHz to 60 GHz
rainRate = 31;               % Rain Rate (mm/h) - Heavy rain
temperature = 25;            % Temperature (Celsius)
waterDensity = 7.5;          % Water vapor density (g/m^3)

% Calculate Dry Air Pressure for gaspl
total_pressure = 101325;     % Air pressure at sea level (Pa)
e_vapor = (waterDensity * (temperature + 273.15)) / 2.167; 
dry_pressure = total_pressure - e_vapor; 

c = physconst('LightSpeed');

eff_atm_dist = 10000;  % Effective atmospheric thickness for gas (~10 km)
eff_rain_dist = 5000;  % Effective rain height (~5 km)

% Free Space Path Loss (FSPL)
fspl_loss = fspl(dist_m, c./freq);

% Gas Attenuation 
gas_loss = gaspl(eff_atm_dist, freq, temperature, dry_pressure, waterDensity);

% Rain Attenuation 
rain_loss = rainpl(eff_rain_dist, freq, rainRate);

% Total Loss 
fspl_loss_col = fspl_loss(:);
gas_loss_col  = gas_loss(:);
rain_loss_col = rain_loss(:);
freq_ghz_col  = freq(:) / 1e9;

total_loss_col = fspl_loss_col + gas_loss_col + rain_loss_col;

% Plot Results 
plot(freq_ghz_col, fspl_loss_col, 'Color', '#ff7f0e', 'LineWidth', 2); 
hold on;

% 2. FSPL + Gas 
plot(freq_ghz_col, fspl_loss_col + gas_loss_col, '--', 'Color', [0 0.7 0.9], 'LineWidth', 2);

% 3. Total Loss 
plot(freq_ghz_col, total_loss_col, '-', 'Color', [0.6 0.6 0.6], 'LineWidth', 2);


grid on;
xlabel('Frequency (GHz)');
ylabel('Loss (dB)');
legend('FSPL', 'FSPL + Gas', 'Total Loss (FSPL+Gas+Rain)', 'Location', 'northwest');
title('Earth-to-GEO Propagation Loss (1 - 60 GHz)');