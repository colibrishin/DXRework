using System;
using System.Collections;
using System.IO;
using Microsoft.Win32;
using Sharpmake;

[module: Include("%EngineDir%/Build/ThirdPartyPrograms/ThirdPartyPrograms.build.cs")]
[module: Include("%EngineDir%/Client/ClientSolution.build.cs")]
[module: Include("%EngineDir%/Engine/Source/EngineSolution.build.cs")]

[Generate]
public class Monolith : Project
{
    public Monolith(EngineTarget target) : base(typeof(EngineTarget))
    {
        Name = GetType().Name;

        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;
        StripFastBuildSourceFiles = false;

        SourceRootPath = @"";
        SourceFilesExtensions.Add(".cs");

        AddTargets(target);

        Solution.Configuration solutionConfiguration = Utils.GetInstantiatedSolutionConfiguration(target, new ClientSolution(), new EngineSolution());

        foreach (Solution.Configuration.IncludedProjectInfo info in solutionConfiguration.IncludedProjectInfos)
        {
            Project instantiated = (Project)Activator.CreateInstance(info.Type);
            // Exclude ImGui sources from the monolith blob so STB_*_IMPLEMENTATION is not compiled multiple times in one TU.
            if (instantiated.Name != "ImGui")
                AdditionalSourceRootPaths.Add(instantiated.SourceRootPath);
        }
    }

    [Configure]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);
        string ProjectFilesDir = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        Environment.SetEnvironmentVariable("ProjectFilesDir", ProjectFilesDir);

        Solution.Configuration solutionConfiguration = Utils.GetInstantiatedSolutionConfiguration(target, new ClientSolution(), new EngineSolution());

        foreach (Solution.Configuration.IncludedProjectInfo info in solutionConfiguration.IncludedProjectInfos)
        {
            conf.AddPrivateDependency(target, info.Type);
        }

        Utils.AddSolutionPrebuildSteps(conf);

        // Build header-parser before Monolith when the vcxproj exists (solution build order).
        string headerParserVcxproj = Path.Combine(Utils.GetSolutionDir(), "Programs", "header-parser", "header-parser.vcxproj");
        if (File.Exists(headerParserVcxproj))
            conf.AddPrivateDependency<HeaderParserProject>(target);

        // Monolith (static lib) compiles sources from all dependency projects; it must see their
        // generated headers (from balius/header-parser) which live under Intermediate/HeaderParser/<ProjectName>/HeaderGenerated.
        string solutionDirForPaths = Utils.GetSolutionDir();
        string engineDir = Utils.GetEngineDir();
        string headerGeneratedRoot = !string.IsNullOrEmpty(engineDir) ? engineDir : solutionDirForPaths;
        if (!string.IsNullOrEmpty(headerGeneratedRoot))
        {
            foreach (Solution.Configuration.IncludedProjectInfo info in solutionConfiguration.IncludedProjectInfos)
            {
                Project dep = (Project)Activator.CreateInstance(info.Type);
                string headerGeneratedPath = Path.Combine(headerGeneratedRoot, "Intermediate", "HeaderParser", dep.Name, "HeaderGenerated");
                conf.IncludePaths.Add(Path.Combine(headerGeneratedPath, "Public"));
                conf.IncludePaths.Add(Path.Combine(headerGeneratedPath, "Private"));
            }
        }

        conf.DumpDependencyGraph = true;
        conf.ExecuteTargetCopy = true;
        conf.IncludeBlobbedSourceFiles = false;

        conf.Output = Configuration.OutputType.Lib;
        conf.Options.Add(Options.Vc.Linker.LinkLibraryDependencies.Enable);
        
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

        string solutionDir = Utils.GetSolutionDir();
        conf.AdditionalCompilerOptions.Add("/FS");
        conf.AdditionalCompilerOptions.Add("/bigobj");
        conf.ProjectPath = Utils.GetProjectDir();
        conf.BlobPath = $@"{solutionDir}/Intermediate/blob/";
        conf.TargetPath = solutionDir + @"/Binaries/" + conf.Name;
        conf.IntermediatePath = solutionDir + @"/Intermediate/Build/" + conf.Name + "/[project.Name]/";
        conf.IsFastBuild = true;
        string FastBuildPath = solutionDir + @"/Programs\Sharpmake\tools\FastBuild\Windows-x64\FBuild.exe";
        FastBuildSettings.FastBuildMakeCommand = FastBuildPath;

        Utils.AddDefines(conf, target);
    }
}