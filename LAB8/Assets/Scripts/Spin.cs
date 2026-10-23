using UnityEngine;

// Rocks an object about the Y axis so the per-vertex colours can be seen
// staying attached to their vertices while the triangle moves. The period is
// 4 s, so the triangle faces the camera at t = 0, 2, 4, ... (including the
// 2 s auto-screenshot).
public class Spin : MonoBehaviour
{
    public float amplitudeDegrees = 40f;
    public float periodSeconds = 4f;

    Quaternion baseRotation;

    void Start()
    {
        baseRotation = transform.rotation;
    }

    void Update()
    {
        float angle = amplitudeDegrees * Mathf.Sin(2f * Mathf.PI * Time.timeSinceLevelLoad / periodSeconds);
        transform.rotation = Quaternion.Euler(0f, angle, 0f) * baseRotation;
    }
}
