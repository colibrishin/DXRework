using System.IO;
using Sharpmake;

[Sharpmake.Export]
public class PhysX : ExportProject
{
    public PhysX()
    {
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\PhysX\physx\include");
        //conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\PhysX\physx\include");

        /*
        conf.LibraryFiles.Add
        (
            @"fmod_vc.lib"
        );
        */
    }
}