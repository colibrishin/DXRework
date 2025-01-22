using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]

[Generate]
public class Debugger : CommonProject
{
    public Debugger() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);        
        conf.AddPublicDependency<DirectXTK>(target);
        conf.AddPublicDependency<DX12Agility>(target);
        conf.AddPrivateDependency<D3D12Wrapper>(target);
        conf.AddPrivateDependency<SceneManager>(target);
        conf.AddPrivateDependency<D3D12Toolkit>(target);
        conf.AddPrivateDependency<InputManager>(target);
    }
}