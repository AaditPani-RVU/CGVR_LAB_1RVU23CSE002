using System.IO;
using UnityEngine;

// Auto-captures a screenshot a couple of seconds after the scene starts,
// mirroring the auto-screenshot pattern used by the OpenGL labs elsewhere in
// this repo. Always writes to Application.persistentDataPath - a
// guaranteed-writable, well-defined location on every platform - and logs
// the resolved path; copy that file into this lab's screenshots/ folder.
public class ScreenshotCapture : MonoBehaviour
{
    public string fileName = "lab9_scene.png";
    public float captureAfterSeconds = 2f;

    private bool captured;

    void Update()
    {
        if (!captured && Time.timeSinceLevelLoad >= captureAfterSeconds)
        {
            captured = true;
            string path = Path.Combine(Application.persistentDataPath, fileName);
            ScreenCapture.CaptureScreenshot(path);
            Debug.Log("Saved screenshot to " + path);
        }
    }
}
