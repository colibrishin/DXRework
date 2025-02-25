using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Launch/Launch.build.cs")]
[module: Include("%EngineDir%/Engine/Monolith/MonolithClient.build.cs")]
[module: Include("%EngineDir%/Engine/Monolith/MonolithServer.build.cs")]

[Generate]
public class KolibriSolution : Solution
{
    public KolibriSolution() : base(typeof(EngineTarget))
    {
        IsFileNameToLower = false;
        Name = "Kolibri";
        FastBuildAllProjectType = typeof(FastBuildAllOverrideProject);

        AddTargets(Utils.GetDefinedTarget());
    }

    [Configure]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);

        if (target.LaunchType == ELaunchType.Editor)
        {
            ClientSolution clientSolution = new ClientSolution();
            EngineSolution engineSolution = new EngineSolution();

            clientSolution.ConfigureAll(conf, target);
            engineSolution.ConfigureAll(conf, target);
        }

        conf.SolutionPath = Utils.GetSolutionDir();
        string ProjectFilesDir = conf.SolutionPath + @"\Intermediate\ProjectFiles";
        Environment.SetEnvironmentVariable("ProjectFilesDir", ProjectFilesDir);

        if (target.LaunchType == ELaunchType.Client)
        {
            conf.AddProject<MonolithClient>(target);
        }

        if (target.LaunchType == ELaunchType.Server)
        {
            conf.AddProject<MonolithServer>(target);
        }
        
        conf.AddProject<Launch>(target);
        conf.SetStartupProject<Launch>();
    }
}