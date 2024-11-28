using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

public abstract class ExportProject : Project
{
    protected ExportProject() : base(typeof(EngineTarget))
    {
        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;

        AddTargets(new EngineTarget(
                ELaunchType.Editor | ELaunchType.Client | ELaunchType.Server,
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release
        ));
    }

    [Configure()]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
    }

    [Configure(Optimization.Debug)] 
    public virtual void ConfigureDebug(Configuration conf, EngineTarget Target)
    {
    }

    [Configure(Optimization.Release)]
    public virtual void ConfigureRelease(Configuration conf, EngineTarget Target)
    {
    }
}

public class VCPKG : ExportProject 
{
    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);

        // Add root include path for vcpkg packages.
        conf.IncludePaths.Add(@"%EngineDir%\vcpkg_installed\x64-windows\include");

        // Add root lib path for vcpkg packages.
        conf.LibraryPaths.Add(@"%EngineDir%\vcpkg_installed\x64-windows\lib");
    }

    public override void ConfigureDebug(Configuration conf, EngineTarget target)
    {
        base.ConfigureDebug(conf, target);

        // Add root include path for vcpkg packages.
        conf.IncludePaths.Add(@"%EngineDir%\vcpkg_installed\x64-windows\include");

        // Add root lib path for vcpkg packages.
        conf.LibraryPaths.Add(@"%EngineDir%\vcpkg_installed\x64-windows\debug\lib");
    }
}