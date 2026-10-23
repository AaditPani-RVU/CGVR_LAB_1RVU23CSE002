using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEngine;

// Lab 10 - the 3D viewing pipeline, done by hand every frame:
//
//   object --M--> world --V--> eye --(near clip)--> --P--> clip --/w--> NDC --viewport--> screen
//
// The same scene goes through two viewports side by side: perspective on the
// left, orthographic on the right. Only the final 2D pixel-space segments are
// handed to the GPU (via GL in OnPostRender). Underneath, two ordinary Unity
// cameras with the same eye / fov / near / far / viewport render the solid
// objects, so the hand-projected wireframe landing exactly on Unity's own
// rendering shows that the matrices are right. The HUD also prints how far
// the hand-built V and P are from Unity's worldToCameraMatrix and
// projectionMatrix.
//
// Attach to a full-screen overlay camera (cullingMask 0, clears nothing,
// highest depth) so OnPostRender runs after both scene cameras.
[RequireComponent(typeof(Camera))]
public class ViewingPipeline : MonoBehaviour
{
    public Camera perspectiveCamera;
    public Camera orthographicCamera;
    public Material lineMaterial;

    [Header("Viewing transformation (eye orbits the target)")]
    public Vector3 target = new Vector3(0f, 0.5f, 5f);
    public float orbitRadius = 21f;
    public float azimuth = -32f;
    public float elevation = 24f;
    public bool autoOrbit = true;
    public float autoOrbitSpeed = 10f;
    public float autoOrbitDelay = 3f;

    [Header("Projection")]
    public float fovY = 45f;
    public float orthoHalfHeight = 8.5f;
    public float near = 0.5f;
    public float far = 60f;

    [Header("Drawing")]
    public float lineWidth = 2f;
    public string tracedObject = "RotatedCube";
    public int tracedVertex = 6;
    public float traceAfterSeconds = 1.5f;

    class Trace
    {
        public Vector3 obj, world;
        public Vector4 eye, clip;
        public Vector3 ndc;
        public Vector2 screen;
    }

    class View
    {
        public string title;
        public Camera cam;
        public bool orthographic;
        public Rect rect;
        public Matrix4x4 P;
        public float pError;
        public Trace trace;
        public readonly List<Vector2> points = new List<Vector2>();
        public readonly List<Color> colors = new List<Color>();
    }

    View[] views;
    PipelineObject[] objects;
    Matrix4x4 V;
    Vector3 eye;
    float vError;
    bool showSolids = true;
    bool traceWritten;
    string status = "";
    GUIStyle titleStyle, textStyle, monoStyle;

    void Start()
    {
        objects = FindObjectsByType<PipelineObject>(FindObjectsSortMode.InstanceID);
        views = new[]
        {
            new View { title = "Perspective projection", cam = perspectiveCamera, orthographic = false },
            new View { title = "Orthographic projection", cam = orthographicCamera, orthographic = true },
        };
    }

