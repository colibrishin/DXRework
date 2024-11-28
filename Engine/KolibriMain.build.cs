using System;
using System.IO;
using Sharpmake;

[module: Include("Kolibri.build.cs")]

public static class Main
{
    [Sharpmake.Main]
    public static void SharpmakeMain(Sharpmake.Arguments arguments)
    {
        KitsRootPaths.SetUseKitsRootForDevEnv(DevEnv.vs2022, KitsRootEnum.KitsRoot10, Options.Vc.General.WindowsTargetPlatformVersion.Latest);
        arguments.Generate<KolibriSolution>();
    }
}
