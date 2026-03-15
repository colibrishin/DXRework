using System.IO;
using Sharpmake;

[module: Include("%EngineDir%/Build/CommonProject.build.cs")]

[Generate]
public class ImGui : CommonProject
{
    public ImGui()
    {
        IsExportProject = true;
        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;
        AddTargets(Utils.GetDefinedTarget());
    }

    public override void ConfigureAll(Configuration conf, EngineTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.SolutionFolder = @"ThirdParty";
        // Disable unity/jumbo build so STB_*_IMPLEMENTATION is compiled in exactly one TU per header (avoids redefinition in merged blob).
        conf.Options.Remove(Options.Vc.Compiler.JumboBuild.Enable);
        conf.Defines.Add("IMGUI_DEFINE_MATH_OPERATORS=1");
        // Always static library: no DLL export/import for ImGui in any configuration.
        conf.Output = Configuration.OutputType.Lib;
        conf.ExportDefines.Remove("ENGINE_IMGUI_API=DLLIMPORT");
        conf.ExportDefines.Remove("ENGINE_IMGUI_API=DLLEXPORT");
        conf.Defines.Remove("ENGINE_IMGUI_API=DLLEXPORT");
        conf.Defines.Remove("ENGINE_IMGUI_API=DLLIMPORT");
        conf.ExportDefines.Add("ENGINE_IMGUI_API=");
        conf.Defines.Add("ENGINE_IMGUI_API=");
        // Force IMGUI_API empty from the very start of each TU so no other header or include order can leave it wrong (avoids "int"/redefinition errors).
        conf.Defines.Add("IMGUI_API=");
        conf.Defines.Add("IMGUI_IMPL_API=");
        conf.ExportDefines.Add("IMGUI_API=");
        conf.ExportDefines.Add("IMGUI_IMPL_API=");
    }
}