    void LateUpdate()
    {
        HandleInput();

        // ---- Viewing transformation --------------------------------------
        float az = azimuth * Mathf.Deg2Rad, el = elevation * Mathf.Deg2Rad;
        eye = target + orbitRadius * new Vector3(
            Mathf.Cos(el) * Mathf.Sin(az), Mathf.Sin(el), -Mathf.Cos(el) * Mathf.Cos(az));
        V = Mat.LookAt(eye, target, Vector3.up);

        vError = 0f;
        foreach (View view in views)
        {
            // Keep the Unity camera in step, purely so its solid render can be
            // compared with the hand-projected wireframe.
            Camera cam = view.cam;
            cam.transform.SetPositionAndRotation(eye, Quaternion.LookRotation(target - eye, Vector3.up));
            cam.nearClipPlane = near;
            cam.farClipPlane = far;
            cam.orthographic = view.orthographic;
            cam.fieldOfView = fovY;
            cam.orthographicSize = orthoHalfHeight;

            // ---- Projection (aspect comes from the viewport) -------------
            view.rect = cam.pixelRect;
            float aspect = view.rect.width / view.rect.height;
            view.P = view.orthographic
                ? Mat.Orthographic(orthoHalfHeight, aspect, near, far)
                : Mat.Perspective(fovY, aspect, near, far);

            vError = Mathf.Max(vError, Mat.MaxAbsDifference(V, cam.worldToCameraMatrix));
            view.pError = Mat.MaxAbsDifference(view.P, cam.projectionMatrix);

            view.points.Clear();
            view.colors.Clear();
            view.trace = null;
        }

        foreach (PipelineObject obj in objects)
        {
            // ---- Modeling transformation ---------------------------------
            Matrix4x4 M = obj.ModelMatrix();
            Matrix4x4 VM = V * M;
            Vector3[] verts = obj.Vertices;
            int[] edges = obj.Edges;

            foreach (View view in views)
            {
                for (int i = 0; i < edges.Length; i += 2)
                {
                    Vector4 a = VM * Point(verts[edges[i]]);
                    Vector4 b = VM * Point(verts[edges[i + 1]]);
                    if (!ClipToNearPlane(ref a, ref b)) continue;
                    view.points.Add(ToScreen(a, view));
                    view.points.Add(ToScreen(b, view));
                    view.colors.Add(obj.wireColor);
                }

                if (obj.name == tracedObject)
                {
                    view.trace = TraceVertex(verts[tracedVertex], M, view);
                }
            }
        }

        if (!traceWritten && Time.timeSinceLevelLoad >= traceAfterSeconds)
        {
            traceWritten = true;
            WriteTrace();
        }
    }

    static Vector4 Point(Vector3 p) => new Vector4(p.x, p.y, p.z, 1f);

    // Clipping in eye space against the near plane z = -near, so no point at
    // or behind the eye is ever divided by w <= 0.
    bool ClipToNearPlane(ref Vector4 a, ref Vector4 b)
    {
        float zNear = -near;
        bool aIn = a.z <= zNear, bIn = b.z <= zNear;
        if (!aIn && !bIn) return false;
        if (!aIn) a = Vector4.Lerp(a, b, (zNear - a.z) / (b.z - a.z));
        if (!bIn) b = Vector4.Lerp(b, a, (zNear - b.z) / (a.z - b.z));
        return true;
    }

    // Projection, perspective divide and viewport mapping.
    static Vector2 ToScreen(Vector4 eyePoint, View view)
    {
        Vector4 clip = view.P * eyePoint;
        Vector3 ndc = new Vector3(clip.x, clip.y, clip.z) / clip.w;
        return Mat.Viewport(ndc, view.rect);
    }

    Trace TraceVertex(Vector3 p, Matrix4x4 M, View view)
    {
        Trace t = new Trace { obj = p };
        Vector4 world = M * Point(p);
        t.world = world;
        t.eye = V * world;
        t.clip = view.P * t.eye;
        t.ndc = new Vector3(t.clip.x, t.clip.y, t.clip.z) / t.clip.w;
        t.screen = Mat.Viewport(t.ndc, view.rect);
        return t;
    }

    void HandleInput()
    {
        float dt = Time.deltaTime;
        if (Input.GetKey(KeyCode.LeftArrow)) { azimuth += 60f * dt; autoOrbit = false; }
        if (Input.GetKey(KeyCode.RightArrow)) { azimuth -= 60f * dt; autoOrbit = false; }
        if (Input.GetKey(KeyCode.UpArrow)) elevation = Mathf.Min(elevation + 40f * dt, 80f);
        if (Input.GetKey(KeyCode.DownArrow)) elevation = Mathf.Max(elevation - 40f * dt, 3f);
        if (Input.GetKey(KeyCode.Equals) || Input.GetKey(KeyCode.KeypadPlus)) fovY = Mathf.Max(fovY - 20f * dt, 15f);
        if (Input.GetKey(KeyCode.Minus) || Input.GetKey(KeyCode.KeypadMinus)) fovY = Mathf.Min(fovY + 20f * dt, 110f);
        if (Input.GetKey(KeyCode.RightBracket)) orthoHalfHeight = Mathf.Max(orthoHalfHeight - 4f * dt, 2f);
        if (Input.GetKey(KeyCode.LeftBracket)) orthoHalfHeight = Mathf.Min(orthoHalfHeight + 4f * dt, 20f);
        if (Input.GetKeyDown(KeyCode.Space)) autoOrbit = !autoOrbit;
        if (Input.GetKeyDown(KeyCode.T)) WriteTrace();
        if (Input.GetKeyDown(KeyCode.Escape)) Application.Quit();
        if (Input.GetKeyDown(KeyCode.H))
        {
            showSolids = !showSolids;
            foreach (PipelineObject obj in objects) obj.GetComponent<Renderer>().enabled = showSolids;
        }

        if (autoOrbit && Time.timeSinceLevelLoad > autoOrbitDelay) azimuth += autoOrbitSpeed * dt;
    }

