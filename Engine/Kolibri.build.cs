using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class KolibriProject : CommonProject
{
    public KolibriProject() : base(false)
    {
        Name = "Kolibri";

        SourceFilesExtensions.Add(".ini");
        SourceFilesExtensions.Add(".hlsl");
        StripFastBuildSourceFiles = false;

        AddTargets(new EngineTarget(
            ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server,
            Platform.win64,
            DevEnv.vs2022,
            Optimization.Debug | Optimization.Release,
            OutputType.Lib,
            Blob.NoBlob,
            BuildSystem.FastBuild
        ));
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
    
        //conf.TargetFileFullNameWithExtension = conf.ProjectName + ".exe";
        conf.TargetFileFullNameWithExtension = "Kolibri.exe";

        conf.AdditionalCompilerOptions.Add("/FS");
        conf.IsFastBuild = true;
        conf.SolutionFolder = @"Engine";

        string FastBuildPath = @"do-fastbuild.bat";
        FastBuildSettings.FastBuildMakeCommand = FastBuildPath;
    }
}

[Generate]
public class KolibriSolution : Solution
{
    public KolibriSolution() : base(typeof(EngineTarget))
    {
        IsFileNameToLower = false;
        Name = "Kolibri";
        FastBuildAllProjectType = typeof(FastBuildAllOverrideProject);

        AddTargets(new EngineTarget(
            ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server,
            Platform.win64,
            DevEnv.vs2022,
            Optimization.Debug | Optimization.Release,
            OutputType.Lib,
            Blob.NoBlob,
            BuildSystem.FastBuild
        ));
    }

    [Configure()]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);

        conf.SolutionPath = Utils.GetSolutionDir();
        string ProjectFilesDir = conf.SolutionPath + @"\Intermediate\ProjectFiles";
        Environment.SetEnvironmentVariable("ProjectFilesDir", ProjectFilesDir);

        conf.AddProject<KolibriProject>(target);
        conf.SetStartupProject<KolibriProject>();
    }
}