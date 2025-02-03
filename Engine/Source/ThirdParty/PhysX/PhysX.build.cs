using System.IO;
using Sharpmake;

[Sharpmake.Export]
public class PhysX : Project
{
    public PhysX() : base(typeof(EngineTarget))
    {
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
        conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\PhysX\physx\include");
        //conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\PhysX\physx\include");

        /*
        conf.LibraryFiles.Add
        (
            @"fmod_vc.lib"
        );
        */
    }

    [Configure(Optimization.Debug)] 
    public virtual void ConfigureDebug(Configuration conf, EngineTarget target)
    {
    }

    [Configure(Optimization.Release)]
    public virtual void ConfigureRelease(Configuration conf, EngineTarget target)
    {
    }
}