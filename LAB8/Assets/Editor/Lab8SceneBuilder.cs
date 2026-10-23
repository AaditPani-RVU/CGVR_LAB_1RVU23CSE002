using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

// Builds the Lab-8 scene entirely from code: a camera, a directional light,
// the vertex-coloured triangle and the GPU-animated wave grid, each with a
// material that uses one of this project's custom vertex/fragment shaders.
// Runs from the menu (Lab8 > Build Sample Scene) or headlessly via:
//   Unity -batchmode -quit -projectPath <this folder> -executeMethod Lab8SceneBuilder.BuildScene
public static class Lab8SceneBuilder
{
    [MenuItem("Lab8/Build Sample Scene")]
    public static void BuildScene()
    {
        Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

        // Materials are saved as assets so the shaders they reference are
        // included in the standalone build.
        Material vertexColorMat = SaveMaterial("Lab8/VertexColor", "Assets/Materials/VertexColor.mat");
        Material waveMat = SaveMaterial("Lab8/WaveLit", "Assets/Materials/WaveLit.mat");

        GameObject cameraGO = new GameObject("Main Camera");
        cameraGO.tag = "MainCamera";
        Camera cam = cameraGO.AddComponent<Camera>();
        cam.clearFlags = CameraClearFlags.SolidColor;
        cam.backgroundColor = new Color(0.08f, 0.08f, 0.11f);
        cam.fieldOfView = 50f;
        cameraGO.AddComponent<AudioListener>();
        cameraGO.transform.position = new Vector3(0f, 2.2f, -6.5f);
        cameraGO.transform.LookAt(new Vector3(0f, 0.4f, 0f));

        GameObject lightGO = new GameObject("Directional Light");
        Light light = lightGO.AddComponent<Light>();
        light.type = LightType.Directional;
        light.intensity = 1.1f;
        lightGO.transform.rotation = Quaternion.Euler(45f, -40f, 0f);

        GameObject triangle = new GameObject("ColoredTriangle");
        triangle.AddComponent<MeshFilter>();
        triangle.AddComponent<MeshRenderer>().sharedMaterial = vertexColorMat;
        ColoredTriangle tri = triangle.AddComponent<ColoredTriangle>();
        tri.size = 2.6f;
        triangle.AddComponent<Spin>();
        triangle.transform.position = new Vector3(-1.9f, 0.9f, 0f);

        GameObject wave = new GameObject("WaveGrid");
        wave.AddComponent<MeshFilter>();
        wave.AddComponent<MeshRenderer>().sharedMaterial = waveMat;
        wave.AddComponent<GridMesh>();
        wave.transform.position = new Vector3(1.9f, 0.3f, 0.3f);
        wave.transform.rotation = Quaternion.Euler(0f, -25f, 0f);

        GameObject labelsGO = new GameObject("Labels");
        Labels labels = labelsGO.AddComponent<Labels>();
        labels.triangle = triangle.transform;
        labels.wave = wave.transform;

        GameObject screenshotGO = new GameObject("ScreenshotCapture");
        screenshotGO.AddComponent<ScreenshotCapture>();

        Directory.CreateDirectory("Assets/Scenes");
        EditorSceneManager.SaveScene(scene, "Assets/Scenes/Lab8Scene.unity");
        Debug.Log("Lab8 scene built and saved to Assets/Scenes/Lab8Scene.unity");
    }

    static Material SaveMaterial(string shaderName, string path)
    {
        Shader shader = Shader.Find(shaderName);
        if (shader == null)
        {
            throw new System.Exception("Shader not found: " + shaderName);
        }
        Directory.CreateDirectory(Path.GetDirectoryName(path));
        AssetDatabase.DeleteAsset(path);
        Material mat = new Material(shader);
        AssetDatabase.CreateAsset(mat, path);
        AssetDatabase.SaveAssets();
        return mat;
    }
}
