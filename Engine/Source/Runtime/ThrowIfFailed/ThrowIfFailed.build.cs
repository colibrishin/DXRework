using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DX12Agility/DX12Agility.build.cs")]

[Generate]
public class ThrowIfFailed : CommonProject
{
    public ThrowIfFailed() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPrivateDependency<DX12Agility>(target);
    }
}