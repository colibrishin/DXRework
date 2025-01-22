using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Core/Core.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Managers/ResourceManager/ResourceManager.build.cs")]

[Generate]
public class Collider : CommonProject
{
    public Collider() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPrivateDependency<Core>(target);
        conf.AddPrivateDependency<ResourceManager>(target);
    }
}