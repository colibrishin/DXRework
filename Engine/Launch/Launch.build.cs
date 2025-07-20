using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Monolith/MonolithClient.build.cs")]
[module: Include("%EngineDir%/Engine/Monolith/MonolithServer.build.cs")]

[Generate]
public class Launch : EngineCommonProject
{
    public Launch() {}

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        
        if (target.LaunchType == ELaunchType.Editor)
        {
            conf.AddPrivateDependency<Boost>(target);
            conf.AddPrivateDependency<Core>(target);
            conf.AddPrivateDependency<CoreModule>(target);
            conf.AddPrivateDependency<CoreType>(target);

            if (target.Platform == Platform.win64 || target.Platform == Platform.win32) 
            {
                conf.AddPrivateDependency<WinAPIWrapper>(target);
            }

            if (target.GraphicAPI == EGraphicAPI.D3D12) 
            {
                conf.AddPrivateDependency<D3D12GraphicInterface>(target);
            }
        }
        else if (target.LaunchType == ELaunchType.Client)
        {
            conf.AddPrivateDependency<MonolithClient>(target);
        }
        else if (target.LaunchType == ELaunchType.Server)
        {
            conf.AddPrivateDependency<MonolithServer>(target);
        }
        
        conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);
        conf.Output = Configuration.OutputType.Exe;
    }
}