using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/ExportProject.build.cs")]

[Sharpmake.Export]
public class FMOD : ExportProject
{
    public FMOD()
    {
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\");
        conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\lib\x64");

        conf.LibraryFiles.Add
        (
            @"fmod_vc.lib"
        );
        conf.TargetCopyFiles.Add(@"lib\x64\fmod.dll");
    }
}