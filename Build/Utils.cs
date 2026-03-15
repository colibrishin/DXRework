using System;
using System.IO;
using System.Reflection;
using Sharpmake;

public class Utils
{
    public static Solution.Configuration GetInstantiatedSolutionConfiguration(ITarget target, params Solution[] solutions)
    {
        Solution.Configuration solutionConfiguration = new Solution.Configuration();

        foreach (Solution solution in solutions)
        {
            Type derivedSolutionType = solution.GetType();
            MethodInfo methodInfo = derivedSolutionType.GetMethod("ConfigureAll");
            methodInfo.Invoke(solution, new object[] { solutionConfiguration, target });
        }

        return solutionConfiguration;
    }

    // GenerateSolution.bat 에서 지정됩니다.
    public static string GetEngineDir()
    {
        return Environment.GetEnvironmentVariable("EngineDir");
    }
    // GenerateSolution.bat 에서 지정됩니다.
    public static string GetSolutionDir()
    {
        return Environment.GetEnvironmentVariable("SharpMakeSolutionDir");
    }
    // EngineSolution ConfigureAll에서 지정 됩니다. (vcxproj 생성 위치)
    public static string GetProjectDir()
    {
        return Environment.GetEnvironmentVariable("ProjectFilesDir");
    }

    public static void MakeConfiturationNameDefine(Solution.Configuration conf, EngineTarget target)
    {
        string outputName = "";

        if (target.Optimization == Optimization.Release) 
        {
            outputName = "Development"; 
        }
        else
        {
            outputName = "Debug";
        }

        // Conf Name
        {
            if (target.LaunchType == ELaunchType.Editor)
            {
                outputName += "Editor";
            }
            else if (target.LaunchType == ELaunchType.Client)
            {
                outputName += "Client";
            }
            else if (target.LaunchType == ELaunchType.Server)
            {
                outputName += "Server";
            }
        }

        conf.Name = outputName;
    }

    public static EngineTarget GetDefinedTarget() 
    {
        return new EngineTarget(
            ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server,
            Platform.win64,
            DevEnv.vs2026,
            Optimization.Debug | Optimization.Release,
            OutputType.Lib,
            EGraphicAPI.D3D12,
            ERenderType.Deferred,
            ERaytracing.On,
            ESoundInterface.FMOD
        );
    }

    /// <summary>
    /// Adds solution-level prebuild steps to conf: build balius (release), then run GenerateSolution.bat.
    /// Use on entry-point projects (e.g. Launch, Monolith).
    /// </summary>
    public static void AddSolutionPrebuildSteps(Project.Configuration conf)
    {
        string solutionDir = GetSolutionDir();
        string engineDir = GetEngineDir();
        if (string.IsNullOrEmpty(engineDir))
            engineDir = solutionDir;

        string baliusDir = Path.Combine(engineDir, "balius");
        if (Directory.Exists(baliusDir))
        {
            // FastBuild resolves ExecExecutable relative to BFF dir, so "cargo" would look for I:\...\cargo. Use full path when available (rustup default).
            string cargoExe = "cargo";
            string cargoHome = Environment.GetEnvironmentVariable("CARGO_HOME");
            if (!string.IsNullOrEmpty(cargoHome))
            {
                string cargoPath = Path.Combine(cargoHome, "bin", "cargo.exe");
                if (File.Exists(cargoPath))
                    cargoExe = cargoPath;
            }
            if (cargoExe == "cargo")
            {
                string userCargo = Path.Combine(Environment.GetEnvironmentVariable("USERPROFILE") ?? "", ".cargo", "bin", "cargo.exe");
                if (File.Exists(userCargo))
                    cargoExe = userCargo;
            }

            // FastBuild Exec() requires .ExecOutput; use a log path so the BFF is valid.
            string buildBaliusLog = Path.Combine(engineDir, "Intermediate", "log", "BuildBalius.log");
            var buildBalius = new Project.Configuration.BuildStepExecutable(
                cargoExe,
                "",
                buildBaliusLog,
                "build --release",
                baliusDir,
                true,
                true
            );
            buildBalius.FastBuildAlwaysShowOutput = true;
            buildBalius.FastBuildExecAlways = true;
            conf.EventCustomPrebuildExecute.Add("BuildBalius", buildBalius);
        }
    }

