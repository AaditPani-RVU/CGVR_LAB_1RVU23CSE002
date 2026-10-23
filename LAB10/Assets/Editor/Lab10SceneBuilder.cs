using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

// Builds the Lab-10 scene entirely from code: a ground grid, a row of
// identical cubes receding into the distance, a pyramid and a rotated cube
// (each a PipelineObject), two scene cameras splitting the screen into a
// perspective and an orthographic viewport, and a full-screen overlay camera
// that runs ViewingPipeline. Runs from the menu (Lab10 > Build Sample Scene)
// or headlessly via:
//   Unity -batchmode -quit -projectPath <this folder> -executeMethod Lab10SceneBuilder.BuildScene
public static class Lab10SceneBuilder
{
    static readonly Color Background = new Color(0.07f, 0.07f, 0.1f);

    [MenuItem("Lab10/Build Sample Scene")]
    public static void BuildScene()
    {
        Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

        GameObject lightGO = new GameObject("Directional Light");
        Light light = lightGO.AddComponent<Light>();
        light.type = LightType.Directional;
        lightGO.transform.rotation = Quaternion.Euler(50f, -30f, 0f);

        // ---- Scene objects (modeling parameters live on PipelineObject) -----
        AddObject("Ground", PipelineObject.Shape.Grid, new Vector3(0f, 0f, 5f), Vector2.zero,
            new Vector3(1.2f, 1f, 1.8f), new Color(0.45f, 0.5f, 0.6f), new Color(0.18f, 0.2f, 0.24f));

        Color cubeWire = new Color(0.35f, 0.85f, 1f);
        for (int i = 0; i < 5; i++)
        {
            AddObject("Cube" + i, PipelineObject.Shape.Cube, new Vector3(-3f, 0.75f, -1f + 3.5f * i), Vector2.zero,
                Vector3.one * 1.5f, cubeWire, new Color(0.15f, 0.4f, 0.55f));
        }

        AddObject("Pyramid", PipelineObject.Shape.Pyramid, new Vector3(2.8f, 1f, 1f), new Vector2(0f, 20f),
            new Vector3(2.2f, 2f, 2.2f), new Color(1f, 0.8f, 0.3f), new Color(0.6f, 0.45f, 0.15f));
        AddObject("RotatedCube", PipelineObject.Shape.Cube, new Vector3(2.5f, 1.4f, 8f), new Vector2(25f, 40f),
            Vector3.one * 2f, new Color(0.6f, 1f, 0.5f), new Color(0.25f, 0.5f, 0.2f));

        // ---- Cameras -----------------------------------------------------------
        Camera persp = AddSceneCamera("Perspective Camera", new Rect(0f, 0f, 0.5f, 1f), false);
        Camera ortho = AddSceneCamera("Orthographic Camera", new Rect(0.5f, 0f, 0.5f, 1f), true);
        persp.gameObject.tag = "MainCamera";
        persp.gameObject.AddComponent<AudioListener>();

        GameObject overlayGO = new GameObject("Pipeline Overlay Camera");
        Camera overlay = overlayGO.AddComponent<Camera>();
        overlay.clearFlags = CameraClearFlags.Nothing;
        overlay.cullingMask = 0;
        overlay.depth = 10;
        ViewingPipeline pipeline = overlayGO.AddComponent<ViewingPipeline>();
        pipeline.perspectiveCamera = persp;
        pipeline.orthographicCamera = ortho;
        pipeline.lineMaterial = SaveMaterial(Shader.Find("Lab10/Line"), "Assets/Materials/Line.mat", Color.white);

        GameObject screenshotGO = new GameObject("ScreenshotCapture");
        screenshotGO.AddComponent<ScreenshotCapture>();

        Directory.CreateDirectory("Assets/Scenes");
        EditorSceneManager.SaveScene(scene, "Assets/Scenes/Lab10Scene.unity");
        Debug.Log("Lab10 scene built and saved to Assets/Scenes/Lab10Scene.unity");
    }

    static void AddObject(string name, PipelineObject.Shape shape, Vector3 t, Vector2 rotXY, Vector3 s,
        Color wire, Color solid)
    {
        GameObject go;
        switch (shape)
        {
            case PipelineObject.Shape.Cube: go = GameObject.CreatePrimitive(PrimitiveType.Cube); break;
            case PipelineObject.Shape.Grid: go = GameObject.CreatePrimitive(PrimitiveType.Plane); break;
            default:
                go = new GameObject();
                go.AddComponent<MeshFilter>();   // mesh is generated in PipelineObject.Awake
                go.AddComponent<MeshRenderer>();
                break;
        }
        go.name = name;
        Object.DestroyImmediate(go.GetComponent<Collider>());

        PipelineObject obj = go.AddComponent<PipelineObject>();
        obj.shape = shape;
        obj.translation = t;
        obj.rotationXY = rotXY;
        obj.scale = s;
        obj.wireColor = wire;

        go.transform.SetPositionAndRotation(t, Quaternion.Euler(rotXY.x, rotXY.y, 0f));
        go.transform.localScale = s;
        go.GetComponent<MeshRenderer>().sharedMaterial =
            SaveMaterial(Shader.Find("Standard"), "Assets/Materials/" + name + ".mat", solid);
    }

    static Camera AddSceneCamera(string name, Rect rect, bool orthographic)
    {
        GameObject go = new GameObject(name);
        Camera cam = go.AddComponent<Camera>();
        cam.rect = rect;
        cam.orthographic = orthographic;
        cam.clearFlags = CameraClearFlags.SolidColor;
        cam.backgroundColor = Background;
        return cam;
    }

    // Materials are saved as assets so the shaders they reference are included
    // in the standalone build.
    static Material SaveMaterial(Shader shader, string path, Color color)
    {
        if (shader == null)
        {
            throw new System.Exception("Shader not found for " + path);
        }
        Directory.CreateDirectory(Path.GetDirectoryName(path));
        AssetDatabase.DeleteAsset(path);
        Material mat = new Material(shader);
        if (mat.HasProperty("_Color")) mat.color = color;
        AssetDatabase.CreateAsset(mat, path);
        AssetDatabase.SaveAssets();
        return mat;
    }
}
