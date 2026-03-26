function generate_simulink_c()
% Batch-generate C code from Simulink models in this assignment.
% Uses Simulink Coder (grt.tlc) and comments visualization blocks.

projectRoot = fileparts(mfilename('fullpath'));
codegenDir = fullfile(projectRoot, 'codegen_out');
cacheDir = fullfile(projectRoot, 'sl_cache');

if ~exist(codegenDir, 'dir')
    mkdir(codegenDir);
end
if ~exist(cacheDir, 'dir')
    mkdir(cacheDir);
end

Simulink.fileGenControl('set', ...
    'CodeGenFolder', codegenDir, ...
    'CacheFolder', cacheDir, ...
    'createDir', true);

models = {
    fullfile(projectRoot, '8 PSK', 'simulation_8_psk.slx')
    fullfile(projectRoot, '64 apsk', 'simulation_64_apsk.slx')
    fullfile(projectRoot, '256 apsk', 'simulation_256_apsk.slx')
};

results = strings(numel(models), 1);

for i = 1:numel(models)
    modelPath = models{i};
    [~, modelName] = fileparts(modelPath);

    fprintf('\n=== Building %s ===\n', modelName);
    try
        if ~isfile(modelPath)
            error('Model file not found: %s', modelPath);
        end

        load_system(modelPath);

        % Comment unsupported visualization blocks to improve codegen success.
        commentBlocksByType(modelName, 'Scope');
        commentBlocksByMaskType(modelName, 'Spectrum Analyzer');
        commentBlocksByMaskType(modelName, 'Time Scope');

        set_param(modelName, 'SystemTargetFile', 'grt.tlc');
        set_param(modelName, 'TargetLang', 'C');
        set_param(modelName, 'GenCodeOnly', 'on');

        slbuild(modelName);
        results(i) = "SUCCESS";
        fprintf('Build succeeded: %s\n', modelName);
    catch ME
        results(i) = "FAILED: " + string(ME.message);
        fprintf(2, 'Build failed: %s\n%s\n', modelName, getReport(ME, 'extended', 'hyperlinks', 'off'));
    end

    if bdIsLoaded(modelName)
        close_system(modelName, 0);
    end
end

fprintf('\n=== Summary ===\n');
for i = 1:numel(models)
    [~, modelName] = fileparts(models{i});
    fprintf('%s: %s\n', modelName, results(i));
end

end

function commentBlocksByType(modelName, blockType)
blocks = find_system(modelName, ...
    'LookUnderMasks', 'all', ...
    'FollowLinks', 'on', ...
    'BlockType', blockType);
for k = 1:numel(blocks)
    try
        set_param(blocks{k}, 'Commented', 'on');
    catch
        % Best effort; continue.
    end
end
end

function commentBlocksByMaskType(modelName, maskType)
blocks = find_system(modelName, ...
    'LookUnderMasks', 'all', ...
    'FollowLinks', 'on', ...
    'MaskType', maskType);
for k = 1:numel(blocks)
    try
        set_param(blocks{k}, 'Commented', 'on');
    catch
        % Best effort; continue.
    end
end
end
