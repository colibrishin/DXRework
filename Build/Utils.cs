using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
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

    /// <summary>Rust/cargo bin directory for FastBuild PATH.</summary>
    public static string GetRustPathForFastBuild()
    {
        string cargoHome = Environment.GetEnvironmentVariable("CARGO_HOME");
        if (!string.IsNullOrEmpty(cargoHome))
        {
            string bin = Path.Combine(cargoHome, "bin");
            if (Directory.Exists(bin))
                return Path.GetFullPath(bin);
        }
        string userProfile = Environment.GetEnvironmentVariable("USERPROFILE");
        if (!string.IsNullOrEmpty(userProfile))
        {
            string bin = Path.Combine(userProfile, ".cargo", "bin");
            if (Directory.Exists(bin))
                return Path.GetFullPath(bin);
        }
        return string.Empty;
    }

    /// <summary>VC tools bin directory (link.exe) for FastBuild PATH. Uses vswhere then VC\Tools\MSVC\*\bin\Hostx64\x64.</summary>
    public static string GetVCToolsPathForFastBuild()
    {
        string vsWhere = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
            "Microsoft Visual Studio", "Installer", "vswhere.exe");
        if (!File.Exists(vsWhere))
            return string.Empty;
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = vsWhere,
                Arguments = "-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };
            using (var p = Process.Start(psi))
            {
                if (p == null)
                    return string.Empty;
                string vsPath = p.StandardOutput?.ReadToEnd()?.Trim();
                if (string.IsNullOrEmpty(vsPath) || !Directory.Exists(vsPath))
                    return string.Empty;
                string msvcRoot = Path.Combine(vsPath, "VC", "Tools", "MSVC");
                if (!Directory.Exists(msvcRoot))
                    return string.Empty;
                string latestVer = Directory.GetDirectories(msvcRoot)
                    .OrderByDescending(d => d, StringComparer.Ordinal)
                    .FirstOrDefault();
                if (string.IsNullOrEmpty(latestVer))
                    return string.Empty;
                string binPath = Path.Combine(latestVer, "bin", "Hostx64", "x64");
                return Directory.Exists(binPath) ? Path.GetFullPath(binPath) : string.Empty;
            }
        }
        catch
        {
            return string.Empty;
        }
    }

    /// <summary>Rust + VC path segment for FastBuild. SDK is appended by Sharpmake (no override).</summary>
    public static string GetFastBuildPathRustAndVCTools()
    {
        string rust = GetRustPathForFastBuild();
        string vc = GetVCToolsPathForFastBuild();
        var parts = new[] { rust, vc }.Where(s => !string.IsNullOrEmpty(s));
        return string.Join(";", parts);
    }

    /// <summary>Windows SDK Lib path (um\x64 and ucrt\x64) so link.exe can find ntdll.lib etc. Note: libs are under Lib\, not bin\.</summary>
    public static string GetWindowsSdkLibPathForFastBuild()
    {
        string programFilesX86 = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
        string kitsRoot = Path.Combine(programFilesX86, "Windows Kits", "10", "Lib");
        string kitsBin = Path.Combine(programFilesX86, "Windows Kits", "10", "bin");

        // Prefer: use version from Lib folder (latest).
        if (Directory.Exists(kitsRoot))
        {
            string latestVer = Directory.GetDirectories(kitsRoot)
                .OrderByDescending(d => d, StringComparer.Ordinal)
                .FirstOrDefault();
            if (!string.IsNullOrEmpty(latestVer))
            {
                string um = Path.Combine(latestVer, "um", "x64");
                string ucrt = Path.Combine(latestVer, "ucrt", "x64");
                var parts = new List<string>();
                if (Directory.Exists(um))
                    parts.Add(Path.GetFullPath(um));
                if (Directory.Exists(ucrt))
                    parts.Add(Path.GetFullPath(ucrt));
                if (parts.Count > 0)
                    return string.Join(";", parts);
            }
        }

        // Fallback: use same SDK version as bin (PATH). Lib path = Lib\<ver>\um\x64 and ucrt\x64.
        if (Directory.Exists(kitsBin))
        {
            string binVer = Directory.GetDirectories(kitsBin)
                .OrderByDescending(d => d, StringComparer.Ordinal)
                .FirstOrDefault();
            if (!string.IsNullOrEmpty(binVer))
            {
                string verName = Path.GetFileName(binVer);
                string libVer = Path.Combine(programFilesX86, "Windows Kits", "10", "Lib", verName);
                string um = Path.Combine(libVer, "um", "x64");
                string ucrt = Path.Combine(libVer, "ucrt", "x64");
                var parts = new List<string>();
                if (Directory.Exists(um))
                    parts.Add(Path.GetFullPath(um));
                if (Directory.Exists(ucrt))
                    parts.Add(Path.GetFullPath(ucrt));
                if (parts.Count > 0)
                    return string.Join(";", parts);
            }
        }

        return string.Empty;
    }

    /// <summary>VC lib path (MSVC\*\lib\x64) for link.exe.</summary>
    public static string GetVCLibPathForFastBuild()
    {
        string vsWhere = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
            "Microsoft Visual Studio", "Installer", "vswhere.exe");
        if (!File.Exists(vsWhere))
            return string.Empty;
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = vsWhere,
                Arguments = "-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath",
                RedirectStandardOutput = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };
            using (var p = Process.Start(psi))
            {
                if (p == null)
                    return string.Empty;
                string vsPath = p.StandardOutput?.ReadToEnd()?.Trim();
                if (string.IsNullOrEmpty(vsPath) || !Directory.Exists(vsPath))
                    return string.Empty;
                string msvcRoot = Path.Combine(vsPath, "VC", "Tools", "MSVC");
                if (!Directory.Exists(msvcRoot))
                    return string.Empty;
                string latestVer = Directory.GetDirectories(msvcRoot)
                    .OrderByDescending(d => d, StringComparer.Ordinal)
                    .FirstOrDefault();
                if (string.IsNullOrEmpty(latestVer))
                    return string.Empty;
                string libPath = Path.Combine(latestVer, "lib", "x64");
                return Directory.Exists(libPath) ? Path.GetFullPath(libPath) : string.Empty;
            }
        }
        catch
        {
            return string.Empty;
        }
    }

    /// <summary>LIB for FastBuild so link.exe finds ntdll.lib and other SDK/VC libs. Append only (do not override existing LIB).</summary>
    public static string GetFastBuildLib()
    {
        string sdk = GetWindowsSdkLibPathForFastBuild();
        string vc = GetVCLibPathForFastBuild();
        var parts = new[] { sdk, vc }.Where(s => !string.IsNullOrEmpty(s));
        return string.Join(";", parts);
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
