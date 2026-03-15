using System;
using System.IO;
using System.Text.RegularExpressions;
using Sharpmake;

[module: Include("%EngineDir%/Build/Utils.cs")]

/// <summary>
/// [Compile] project that references the existing header-parser.vcxproj (e.g. from CMake).
/// Added to the solution only when header-parser.vcxproj exists. Single exe project; config names
/// are "Debug" / "Release" to match the two vcxproj configs.
/// </summary>
[Compile]
public class HeaderParserProject : Project
{
    public HeaderParserProject() : base(typeof(EngineTarget))
    {
        Name = "header-parser";
        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;
        AddTargets(Utils.GetDefinedTarget());
    }

    [Configure]
    public void ConfigureAll(Configuration conf, EngineTarget target)
    {
        conf.SolutionFolder = @"ThirdParty";
        conf.Output = Project.Configuration.OutputType.Exe;

        string solutionDir = Utils.GetSolutionDir();
        string projectDir = Path.Combine(solutionDir, "Programs", "header-parser");
        string vcxprojPath = Path.Combine(projectDir, "header-parser.vcxproj");

        conf.ProjectPath = projectDir;
        conf.ProjectFileName = "header-parser";

        // Force to use the relase version of header-parser
        conf.Name = "Release";

        // Avoid "Unrecognized Guid format" when the generator uses this config and ReadGuidFromProjectFile returns null.
        if (File.Exists(vcxprojPath))
        {
            try
            {
                string content = File.ReadAllText(vcxprojPath);
                Match m = Regex.Match(content, @"<ProjectGuid>\s*\{([^}]+)\}\s*</ProjectGuid>");
                if (m.Success)
                    conf.ProjectGuid = "{" + m.Groups[1].Value + "}";
            }
            catch { }
        }
    }
}
