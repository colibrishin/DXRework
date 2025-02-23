using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]
[module: Include("%EngineDir%/Engine/Source/ThirdParty/ImGui/ImGui.build.cs")]

[Generate]
public class ImGuiManager : EngineCommonProject
{
    public ImGuiManager() { }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency<Core>(target);
        conf.AddPublicDependency<CoreRender>(target);
        conf.AddPublicDependency<ImGui>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPrivateDependency<RenderPipeline>(target);
        
        conf.AddPrivateDependency<EngineEntryPoint>(target);
        if (target.Platform == Platform.win64 || target.Platform == Platform.win32)
        {
            conf.AddPrivateDependency<WinAPIWrapper>(target);
        }

        if (target.GraphicAPI == EGraphicAPI.D3D12)
        {
            conf.AddPrivateDependency<D3D12GraphicInterface>(target);
        }
    }
}