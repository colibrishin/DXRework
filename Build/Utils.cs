using System;
using System.IO;
using Sharpmake;

public class Utils
{
    // GenerateSolution.bat 에서 지정됩니다.
    public static string GetEngineDir()
    {
        return Environment.GetEnvironmentVariable("EngineDir");
    }
    // GenerateSolution.bat 에서 지정됩니다.
    public static string GetSolutionDir()
    {
        return Environment.GetEnvironmentVariable("SharpMakeSolutionDir");
    }
    // EngineSolution ConfigureAll에서 지정 됩니다. (vcxproj 생성 위치)
    public static string GetProjectDir()
    {
        return Environment.GetEnvironmentVariable("ProjectFilesDir");
    }

    public static void MakeConfiturationNameDefine(Solution.Configuration conf, EngineTarget target)
    {
        if (target.Name == "Release") { conf.Name = "Development"; }

        // Conf Name
        {
            if (target.LaunchType == ELaunchType.Editor)
            {
                conf.Name += " Editor";
            }
            else if (target.LaunchType == ELaunchType.Client)
            {
                conf.Name += " Client";
            }
            else if (target.LaunchType == ELaunchType.Server)
            {
                conf.Name += " Server";
            }
        }
    }

    public static void MakeConfiturationNameDefine(Project.Configuration conf, EngineTarget target)
    {
        if (target.Name == "Release") { conf.Name = "Development"; }

        // Conf Name
        {
            if (target.LaunchType == ELaunchType.Editor)
            {
                conf.Name += " Editor";
            }
            else if (target.LaunchType == ELaunchType.Client)
            {
                conf.Name += " Client";
            }
            else if (target.LaunchType == ELaunchType.Server)
            {
                conf.Name += " Server";
            }
        }

        // Defines
        {
            conf.Defines.Add("SOLUTION_NAME=\"$(SolutionName)\"");
            conf.Defines.Add("_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING");
            conf.Defines.Add("_HAS_STD_BYTE=0");

            if (target.Name == "Release")
            {
                conf.Defines.Add("WITH_DEBUG=0");
            }
            else
            {
                conf.Defines.Add("WITH_DEBUG=1");
            }

            if (target.LaunchType == ELaunchType.Editor)
            {
                conf.Defines.Add("WITH_EDITOR=1");
                conf.ResourceFileDefine += "WITH_EDITOR=1";
            }
            else
            {
                conf.Defines.Add("WITH_EDITOR=0");
                conf.ResourceFileDefine += "WITH_EDITOR=0";
            }

            if (target.LaunchType == ELaunchType.Client)
            {
                conf.Defines.Add("CLIENT=1");
            }
            else
            {
                conf.Defines.Add("CLIENT=0");
            }

            if (target.LaunchType == ELaunchType.Server)
            {
                conf.Defines.Add("SERVER=1");
            }
            else
            {
                conf.Defines.Add("SERVER=0");
            }
        }
    }
}
