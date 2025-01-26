using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[module:Include("Utils.cs")]

[Fragment, Flags]
public enum ELaunchType
{
    Editor = 1 << 0,
    Client = 1 << 1,
    Server = 1 << 2
}

[Fragment, Flags]
public enum EGraphicAPI
{
    D3D12 = 1 << 0,
}

public class EngineTarget : Target
{
    public ELaunchType LaunchType;
    public EGraphicAPI GraphicAPI;

    public EngineTarget() { }
    public EngineTarget(
        ELaunchType launchType,
        Platform platform,
        DevEnv devEnv,
        Optimization optimization,
        OutputType outputType = OutputType.Lib,
        Blob blob = Blob.NoBlob,
        BuildSystem buildSystem = BuildSystem.FastBuild,
        DotNetFramework framework = DotNetFramework.v3_5,
        EGraphicAPI graphicAPI = EGraphicAPI.D3D12) 
    : base(platform, devEnv, optimization, outputType, blob, buildSystem, framework)
    {
        LaunchType = launchType;
        GraphicAPI = graphicAPI;
    }
}

public abstract class CommonProject : Project
{
    protected CommonProject(bool bAddTarget = true) : base(typeof(EngineTarget))
    {
        Name = GetType().Name;

        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;
        StripFastBuildSourceFiles = false;

        SourceRootPath = @"[project.RootPath]";
        SourceFilesExtensions.Add(".cs");
        //SourceFilesCompileExtensions.Add(".ixx");

        if (bAddTarget == true)
        {
            AddTargets(new EngineTarget(
                    ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server,
                    Platform.win64,
                    DevEnv.vs2022,
                    Optimization.Debug | Optimization.Release
            ));
        }
    }

    [Configure]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);

        conf.DumpDependencyGraph = true;
        conf.ExecuteTargetCopy = true;

        string emptyAPIString = "ENGINE_" + Name.ToUpper() + "_API=EMPTY";

        conf.ExportDefines.Add(emptyAPIString);
        conf.Defines.Add(emptyAPIString);

        // conf.Output = Configuration.OutputType.Exe;
        if (target.LaunchType == ELaunchType.Editor)
        {
            conf.Output = Configuration.OutputType.Dll;

            conf.ExportDefines.Remove(emptyAPIString);
            conf.Defines.Remove(emptyAPIString);

            conf.ExportDefines.Add("ENGINE_" + Name.ToUpper() + "_API=DLLIMPORT");
            conf.Defines.Add("ENGINE_" + Name.ToUpper() + "_API=DLLEXPORT");
        }
        else
        {
            conf.Output = Configuration.OutputType.Lib;
        }
        
        conf.Options.Add(Options.Vc.General.CharacterSet.Unicode);
        conf.Options.Add(Options.Vc.Compiler.JumboBuild.Enable);
        conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP20);
        //conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.Latest);
        if (target.Optimization == Optimization.Debug)
        {
            conf.Options.Add(Options.Vc.Compiler.Inline.Default);

            // Visual Studio 핫리로드 대응
            conf.Options.Add(Options.Vc.Linker.Incremental.Enable);
            conf.Options.Add(Options.Vc.General.DebugInformation.ProgramDatabaseEnC);
            conf.Options.Add(Options.Vc.Compiler.FunctionLevelLinking.Enable);
        }

        // RTTI
        conf.Options.Add(Options.Vc.Compiler.RTTI.Enable);

        conf.ProjectFileName = "[project.Name]";

        // Exceptions
        conf.Options.Add(Options.Vc.Compiler.Exceptions.Enable);

        // Debug
        {
            conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings();
            conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = "$(OutputPath)";
        }

        // Path
        {
            conf.ProjectPath = Utils.GetProjectDir();

            string SolutionDir = Utils.GetSolutionDir();
            conf.TargetPath = SolutionDir + @"/Binaries/" + conf.Name;
            conf.IntermediatePath = SolutionDir + @"/Intermediate/Build/" + conf.Name + "/[project.Name]/";

            conf.AdditionalCompilerOptions.Add("/FS");
            conf.IsFastBuild = true;
            string FastBuildPath = SolutionDir + @"/Programs\Sharpmake\tools\FastBuild\Windows-x64\FBuild.exe";
            FastBuildSettings.FastBuildMakeCommand = FastBuildPath;

            // Include
            {
                conf.IncludePrivatePaths.Add(conf.ProjectPath + @"/Private");

                string HeaderParserTargetDir = SolutionDir + @"/Intermediate/HeaderParser/HeaderGenerated/[project.Name]";
                conf.IncludePaths.Add(HeaderParserTargetDir);
                conf.IncludePaths.Add(@"[project.SourceRootPath]");
                conf.IncludePaths.Add(@"[project.SourceRootPath]/Public");
                conf.IncludePaths.Add(SolutionDir + @"/Engine");
            }
        }

        conf.AdditionalCompilerOptions.Add("/bigobj");
        
        //if (target.LaunchType == ELaunchType.Editor)
        //{
        //    //conf.ForceSymbolReferences.Add("IMPLEMENT_MODULE_" + conf.Project.Name);
        //}

        // Runtime Library
        {
            //if (target.LaunchType == ELaunchType.Editor)
            {
                if (target.Optimization == Optimization.Debug)
                    conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
                else
                    conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
            }
            //else
            //{
            //    if (target.Optimization == Optimization.Debug)
            //        conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebug);
            //    else
            //        conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreaded);
            //}
        }
        
        string EngineDir = Utils.GetEngineDir();
        string GitDir = @"C:\Program Files\Git"; // todo: find git directory with where git

        Configuration.BuildStepExecutable Exec = new Configuration.BuildStepExecutable(
            $@"{EngineDir}\balius\target\release\balius.exe",
            $@"",
            @"[project.Name]-headerparser.log",
            $@"""{EngineDir}"" [project.Name] ""[project.SourceRootPath]"" ""{GitDir}""",
            EngineDir,
            true,
            true
        );
        Exec.FastBuildAlwaysShowOutput = false;
        Exec.FastBuildExecAlways = true;

        conf.EventCustomPrebuildExecute.Add(@"[project.Name]-headerparser", Exec);

        conf.CustomProperties.Add("CustomOptimizationProperty", $"Custom-{target.Optimization}");

        conf.AdditionalCompilerOptions.Add("/FS");
        conf.IsFastBuild = true;

        {
            conf.Defines.Add("NOMINMAX=1");
            if (target.GraphicAPI == EGraphicAPI.D3D12) 
            {
                conf.Defines.Add("USE_DX12");
            }

            //conf.Defines.Add("SNIFF_DEVICE_REMOVAL");

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
            conf.Defines.Add("CFG_RAYTRACING=0");
            conf.Defines.Add("CFG_LAYER_COUNT=0");
            conf.Defines.Add("CFG_EPSILON=0.0001f");

            conf.Defines.Add("CFG_MAX_DIRECTIONAL_LIGHT=8");
            conf.Defines.Add("CFG_PER_PARAM_BUFFER_SIZE=8");
            conf.Defines.Add("CFG_FRAME_LATENCY_TOLERANCE_SECOND=1");
            conf.Defines.Add("CFG_MAX_CONCURRENT_COMMAND_LIST=(1ULL << 8)");

            conf.Defines.Add("CFG_DEBUG_MAX_MESSAGE=200");
            conf.Defines.Add("CFG_DEBUG_MESSAGE_Y_MOVEMENT=10");
            conf.Defines.Add("CFG_DEBUG_MESSAGE_LIFETIME=1.f");
        }
    }
}