using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class InputManager : EngineCommonProject
{
    public InputManager() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<Boost>(target);

        if (target.Platform == Platform.win64)
        {
            if (target.GraphicAPI == EGraphicAPI.D3D12)
            {
                conf.AddPrivateDependency<DirectInputInterface>(target);
            }
        }
    }
}