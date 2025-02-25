using System;
using System.IO;
using System.Collections;
using System.Diagnostics;
using System.Reflection;
using Microsoft.Win32;
using Sharpmake;

[module:Include("Utils.cs")]

public class FastBuildAllOverrideProject : FastBuildAllProject 
{
    public FastBuildAllOverrideProject() : base(typeof(EngineTarget))
    {
    }

    [Configure()]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target) 
    {
        Utils.MakeConfiturationNameDefine(conf, target);
    }
}


[Fragment, Flags]
public enum ELaunchType
{
    Editor = 1 << 0,
    Client = 1 << 1,
    Server = 1 << 2
}

public enum EGraphicAPI
{
    D3D12 = 1 << 0,
}

public enum ERenderType
{
    Deferred = 1 << 0,
    ForwardOnly = 1 << 1
}

public enum ERaytracing
{
    Off = 1 << 0,
    On = 1 << 1
}

public class EngineTarget : Target
{
    public ELaunchType LaunchType;
    public EGraphicAPI GraphicAPI;
    public ERenderType RenderType;
    public ERaytracing Raytracing;

    public EngineTarget() { }
    public EngineTarget(
        ELaunchType launchType,
        Platform platform,
        DevEnv devEnv,
        Optimization optimization,
        OutputType outputType = OutputType.Lib,
        EGraphicAPI graphicAPI = EGraphicAPI.D3D12,
        ERenderType renderType = ERenderType.Deferred,
        ERaytracing raytracing = ERaytracing.On,
        Blob blob = Blob.NoBlob,
        BuildSystem buildSystem = BuildSystem.FastBuild,
        DotNetFramework framework = DotNetFramework.v3_5) 
    : base(platform, devEnv, optimization, outputType, blob, buildSystem, framework)
    {
        LaunchType = launchType;
        GraphicAPI = graphicAPI;
        RenderType = renderType;
        Raytracing = raytracing;
    }
}

public abstract class EngineCommonProject : CommonProject 
{
    public EngineCommonProject() : base(true)
    {
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.SolutionFolder = @"Engine";
    }
}

public abstract class CommonProject : Project
{
    private static FileInfo GetSharpmakeFilePathImpl()
    {
        StackTrace stackTrace = new StackTrace(true);

        for (int i = 0; i < stackTrace.FrameCount - 1; ++i)
        {
            StackFrame stackFrame = stackTrace.GetFrame(i);
            MethodBase method = stackFrame.GetMethod();
            if (method.DeclaringType == typeof(CommonProject))
            {
                int iteration = 1;
                while (true)
                {
                    stackFrame = stackTrace.GetFrame(i + iteration);
                    method = stackFrame.GetMethod();

                    if (!method.DeclaringType.IsSubclassOf(typeof(Project)))
                    {
                        stackFrame = stackTrace.GetFrame(i + iteration - 1);
                        method = stackFrame.GetMethod();
                        break;
                    }
                    ++iteration;
                }
                return new FileInfo(stackFrame.GetFileName());
            }
        }
        throw new Error("Unable to find the file stacktrace"); 
    }

    private static string GetSharpmakeFilePath()
    {
        FileInfo fileInfo = GetSharpmakeFilePathImpl();
        return fileInfo.Directory.ToString();
    }

    protected CommonProject(bool bAddTarget = true) : base(typeof(EngineTarget))
    {
        Name = GetType().Name;

        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;
        StripFastBuildSourceFiles = false;
        SourceRootPath = GetSharpmakeFilePath();

        SourceFilesExtensions.Add(".cs");
        //SourceFilesCompileExtensions.Add(".ixx");

        if (bAddTarget == true)
        {
            AddTargets(Utils.GetDefinedTarget());
        }
    }

    [Configure]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);

        conf.DumpDependencyGraph = true;
        conf.ExecuteTargetCopy = true;
        conf.IncludeBlobbedSourceFiles = false;
        conf.BlobPath = $@"{Utils.GetSolutionDir()}/Intermediate/blob/";

        string emptyAPIString = $"ENGINE_{Name.ToUpper()}_API=";

        conf.ExportDefines.Add(emptyAPIString);
        conf.Defines.Add(emptyAPIString);

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
            conf.Options.Add(Options.Vc.Linker.LinkLibraryDependencies.Enable);
        }
        
        conf.Options.Add(Options.Vc.General.CharacterSet.Unicode);
        conf.Options.Add(Options.Vc.Compiler.JumboBuild.Enable);
        conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP20);
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

        conf.Options.Add(Options.Vc.CodeAnalysis.ClangTidyCodeAnalysis.Enable);

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

        if (target.Optimization == Optimization.Debug)
        {
            conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
            conf.AdditionalLinkerOptions.Add("/NODEFAULTLIB:libcmt.lib");
            conf.AdditionalLinkerOptions.Add("/NODEFAULTLIB:msvcrt.lib");
            conf.AdditionalLinkerOptions.Add("/NODEFAULTLIB:libcmtd.lib");
        }
        else
        {
            conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
            conf.AdditionalLinkerOptions.Add("/NODEFAULTLIB:libcmt.lib");
            conf.AdditionalLinkerOptions.Add("/NODEFAULTLIB:libcmtd.lib");
            conf.AdditionalLinkerOptions.Add("/NODEFAULTLIB:msvcrtd.lib");
        }
        
        string EngineDir = Utils.GetEngineDir();
        string GitDir = @"C:\Program Files\Git"; // todo: find git directory with where git

        Configuration.BuildStepExecutable Exec = new Configuration.BuildStepExecutable(
            $@"{EngineDir}\balius\target\release\balius.exe",
            $@"",
            $@"{EngineDir}\Intermediate\log\[project.Name]-headerparser.log",
            $@"""{EngineDir}"" [project.Name] ""[project.SourceRootPath]"" ""{GitDir}"" ""{conf.Name}""",
            EngineDir,
            true,
            true
        );
        Exec.FastBuildAlwaysShowOutput = false;
        Exec.FastBuildExecAlways = true;

        conf.EventCustomPrebuildExecute.Add(@"[project.Name]-headerparser", Exec);
        conf.CustomProperties.Add("CustomOptimizationProperty", $"Custom-{target.Optimization}");

        Utils.AddDefines(conf, target);

        if (target.GraphicAPI == EGraphicAPI.D3D12)
        {
            conf.AddPublicDependency<DirectXTK>(target);
        }
    }
}