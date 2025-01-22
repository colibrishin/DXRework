using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class Shader : CommonProject
{
    public Shader() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPrivateDependency<ResourceManager>(target);
        conf.AddPrivateDependency<DX12Agility>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<ThrowIfFailed>(target);
    }
}