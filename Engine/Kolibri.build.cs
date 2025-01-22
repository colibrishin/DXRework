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

        conf.Defines.Add("CFG_MAX_DIRECTIONAL_LIGHT=8");
        conf.Defines.Add("CFG_PER_PARAM_BUFFER_SIZE=8");
        conf.Defines.Add("CFG_FRAME_LATENCY_TOLERANCE_SECOND=1");
        conf.Defines.Add("CFG_MAX_CONCURRENT_COMMAND_LIST=(1ULL << 8)");

        conf.Defines.Add("CFG_DEBUG_MAX_MESSAGE=200");
        conf.Defines.Add("CFG_DEBUG_MESSAGE_Y_MOVEMENT=10");
        conf.Defines.Add("CFG_DEBUG_MESSAGE_LIFETIME=1.f");
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