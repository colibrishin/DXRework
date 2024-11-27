using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreComponent/CoreComponent.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXMath/DirectXMath.build.cs")]

[Generate]
public class Collider : CommonProject
{
    public Collider() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPrivateDependency<CoreComponent>(target);
        conf.AddPrivateDependency<Transform>(target);
        conf.AddPrivateDependency<DirectXMath>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
    }
}