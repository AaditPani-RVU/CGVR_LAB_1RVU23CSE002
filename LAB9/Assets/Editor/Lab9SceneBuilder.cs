using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

// Builds the Lab-9 sample scene entirely from code: a ground plane, a
// WASD-controlled player, a follow camera, a handful of spinning
// collectibles, and a score UI - so no manual GameObject assembly is needed
// in the Editor. Runs from the menu (Lab9 > Build Sample Scene) or headlessly
// via:
//   Unity -batchmode -quit -projectPath <this folder> -executeMethod Lab9SceneBuilder.BuildScene
public static class Lab9SceneBuilder
{
    [MenuItem("Lab9/Build Sample Scene")]
    public static void BuildScene()
    {
        Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

        GameObject ground = GameObject.CreatePrimitive(PrimitiveType.Plane);
        ground.name = "Ground";

        GameObject lightGO = new GameObject("Directional Light");
        Light light = lightGO.AddComponent<Light>();
        light.type = LightType.Directional;
        lightGO.transform.rotation = Quaternion.Euler(50f, -30f, 0f);

        GameObject player = GameObject.CreatePrimitive(PrimitiveType.Capsule);
        player.name = "Player";
        player.tag = "Player";
        player.transform.position = new Vector3(0f, 1f, 0f);
        Object.DestroyImmediate(player.GetComponent<CapsuleCollider>());
        player.AddComponent<CharacterController>();
        player.AddComponent<PlayerController>();

        GameObject cameraGO = new GameObject("Main Camera");
        cameraGO.tag = "MainCamera";
        cameraGO.AddComponent<Camera>();
        cameraGO.AddComponent<AudioListener>();
        CameraFollow follow = cameraGO.AddComponent<CameraFollow>();
        follow.target = player.transform;
        cameraGO.transform.position = player.transform.position + follow.offset;

        Vector3[] collectiblePositions =
        {
            new Vector3(3f, 0.5f, 3f),
            new Vector3(-3f, 0.5f, 3f),
            new Vector3(3f, 0.5f, -3f),
            new Vector3(-3f, 0.5f, -3f),
            new Vector3(0f, 0.5f, 5f),
        };
        foreach (Vector3 pos in collectiblePositions)
        {
            GameObject item = GameObject.CreatePrimitive(PrimitiveType.Cube);
            item.name = "Collectible";
            item.transform.position = pos;
            item.transform.localScale = Vector3.one * 0.6f;
            item.GetComponent<Collider>().isTrigger = true;
            item.AddComponent<Collectible>();
            item.GetComponent<Renderer>().material.color = Color.yellow;
        }

        GameObject canvasGO = new GameObject("Canvas");
        Canvas canvas = canvasGO.AddComponent<Canvas>();
        canvas.renderMode = RenderMode.ScreenSpaceOverlay;
        canvasGO.AddComponent<CanvasScaler>();
        canvasGO.AddComponent<GraphicRaycaster>();

        GameObject textGO = new GameObject("ScoreText");
        textGO.transform.SetParent(canvasGO.transform, false);
        Text text = textGO.AddComponent<Text>();
        Font font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        if (font == null)
        {
            font = Resources.GetBuiltinResource<Font>("Arial.ttf");
        }
        text.font = font;
        text.fontSize = 28;
        text.color = Color.white;
        text.text = "Score: 0";
        RectTransform rt = text.rectTransform;
        rt.anchorMin = new Vector2(0f, 1f);
        rt.anchorMax = new Vector2(0f, 1f);
        rt.pivot = new Vector2(0f, 1f);
        rt.anchoredPosition = new Vector2(20f, -20f);
        rt.sizeDelta = new Vector2(300f, 50f);

        GameObject scoreManagerGO = new GameObject("ScoreManager");
        ScoreManager scoreManager = scoreManagerGO.AddComponent<ScoreManager>();
        scoreManager.scoreText = text;

        GameObject screenshotGO = new GameObject("ScreenshotCapture");
        screenshotGO.AddComponent<ScreenshotCapture>();

        Directory.CreateDirectory("Assets/Scenes");
        EditorSceneManager.SaveScene(scene, "Assets/Scenes/Lab9Scene.unity");
        Debug.Log("Lab9 scene built and saved to Assets/Scenes/Lab9Scene.unity");
    }
}
