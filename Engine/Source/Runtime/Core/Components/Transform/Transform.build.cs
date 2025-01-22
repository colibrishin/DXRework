using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Core/Core.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXMath/DirectXMath.build.cs")]

[Generate]
public class Transform : CommonProject
{
    public Transform() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPrivateDependency<DirectXMath>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
    }
}