    public static void AddDefines(Project.Configuration conf, EngineTarget target)
    {
        if (target.Platform == Platform.win64 || target.Platform == Platform.win32)
        {
            conf.Defines.Add("WIN32_LEAN_AND_MEAN");
            conf.Defines.Add("PLATFORM=Windows");
        }

        conf.Defines.Add("NOMINMAX=1");
        
        if (target.GraphicAPI == EGraphicAPI.D3D12) 
        {
            conf.Defines.Add("USE_D3D12");
            conf.AddPublicDependency<DirectXTK>(target);
            conf.Defines.Add("DIRECTX_TOOLKIT_IMPORT");
        }
        if (target.SoundInterface == ESoundInterface.FMOD)
        {
            conf.Defines.Add("USE_FMOD");
        }

        conf.Defines.Add($"CFG_RAYTRACING={Convert.ToInt32(target.Raytracing == ERaytracing.On)}");

        conf.Defines.Add("CFG_CASCADE_SHADOW_COUNT=3");
        conf.Defines.Add("CFG_CASCADE_SHADOW_TEX_WIDTH=500");
        conf.Defines.Add("CFG_CASCADE_SHADOW_TEX_HEIGHT=500");

        conf.Defines.Add("CFG_WIDTH=1024");
        conf.Defines.Add("CFG_HEIGHT=768");
        conf.Defines.Add("CFG_VSYNC=1");
        conf.Defines.Add("CFG_FULLSCREEN=0");
        conf.Defines.Add("CFG_FRAME_BUFFER=2");
        conf.Defines.Add("CFG_SCREEN_NEAR=0.1f");
        conf.Defines.Add("CFG_SCREEN_FAR=1000.f");
        conf.Defines.Add("CFG_FOV=90.f");
        conf.Defines.Add("CFG_LAYER_COUNT=0");
        conf.Defines.Add("CFG_EPSILON=0.0001f");

        foreach (ERenderType renderType in Enum.GetValues(typeof(ERenderType)))
        {
            conf.Defines.Add($"CFG_RENDERTYPE_{renderType.ToString().ToUpper()}={Convert.ToInt32(target.RenderType == renderType)}");
        }

        conf.Defines.Add("CFG_MAX_DIRECTIONAL_LIGHT=8");
        conf.Defines.Add("CFG_PER_PARAM_BUFFER_SIZE=8");
        conf.Defines.Add("CFG_FRAME_LATENCY_TOLERANCE_SECOND=1");
        conf.Defines.Add("CFG_MAX_CONCURRENT_COMMAND_LIST=(1ULL << 8)");

        conf.Defines.Add("CFG_DEBUG_MAX_MESSAGE=200");
        conf.Defines.Add("CFG_DEBUG_MESSAGE_Y_MOVEMENT=10");
        conf.Defines.Add("CFG_DEBUG_MESSAGE_LIFETIME=1.f");

        if (target.LaunchType == ELaunchType.Client || target.LaunchType == ELaunchType.Server)
        {
            conf.Defines.Add("CFG_MONOLITH");
        }
    }

    public static EngineTarget GetDefinedTargetLaunchTypeFiltered(ELaunchType launchType) 
    {
        EngineTarget target = Utils.GetDefinedTarget();
        target.LaunchType = launchType;
        return target;
    }

    public static void MakeConfiturationNameDefine(Project.Configuration conf, EngineTarget target)
    {
        if (target.Optimization == Optimization.Release) { conf.Name = "Development"; }

        // Conf Name
        {
            if (target.LaunchType == ELaunchType.Editor)
            {
                conf.Name += "Editor";
            }
            else if (target.LaunchType == ELaunchType.Client)
            {
                conf.Name += "Client";
            }
            else if (target.LaunchType == ELaunchType.Server)
            {
                conf.Name += "Server";
            }
        }

        // Defines
        {
            conf.Defines.Add("_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING");
            conf.Defines.Add("_HAS_STD_BYTE=0");

            if (target.Name == "Release")
            {
                conf.Defines.Add("WITH_DEBUG=0");
            }
            else
            {
                conf.Defines.Add("WITH_DEBUG=1");
            }

            if (target.LaunchType == ELaunchType.Editor)
            {
                conf.Defines.Add("SHADER_DEBUG=1");
                conf.Defines.Add("WITH_EDITOR=1");
                conf.ResourceFileDefine += "WITH_EDITOR=1";
            }
            else
            {
                conf.Defines.Add("SHADER_DEBUG=0");
                conf.Defines.Add("WITH_EDITOR=0");
                conf.ResourceFileDefine += "WITH_EDITOR=0";
            }

            if (target.LaunchType == ELaunchType.Client)
            {
                conf.Defines.Add("CLIENT=1");
            }
            else
            {
                conf.Defines.Add("CLIENT=0");
            }

            if (target.LaunchType == ELaunchType.Server)
            {
                conf.Defines.Add("SERVER=1");
            }
            else
            {
                conf.Defines.Add("SERVER=0");
            }
        }
    }
}
