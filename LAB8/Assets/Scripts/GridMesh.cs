using UnityEngine;

// A flat, finely subdivided grid in the XZ plane. It carries no animation of
// its own: WaveLit.shader's vertex stage displaces it on the GPU every frame.
[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class GridMesh : MonoBehaviour
{
    public int resolution = 80;
    public float size = 3f;

    void Awake()
    {
        int n = resolution + 1;
        Vector3[] vertices = new Vector3[n * n];
        for (int z = 0; z < n; z++)
        {
            for (int x = 0; x < n; x++)
            {
                vertices[z * n + x] = new Vector3(
                    (x / (float)resolution - 0.5f) * size, 0f,
                    (z / (float)resolution - 0.5f) * size);
            }
        }

        int[] triangles = new int[resolution * resolution * 6];
        int t = 0;
        for (int z = 0; z < resolution; z++)
        {
            for (int x = 0; x < resolution; x++)
            {
                int i = z * n + x;
                triangles[t++] = i;
                triangles[t++] = i + n;
                triangles[t++] = i + 1;
                triangles[t++] = i + 1;
                triangles[t++] = i + n;
                triangles[t++] = i + n + 1;
            }
        }

        Mesh mesh = new Mesh { name = "WaveGrid" };
        mesh.vertices = vertices;
        mesh.triangles = triangles;
        // Pad the bounds so the GPU-displaced crests are never frustum-culled.
        mesh.bounds = new Bounds(Vector3.zero, new Vector3(size, 2f, size));
        GetComponent<MeshFilter>().mesh = mesh;
    }
}
