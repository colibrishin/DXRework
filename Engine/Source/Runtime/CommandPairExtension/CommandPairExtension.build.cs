using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/Boost/Boost.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DX12Agility/DX12Agility.build.cs")]

[Generate]
public class CommandPairExtension : CommonProject
{
    public CommandPairExtension() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<DX12Agility>(target);
        conf.AddPublicDependency<CommandPair>(target);
        conf.AddPublicDependency<DescriptorHeap>(target);
    }
}