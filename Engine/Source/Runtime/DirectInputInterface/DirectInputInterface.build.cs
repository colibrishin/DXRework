using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class DirectInputInterface : EngineCommonProject
{
    public DirectInputInterface() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<DX12Agility>(target);
        
        if (target.Platform == Platform.win64) 
        {
            conf.AddPrivateDependency<WinAPIWrapper>(target);
        }
    }
}