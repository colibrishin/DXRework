using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ReflectionEvaluator : CommonProject
{
    public ReflectionEvaluator() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Texture2D>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<CommandPair>(target);
        conf.AddPrivateDependency<DescriptorHeap>(target);
    }
}