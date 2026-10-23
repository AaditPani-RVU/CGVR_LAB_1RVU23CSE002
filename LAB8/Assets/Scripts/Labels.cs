using UnityEngine;

// On-screen captions (IMGUI) naming what each shader pair is doing.
public class Labels : MonoBehaviour
{
    public Transform triangle;
    public Transform wave;

    GUIStyle style;
    GUIStyle titleStyle;

    void OnGUI()
    {
        if (style == null)
        {
            style = new GUIStyle(GUI.skin.label) { fontSize = 18, alignment = TextAnchor.UpperCenter };
            style.normal.textColor = Color.white;
            titleStyle = new GUIStyle(style) { fontSize = 26, fontStyle = FontStyle.Bold };
        }

        GUI.Label(new Rect(0, 20, Screen.width, 40), "Lab 8 - Vertex & Fragment Shaders (Unity)", titleStyle);
        Caption(triangle, "Lab8/VertexColor\nper-vertex RGB, interpolated by the rasteriser");
        Caption(wave, "Lab8/WaveLit\nvertex: wave displacement + normals\nfragment: height colour + Lambert");
    }

    void Caption(Transform target, string text)
    {
        if (target == null) return;
        Vector3 p = Camera.main.WorldToScreenPoint(target.position + Vector3.down * 1.6f);
        GUI.Label(new Rect(p.x - 250, Screen.height - p.y, 500, 90), text, style);
    }
}
