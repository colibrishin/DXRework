using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class CoreActor : CommonProject
{
    public CoreActor() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<CoreRenderable>(target);
        conf.AddPublicDependency<Delegation>(target);
    }
}