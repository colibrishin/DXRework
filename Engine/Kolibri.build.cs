using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]

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

        FastBuildSettings.FastBuildMakeCommand = "Programs/Sharpmake/tools/FastBuild/Windows-x64/FBuild.exe";
        conf.AdditionalCompilerOptions.Add("/FS");
        conf.IsFastBuild = true;
        conf.SolutionFolder = @"Engine";

        conf.AddPrivateDependency<Boost>(target);

        conf.Defines.Add("NOMINMAX=1");
        conf.Defines.Add("IMGUI_DEFINE_MATH_OPERATORS=1");
        conf.Defines.Add("USE_DX12");
        //conf.Defines.Add("SNIFF_DEVICE_REMOVAL");
        conf.Defines.Add("PER_PARAM_BUFFER_SIZE=8");
        conf.Defines.Add("MAX_CONCURRENT_COMMAND_LIST=256");
        conf.Defines.Add("CASCADE_SHADOW_COUNT=3");
        conf.Defines.Add("CASCADE_SHADOW_TEX_WIDTH=500");
        conf.Defines.Add("CASCADE_SHADOW_TEX_HEIGHT=500");
        conf.Defines.Add("MAX_DIRECTIONAL_LIGHT=8");
    }
}

[Generate]
public class KolibriSolution : Solution
{
    public KolibriSolution() : base(typeof(EngineTarget))
    {
        IsFileNameToLower = true;
        Name = "Kolibri";

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