using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DX12Agility/DX12Agility.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/DirectXTK/DirectXTK.build.cs")]


[Generate]
public class PhysicsManager : CommonProject
{
    public PhysicsManager() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<TBB>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPublicDependency<DirectXTK>(target);

        conf.Defines.Add("CFG_RESTITUTION_COEFFICIENT=0.666f");
        conf.Defines.Add("CFG_DRAG_COEFFICIENT=0.25f");
        conf.Defines.Add("CFG_SPECULATION_ENABLED=1");
        conf.Defines.Add("CFG_SPECULATION_BISECTION_MAX_ITERATION=(1 << 7)");
    }
}