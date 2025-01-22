using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class DescriptorHeap : CommonProject
{
    public DescriptorHeap() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<ThrowIfFailed>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
    }
}