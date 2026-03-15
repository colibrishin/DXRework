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

public enum ESoundInterface 
{
    FMOD = 1 << 0,
}

/// <summary>Interface for engine project configuration; use Project.Configuration so implementations match.</summary>
public interface IEngineProjectConfigure
{
    void ConfigureAll(Project.Configuration conf, EngineTarget target);
}

public class EngineTarget : Target
{
    public ELaunchType LaunchType;
    public EGraphicAPI GraphicAPI;
    public ERenderType RenderType;
    public ERaytracing Raytracing;
    public ESoundInterface SoundInterface;

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
        ESoundInterface soundInterface = ESoundInterface.FMOD,
        Blob blob = Blob.NoBlob,
        BuildSystem buildSystem = BuildSystem.FastBuild,
        DotNetFramework framework = DotNetFramework.v4_7) 
    : base(platform, devEnv, optimization, outputType, blob, buildSystem, framework)
    {
        LaunchType = launchType;
        GraphicAPI = graphicAPI;
        RenderType = renderType;
        Raytracing = raytracing;
        SoundInterface = soundInterface;
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

public abstract class CommonProject : Project, IEngineProjectConfigure
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

    /// <param name="bAddTarget">Add default targets.</param>
    /// <param name="name">Project name; also used for HeaderParser path (must match [project.Name] for balius). If null, use GetType().Name.</param>
    protected CommonProject(bool bAddTarget = true, string name = null) : base(typeof(EngineTarget))
    {
        Name = name ?? GetType().Name;

        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;
        StripFastBuildSourceFiles = false;
        SourceRootPath = GetSharpmakeFilePath();

        SourceFilesExtensions.Add(".cs");

        if (bAddTarget == true)
        {
            AddTargets(Utils.GetDefinedTarget());
        }

        // Use EngineDir so generated-header path matches where balius (prebuild) writes; fallback to SolutionDir.
        string engineDir = Utils.GetEngineDir();
        string solutionDir = Utils.GetSolutionDir();
        string headerGeneratedRoot = !string.IsNullOrEmpty(engineDir) ? engineDir : solutionDir;

        // Per-project: Intermediate/HeaderParser/<Name>/HeaderGenerated (Name must match [project.Name] used by balius).
        string headerGeneratedPath = Path.Combine(headerGeneratedRoot, "Intermediate", "HeaderParser", Name, "HeaderGenerated");
        AdditionalSourceRootPaths.Add(headerGeneratedPath);
    }

    [Configure(Optimization.Debug)]
    public virtual void ConfigureDebug(Configuration conf, EngineTarget target)
    {
    }

    [Configure(Optimization.Release)]
    public virtual void ConfigureRelease(Configuration conf, EngineTarget target)
    {
    }

    [Configure]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);

        conf.DumpDependencyGraph = true;
        conf.ExecuteTargetCopy = true;
        conf.IncludeBlobbedSourceFiles = false;
        conf.BlobPath = $@"{Utils.GetSolutionDir()}/Intermediate/blob/";
        conf.ExportAdditionalLibrariesEvenForStaticLib = true;

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
            conf.AdditionalCompilerOptions.Add("/Zm1000");
            conf.IsFastBuild = true;
            string FastBuildPath = SolutionDir + @"/Programs\Sharpmake\tools\FastBuild\Windows-x64\FBuild.exe";
            FastBuildSettings.FastBuildMakeCommand = FastBuildPath;
            FastBuildSettings.FastBuildAllowDBMigration = true;

            // Include
            {
                conf.IncludePrivatePaths.Add(conf.ProjectPath + @"/Private");

                // Use EngineDir so include path matches where balius (prebuild) writes generated headers.
                // header-parser writes to HeaderGenerated/Public/ and HeaderGenerated/Private/; code uses #include "X.generated.h"
                // and "Public/..." / "Private/..." for tracking headers (same path rule as normal generated headers).
                string engineDir = Utils.GetEngineDir();
                string headerParserRoot = !string.IsNullOrEmpty(engineDir) ? engineDir : SolutionDir;
                string HeaderParserTargetDir = headerParserRoot + @"/Intermediate/HeaderParser/[project.Name]/HeaderGenerated";
                conf.IncludePaths.Add(HeaderParserTargetDir);
                conf.IncludePaths.Add(HeaderParserTargetDir + @"/Public");
                conf.IncludePaths.Add(HeaderParserTargetDir + @"/Private");
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

        string baliusExe = Path.Combine(EngineDir, "balius", "target", "release", "balius.exe");
        string baliusArgs = $@"""{EngineDir}"" ""[project.Name]"" ""[project.SourceRootPath]"" ""{GitDir}"" ""[conf.Name]""";

        Configuration.BuildStepExecutable Exec = new Configuration.BuildStepExecutable(
            baliusExe,
            "",
            $@"{EngineDir}\Intermediate\log\[project.Name]-headerparser.log",
            baliusArgs,
            EngineDir,
            true,
            true
        );
        Exec.FastBuildAlwaysShowOutput = true;
        Exec.FastBuildExecAlways = true;

        conf.EventCustomPrebuildExecute.Add(@"[project.Name]-headerparser", Exec);

        // Build header-parser before this project so the prebuild can use an up-to-date header-parser.exe.
        string headerParserVcxproj = Path.Combine(Utils.GetSolutionDir(), "Programs", "header-parser", "header-parser.vcxproj");
        conf.ProjectReferencesByPath.Add(headerParserVcxproj);

        // Generated headers: Sharpmake emits PropertyGroup + ClCompile discovery in vcxproj when HeaderGeneratedRoot is set; Clean via AdditionalNMakeCleanCommands.
        string headerParserRootForTargets = !string.IsNullOrEmpty(Utils.GetEngineDir()) ? Utils.GetEngineDir() : Utils.GetSolutionDir();
        conf.CustomProperties.Add("HeaderGeneratedRoot", headerParserRootForTargets);
        string headerGeneratedPath = Path.Combine(headerParserRootForTargets, "Intermediate", "HeaderParser", Name, "HeaderGenerated");
        conf.AdditionalNMakeCleanCommands = "if exist \"" + headerGeneratedPath + "\" rmdir /s /q \"" + headerGeneratedPath + "\"";

        conf.CustomProperties.Add("CustomOptimizationProperty", $"Custom-{target.Optimization}");

        Utils.AddDefines(conf, target);
    }
}