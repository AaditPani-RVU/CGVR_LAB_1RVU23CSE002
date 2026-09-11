using UnityEditor;

// Builds a standalone Mac player of the Lab9 scene, so it can be launched
// and screenshotted from the terminal the same way the OpenGL labs are.
// Run headlessly via:
//   Unity -batchmode -quit -projectPath <this folder> -executeMethod Lab9Builder.BuildStandalonePlayer
public static class Lab9Builder
{
    public static void BuildStandalonePlayer()
    {
        BuildPlayerOptions options = new BuildPlayerOptions
        {
            scenes = new[] { "Assets/Scenes/Lab9Scene.unity" },
            locationPathName = "Build/Lab9.app",
            target = BuildTarget.StandaloneOSX,
            options = BuildOptions.None
        };
        BuildPipeline.BuildPlayer(options);
    }
}
