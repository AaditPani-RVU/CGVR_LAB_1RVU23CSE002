using UnityEditor;

// Builds a standalone Mac player of the Lab10 scene, so it can be launched
// and screenshotted from the terminal the same way the OpenGL labs are.
// Output goes to UnityBuild/ (not Build/) so it can't collide with the CMake
// build/ folder on macOS's case-insensitive filesystem.
// Run headlessly via:
//   Unity -batchmode -quit -projectPath <this folder> -executeMethod Lab10Builder.BuildStandalonePlayer
public static class Lab10Builder
{
    public static void BuildStandalonePlayer()
    {
        PlayerSettings.fullScreenMode = UnityEngine.FullScreenMode.Windowed;
        PlayerSettings.defaultScreenWidth = 1280;
        PlayerSettings.defaultScreenHeight = 720;
        PlayerSettings.resizableWindow = true;
        PlayerSettings.runInBackground = true;

        BuildPlayerOptions options = new BuildPlayerOptions
        {
            scenes = new[] { "Assets/Scenes/Lab10Scene.unity" },
            locationPathName = "UnityBuild/Lab10.app",
            target = BuildTarget.StandaloneOSX,
            options = BuildOptions.None
        };
        BuildPipeline.BuildPlayer(options);
    }
}
