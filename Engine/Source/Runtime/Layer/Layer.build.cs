using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/Runtime/Abstracts/CoreRenderable/CoreRenderable.build.cs")]

[Generate]
public class Layer : CommonProject
{
    public Layer() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreRenderable>(target);
        conf.AddPublicDependency<ConcurrentTypeLibrary>(target);
    }
}