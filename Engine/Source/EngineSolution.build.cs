using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/**/*.build.cs")]

[Generate]
public class EngineSolution : Solution
{
    public EngineSolution() : base(typeof(EngineTarget))
    {
        IsFileNameToLower = false;
        Name = "Engine";

        AddTargets(new EngineTarget(
            ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server,
            Platform.win64,
            DevEnv.vs2022,
            Optimization.Debug | Optimization.Release));
    }

    [Configure()]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);

        conf.SolutionPath = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        string ProjectFilesDir = Utils.GetSolutionDir() + @"\Intermediate\ProjectFiles";
        Environment.SetEnvironmentVariable("ProjectFilesDir", ProjectFilesDir);

		// Add Projects
        {
            // // ThirdParty
            // conf.AddProject<FBX>(target);

            // // dll
            // conf.AddProject<Core>(target);
            // conf.AddProject<Engine>(target);
            // conf.AddProject<Launch>(target);
            // conf.AddProject<Network>(target);
            // conf.AddProject<RenderCore>(target);
            // conf.AddProject<Renderer>(target);
            // conf.AddProject<RHI>(target);
            // conf.AddProject<D3D11RHI>(target);
            // conf.AddProject<Slate>(target);

            // // config
            // conf.AddProject<EngineConfig>(target);

            // exe
            // conf.AddProject<SE1>(target);
            // conf.SetStartupProject<SE1>();
        }
    }
}