using System;
using System.Collections;
using Microsoft.Win32;
using Sharpmake;

[Sharpmake.Export]
public abstract class ExportProject : Project
{
    protected ExportProject() : base(typeof(EngineTarget))
    {
        Name = GetType().Name;

        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;
        IsExportProject = true;
        StripFastBuildSourceFiles = false;

        SourceRootPath = @"[project.RootPath]";
        SourceFilesExtensions.Add(".cs");

        AddTargets(Utils.GetDefinedTarget());
    }

    [Configure()]
    public virtual void ConfigureAll(Configuration conf, EngineTarget target)
    {
        Utils.MakeConfiturationNameDefine(conf, target);
        conf.SolutionFolder = @"ThirdParty";
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

public class VCPKG : ExportProject
{
    protected VCPKG()
    {
    }

    protected string GetVCPKGBinPath(EngineTarget target) 
    {
        string SolutionDir = Utils.GetSolutionDir();
        
        if (target.Optimization == Optimization.Debug) 
        {
            return SolutionDir + @"/vcpkg_installed/x64-windows/debug/bin";
        }
        else if (target.Optimization == Optimization.Release) 
        {
            return SolutionDir + @"/vcpkg_installed/x64-windows/bin";
        }
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target) 
    {
        base.ConfigureAll(conf, target);
    }

    public override void ConfigureRelease(Configuration conf, EngineTarget target)
    {
        base.ConfigureRelease(conf, target);
        string SolutionDir = Utils.GetSolutionDir();

        // Add root include path for vcpkg packages.
        conf.IncludePaths.Add(SolutionDir + @"\vcpkg_installed\x64-windows\include");

        // Add root lib path for vcpkg packages.
        conf.LibraryPaths.Add(SolutionDir + @"\vcpkg_installed\x64-windows\lib");
    }

    public override void ConfigureDebug(Configuration conf, EngineTarget target)
    {
        base.ConfigureDebug(conf, target);
        string SolutionDir = Utils.GetSolutionDir();

        // Add root include path for vcpkg packages.
        conf.IncludePaths.Add(SolutionDir + @"\vcpkg_installed\x64-windows\include");

        // Add root lib path for vcpkg packages.
        conf.LibraryPaths.Add(SolutionDir + @"\vcpkg_installed\x64-windows\debug\lib");
    }
}