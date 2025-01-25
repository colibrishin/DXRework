using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ImGui : CommonProject
{
    public ImGui()
    {
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);

        conf.Defines.Add("IMGUI_DEFINE_MATH_OPERATORS=1");
        conf.Output = Configuration.OutputType.Lib;
    }
}