using UnityEngine;

// One object in the Lab-10 scene. It owns:
//  - its modeling parameters (translation, rotation, scale), from which
//    ModelMatrix() builds M = T * Ry * Rx * S by hand, and
//  - its wireframe in object space (vertices + edge index pairs).
// It also applies the same parameters to its own Transform so Unity's
// renderer draws a solid copy underneath the hand-projected wireframe.
public class PipelineObject : MonoBehaviour
{
    public enum Shape { Cube, Pyramid, Grid }

    public Shape shape = Shape.Cube;
    public Vector3 translation;
    public Vector2 rotationXY;            // degrees about X, then Y
    public Vector3 scale = Vector3.one;
    public Color wireColor = Color.white;

    public Vector3[] Vertices { get; private set; }
    public int[] Edges { get; private set; }

    void Awake()
    {
        transform.SetPositionAndRotation(translation, Quaternion.Euler(rotationXY.x, rotationXY.y, 0f));
        transform.localScale = scale;

        switch (shape)
        {
            case Shape.Cube: BuildCube(); break;
            case Shape.Pyramid: BuildPyramid(); break;
            case Shape.Grid: BuildGrid(); break;
        }
    }

    // ---- Modeling transformation -------------------------------------------

    public Matrix4x4 ModelMatrix()
    {
        return Mat.Translate(translation) * Mat.RotateY(rotationXY.y) * Mat.RotateX(rotationXY.x) * Mat.Scale(scale);
    }

    // ---- Geometry (object space) -------------------------------------------

    void BuildCube()
    {
        // Matches Unity's built-in cube: unit cube centred at the origin.
        Vertices = new[]
        {
            new Vector3(-0.5f, -0.5f, -0.5f), new Vector3(0.5f, -0.5f, -0.5f),
            new Vector3(0.5f, 0.5f, -0.5f),   new Vector3(-0.5f, 0.5f, -0.5f),
            new Vector3(-0.5f, -0.5f, 0.5f),  new Vector3(0.5f, -0.5f, 0.5f),
            new Vector3(0.5f, 0.5f, 0.5f),    new Vector3(-0.5f, 0.5f, 0.5f),
        };
        Edges = new[] { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };
    }

    void BuildPyramid()
    {
        Vector3 a = new Vector3(-0.5f, -0.5f, -0.5f), b = new Vector3(0.5f, -0.5f, -0.5f);
        Vector3 c = new Vector3(0.5f, -0.5f, 0.5f), d = new Vector3(-0.5f, -0.5f, 0.5f);
        Vector3 apex = new Vector3(0f, 0.5f, 0f);
        Vertices = new[] { a, b, c, d, apex };
        Edges = new[] { 0, 1, 1, 2, 2, 3, 3, 0, 0, 4, 1, 4, 2, 4, 3, 4 };

        // Flat-shaded solid: separate vertices per face so normals don't smooth.
        Vector3[] tris =
        {
            a, apex, b,   b, apex, c,   c, apex, d,   d, apex, a,
            a, b, c,      a, c, d,
        };
        int[] indices = new int[tris.Length];
        for (int i = 0; i < indices.Length; i++) indices[i] = i;
        Mesh mesh = new Mesh { name = "Pyramid", vertices = tris, triangles = indices };
        mesh.RecalculateNormals();
        mesh.RecalculateBounds();
        GetComponent<MeshFilter>().mesh = mesh;
    }

    void BuildGrid()
    {
        // Matches Unity's built-in plane: 10 x 10 units in XZ, so a 1-unit grid
        // in object space lines up with the solid plane after scaling.
        const int half = 5;
        int lines = 2 * half + 1;
        Vertices = new Vector3[lines * 4];
        Edges = new int[lines * 4];
        int v = 0;
        for (int i = -half; i <= half; i++)
        {
            Vertices[v] = new Vector3(i, 0f, -half); Edges[v] = v; v++;
            Vertices[v] = new Vector3(i, 0f, half);  Edges[v] = v; v++;
            Vertices[v] = new Vector3(-half, 0f, i); Edges[v] = v; v++;
            Vertices[v] = new Vector3(half, 0f, i);  Edges[v] = v; v++;
        }
    }
}
