using UnityEngine;

// Builds the same red/green/blue triangle as shader_triangle.cpp's VBO: three
// positions plus a per-vertex colour attribute, uploaded as a Mesh (Unity's
// wrapper around a VAO/VBO/IBO).
[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class ColoredTriangle : MonoBehaviour
{
    public float size = 1.5f;

    void Awake()
    {
        float h = size * Mathf.Sqrt(3f) / 2f;
        Mesh mesh = new Mesh { name = "ColoredTriangle" };
        mesh.vertices = new[]
        {
            new Vector3(-size / 2f, -h / 3f, 0f),
            new Vector3(0f, 2f * h / 3f, 0f),
            new Vector3(size / 2f, -h / 3f, 0f),
        };
        mesh.colors = new[] { Color.red, Color.green, Color.blue };
        mesh.triangles = new[] { 0, 1, 2 };
        mesh.RecalculateBounds();
        GetComponent<MeshFilter>().mesh = mesh;
    }
}
