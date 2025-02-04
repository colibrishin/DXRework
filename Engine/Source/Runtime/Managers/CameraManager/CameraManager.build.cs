using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class CameraManager : CommonProject
{
    public CameraManager() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<CoreRender>(target);
        conf.AddPublicDependency<DirectXTK>(target);
        conf.AddPublicDependency<Boost>(target);
    }
}