    // ---- Drawing: only 2D pixel coordinates reach the GPU ------------------

    void OnPostRender()
    {
        if (views == null || lineMaterial == null) return;
        lineMaterial.SetPass(0);

        foreach (View view in views)
        {
            GL.PushMatrix();
            // Restrict drawing to this view's rectangle; the pixel matrix maps
            // the screen coordinates computed above 1:1 onto it.
            GL.Viewport(view.rect);
            GL.LoadPixelMatrix(view.rect.xMin, view.rect.xMax, view.rect.yMin, view.rect.yMax);
            GL.Begin(GL.QUADS);
            for (int i = 0; i < view.colors.Count; i++)
            {
                Segment(view.points[2 * i], view.points[2 * i + 1], view.colors[i], lineWidth);
            }
            if (view.trace != null) Marker(view.trace.screen, new Color(1f, 0.25f, 0.6f), 7f);
            GL.End();
            GL.PopMatrix();
        }

        GL.PushMatrix();
        GL.Viewport(new Rect(0, 0, Screen.width, Screen.height));
        GL.LoadPixelMatrix();
        GL.Begin(GL.QUADS);
        float mid = views[1].rect.xMin;
        Segment(new Vector2(mid, 0), new Vector2(mid, Screen.height), new Color(1f, 1f, 1f, 0.6f), 2f);
        GL.End();
        GL.PopMatrix();
    }

    static void Segment(Vector2 a, Vector2 b, Color color, float width)
    {
        Vector2 d = b - a;
        if (d.sqrMagnitude < 1e-6f) return;
        Vector2 n = new Vector2(-d.y, d.x).normalized * (width * 0.5f);
        GL.Color(color);
        GL.Vertex3(a.x - n.x, a.y - n.y, 0f);
        GL.Vertex3(a.x + n.x, a.y + n.y, 0f);
        GL.Vertex3(b.x + n.x, b.y + n.y, 0f);
        GL.Vertex3(b.x - n.x, b.y - n.y, 0f);
    }

    static void Marker(Vector2 p, Color color, float size)
    {
        GL.Color(color);
        GL.Vertex3(p.x - size, p.y - size, 0f);
        GL.Vertex3(p.x - size, p.y + size, 0f);
        GL.Vertex3(p.x + size, p.y + size, 0f);
        GL.Vertex3(p.x + size, p.y - size, 0f);
    }

    // ---- HUD ----------------------------------------------------------------

