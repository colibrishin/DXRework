using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Texture : CommonProject
{
    public Texture() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<DX12Agility>(target);
        conf.AddPublicDependency<CommandPair>(target);
        conf.AddPublicDependency<DescriptorHeap>(target);

        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<DirectXTex>(target);
        conf.AddPrivateDependency<DirectXTK>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<ThrowIfFailed>(target);
    }
}