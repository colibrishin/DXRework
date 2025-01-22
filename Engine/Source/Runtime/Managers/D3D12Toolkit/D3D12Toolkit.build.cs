using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DX12Agility/DX12Agility.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]

[Generate]
public class D3D12Toolkit : CommonProject
{
    public D3D12Toolkit() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<DirectXTK>(target);
        
        conf.AddPrivateDependency<DX12Agility>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<SceneManager>(target);
        //conf.AddPrivateDependency<RenderPipeline>(target);
    }
}