using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]

[Generate]
public class InputManager : EngineCommonProject
{
    public InputManager() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<DirectXTK>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPrivateDependency<WinAPIWrapper>(target);
    }
}