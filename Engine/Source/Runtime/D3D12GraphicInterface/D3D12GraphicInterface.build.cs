using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class D3D12GraphicInterface : CommonProject
{
    public D3D12GraphicInterface() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<DX12Agility>(target);
        conf.AddPrivateDependency<WinAPIWrapper>(target);
        conf.AddPrivateDependency<Texture>(target);
    }
}