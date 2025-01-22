using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Mesh : CommonProject
{
    public Mesh() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<DX12Agility>(target);
        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPublicDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<CommandPair>(target);
        conf.AddPrivateDependency<DescriptorHeap>(target);
    }
}