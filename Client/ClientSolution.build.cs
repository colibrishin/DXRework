using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/EngineSolution.build.cs")]

[Generate]
public class ClientProject : CommonProject
{
    public ClientProject() : base(false)
    {
        Name = "Client";

        SourceFilesExtensions.Add(".ini");
        SourceFilesExtensions.Add(".hlsl");
        SourceFilesExtensions.Add(".jpg");
        SourceFilesExtensions.Add(".png");
        StripFastBuildSourceFiles = false;

        AddTargets(Utils.GetDefinedTarget());
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AdditionalCompilerOptions.Add("/FS");
        conf.IsFastBuild = true;
        conf.SolutionFolder = @"Client";

        string FastBuildPath = @"do-fastbuild.bat";
        FastBuildSettings.FastBuildMakeCommand = FastBuildPath;
        
        conf.TargetCopyFiles.Add
        (
            @"Sky.jpg",
            @"Texture.png",
            @"Texture-Normal.png",
        );

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
    }
}

[Generate]
public class ClientSolution : EngineSolution
{
    public ClientSolution()
    {
        IsFileNameToLower = false;
        Name = "Client";
        FastBuildAllProjectType = typeof(FastBuildAllOverrideProject);
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.SolutionPath = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        string ProjectFilesDir = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        Environment.SetEnvironmentVariable("ProjectFilesDir", ProjectFilesDir);

        conf.AddProject<ClientProject>(target);
    }
}