    void OnGUI()
    {
        if (views == null) return;
        if (titleStyle == null)
        {
            titleStyle = new GUIStyle(GUI.skin.label) { fontSize = 20, fontStyle = FontStyle.Bold, alignment = TextAnchor.UpperCenter };
            titleStyle.normal.textColor = Color.white;
            textStyle = new GUIStyle(GUI.skin.label) { fontSize = 13, alignment = TextAnchor.UpperCenter };
            textStyle.normal.textColor = new Color(0.85f, 0.85f, 0.85f);
            monoStyle = new GUIStyle(GUI.skin.box) { fontSize = 12, alignment = TextAnchor.UpperLeft, padding = new RectOffset(8, 8, 6, 6) };
            monoStyle.normal.textColor = Color.white;
        }

        foreach (View view in views)
        {
            float top = Screen.height - view.rect.yMax;
            string sub = view.orthographic
                ? $"half-height {orthoHalfHeight:F1}, w = 1  ·  max|P − Unity P| = {view.pError:0.0e+0}"
                : $"fov {fovY:F0}°, w = −z_eye  ·  max|P − Unity P| = {view.pError:0.0e+0}";
            GUI.Label(new Rect(view.rect.xMin, top + 12, view.rect.width, 30), view.title, titleStyle);
            GUI.Label(new Rect(view.rect.xMin, top + 38, view.rect.width, 22), sub, textStyle);

            if (view.trace != null)
            {
                Trace t = view.trace;
                string text =
                    $"{tracedObject} corner {Vec(t.obj)}\n" +
                    $"world  M·p      {Vec(t.world)}\n" +
                    $"eye    V·world  {Vec((Vector3)t.eye)}\n" +
                    $"clip   P·eye    ({t.clip.x:F2}, {t.clip.y:F2}, {t.clip.z:F2}, w={t.clip.w:F2})\n" +
                    $"NDC    clip / w {Vec(t.ndc)}\n" +
                    $"screen viewport ({t.screen.x:F0}, {t.screen.y:F0}) px";
                GUI.Box(new Rect(view.rect.xMin + 12, Screen.height - view.rect.yMin - 158, 330, 120), text, monoStyle);
            }
        }

        string help = $"max|V − Unity V| = {vError:0.0e+0}   ·   ←/→ orbit  ↑/↓ elevation  +/− fov  [ ] ortho size  " +
                      $"Space auto-orbit  H solids  T write trace  Esc quit" + (status.Length > 0 ? "   ·   " + status : "");
        GUI.Label(new Rect(0, Screen.height - 26, Screen.width, 22), help, textStyle);
    }

    static string Vec(Vector3 v) => $"({v.x:F2}, {v.y:F2}, {v.z:F2})";

    // ---- Trace file -----------------------------------------------------------

    void WriteTrace()
    {
        PipelineObject traced = System.Array.Find(objects, o => o.name == tracedObject);
        if (traced == null) return;

        StringBuilder sb = new StringBuilder();
        sb.AppendLine("Lab 10 - 3D viewing pipeline trace (Unity)");
        sb.AppendLine("object --M--> world --V--> eye --P--> clip --/w--> NDC --viewport--> screen");
        sb.AppendLine();
        sb.AppendLine($"Model matrix M of {traced.name} (T * Ry * Rx * S):");
        sb.Append(Mat.Format(traced.ModelMatrix()));
        sb.AppendLine($"  max|M - transform.localToWorldMatrix| = {Mat.MaxAbsDifference(traced.ModelMatrix(), traced.transform.localToWorldMatrix):0.0e+0}");
        sb.AppendLine();
        sb.AppendLine($"View matrix V = lookAt(eye {Vec(eye)}, target {Vec(target)}, up (0, 1, 0)):");
        sb.Append(Mat.Format(V));
        sb.AppendLine($"  max|V - camera.worldToCameraMatrix| = {vError:0.0e+0}");

        foreach (View view in views)
        {
            sb.AppendLine();
            sb.AppendLine($"{view.title}: viewport {view.rect.width:F0}x{view.rect.height:F0} at ({view.rect.x:F0}, {view.rect.y:F0}), near {near}, far {far}");
            sb.Append(Mat.Format(view.P));
            sb.AppendLine($"  max|P - camera.projectionMatrix| = {view.pError:0.0e+0}");

            Trace t = TraceVertex(traced.Vertices[tracedVertex], traced.ModelMatrix(), view);
            Vector3 unityScreen = view.cam.WorldToScreenPoint(t.world);
            sb.AppendLine($"  object  {Vec(t.obj)}");
            sb.AppendLine($"  world   {Vec(t.world)}");
            sb.AppendLine($"  eye     {Vec((Vector3)t.eye)}");
            sb.AppendLine($"  clip    ({t.clip.x:F4}, {t.clip.y:F4}, {t.clip.z:F4}, {t.clip.w:F4})");
            sb.AppendLine($"  NDC     ({t.ndc.x:F4}, {t.ndc.y:F4}, {t.ndc.z:F4})");
            sb.AppendLine($"  screen  ({t.screen.x:F2}, {t.screen.y:F2})   Unity WorldToScreenPoint: ({unityScreen.x:F2}, {unityScreen.y:F2})");
        }

        string path = Path.Combine(Application.persistentDataPath, "pipeline_trace.txt");
        File.WriteAllText(path, sb.ToString());
        status = "trace saved";
        Debug.Log("Saved pipeline trace to " + path);
    }
}
