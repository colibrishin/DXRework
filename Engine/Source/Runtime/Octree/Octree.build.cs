using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Octree : CommonProject
{
    public Octree() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<TypeLibrary>(target);
        conf.AddPublicDependency<DirectXTK>(target);
    }
}