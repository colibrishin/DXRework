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
        conf.AddPublicDependency<ImGui>(target);
        conf.AddPublicDependency<DirectXTK>(target);
        conf.AddPublicDependency<Boost>(target);
        conf.AddPrivateDependency<RenderPipeline>(target);
        
        conf.AddPrivateDependency<WinAPIWrapper>(target); // todo: use platform flag

        if (target.GraphicAPI == EGraphicAPI.D3D12)
        {
            conf.AddPrivateDependency<D3D12GraphicInterface>(target); // todo: use dx12 dx11 flag
        }
    }
}