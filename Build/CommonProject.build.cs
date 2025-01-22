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

public class EngineTarget : Target
{
    public ELaunchType LaunchType;

    public EngineTarget() { }
    public EngineTarget(
        ELaunchType launchType,
        Platform platform,
        DevEnv devEnv,
        Optimization optimization,
        OutputType outputType = OutputType.Lib,
        Blob blob = Blob.NoBlob,
        BuildSystem buildSystem = BuildSystem.MSBuild,
        DotNetFramework framework = DotNetFramework.v3_5) 
    : base(platform, devEnv, optimization, outputType, blob, buildSystem, framework)
    {
        LaunchType = launchType; //ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server;
    }
}

public abstract class CommonProject : Project
{
    protected CommonProject(bool bAddTarget = true) : base(typeof(EngineTarget))
    {
        Name = GetType().Name;

        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;

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

        // conf.Output = Configuration.OutputType.Exe;
        if (target.LaunchType == ELaunchType.Editor)
        {
            conf.Output = Configuration.OutputType.Dll;
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

            // Include
            {
                //conf.IncludePrivatePaths.Add("[project.SourceRootPath]");
                conf.IncludePrivatePaths.Add(conf.ProjectPath);

                string HeaderParserTargetDir = SolutionDir + @"/Intermediate/HeaderParser/HeaderParserGenerated/[project.Name]";
                conf.IncludePaths.Add(HeaderParserTargetDir);
                conf.IncludePaths.Add(@"[project.SourceRootPath]");
                conf.IncludePaths.Add(@"[project.SourceRootPath]\Public");
            }
        }
        
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
        ///string EngineDir = Utils.GetEngineDir();
        ///conf.EventPreBuild.Add(@"cmd /c """"" + EngineDir + @"\Engine\Source\Programs\HeaderParser\HeaderParser.bat"" ""$(SolutionDir)"" [project.Name] ""[project.SourceRootPath]"" " + @""""+ EngineDir + @"""""");

        conf.CustomProperties.Add("CustomOptimizationProperty", $"Custom-{target.Optimization}");
    }
}