using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreRenderable/CoreRenderable.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreComponent/CoreComponent.build.cs")]

[Generate]
public class Scene : CommonProject
{
    public Scene() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreRenderable>(target);
        conf.AddPublicDependency<ConcurrentTypeLibrary>(target);
        conf.AddPublicDependency<CoreComponent>(target);
        conf.AddPublicDependency<Script>(target);
        conf.AddPublicDependency<Octree>(target);
        conf.AddPublicDependency<Layer>(target);
        conf.AddPrivateDependency<BoundingGetter>(target);
    }
}