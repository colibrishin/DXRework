using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/TBB/TBB.build.cs")]

[Generate]
public class Launch : CommonProject
{
    public Launch() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<TBB>(target);

        conf.AddPrivateDependency<WinAPIWrapper>(target);
        conf.AddPrivateDependency<EngineEntryPoint>(target);
        
        conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);
        conf.Output = Configuration.OutputType.Exe;
    }
}