using System;
using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Client/ClientSolution.build.cs")]

[Generate]
public class KolibriSolution : ClientSolution
{
    public KolibriSolution()
    {
        IsFileNameToLower = false;
        Name = "Kolibri";
        FastBuildAllProjectType = typeof(FastBuildAllOverrideProject);
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        
        conf.SolutionPath = Utils.GetSolutionDir();
        string ProjectFilesDir = conf.SolutionPath + @"\Intermediate\ProjectFiles";
        Environment.SetEnvironmentVariable("ProjectFilesDir", ProjectFilesDir);
    }
}