using UnityEngine;

// Hand-built 4x4 homogeneous matrices (column vectors, p' = M * p) for every
// stage of the viewing pipeline. Nothing here calls Unity's own
// Matrix4x4.TRS / LookAt / Perspective / Ortho helpers; ViewingPipeline only
// uses Unity's camera matrices afterwards, to check these against them.
public static class Mat
{
    public static Matrix4x4 Translate(Vector3 t)
    {
        Matrix4x4 m = Matrix4x4.identity;
        m.m03 = t.x; m.m13 = t.y; m.m23 = t.z;
        return m;
    }

    public static Matrix4x4 Scale(Vector3 s)
    {
        Matrix4x4 m = Matrix4x4.identity;
        m.m00 = s.x; m.m11 = s.y; m.m22 = s.z;
        return m;
    }

    public static Matrix4x4 RotateX(float degrees)
    {
        float r = degrees * Mathf.Deg2Rad, c = Mathf.Cos(r), s = Mathf.Sin(r);
        Matrix4x4 m = Matrix4x4.identity;
        m.m11 = c; m.m12 = -s;
        m.m21 = s; m.m22 = c;
        return m;
    }

    public static Matrix4x4 RotateY(float degrees)
    {
        float r = degrees * Mathf.Deg2Rad, c = Mathf.Cos(r), s = Mathf.Sin(r);
        Matrix4x4 m = Matrix4x4.identity;
        m.m00 = c;  m.m02 = s;
        m.m20 = -s; m.m22 = c;
        return m;
    }

    // Viewing transformation. Builds the camera basis
    //   u (right), v (up), n (backwards, = -forward)
    // and moves the eye to the origin, so the camera looks down -z in eye
    // space - the same convention as OpenGL and Unity's worldToCameraMatrix.
    public static Matrix4x4 LookAt(Vector3 eye, Vector3 target, Vector3 up)
    {
        Vector3 f = (target - eye).normalized;
        Vector3 u = Vector3.Cross(up, f).normalized;
        Vector3 v = Vector3.Cross(f, u);
        Vector3 n = -f;

        Matrix4x4 m = Matrix4x4.identity;
        m.m00 = u.x; m.m01 = u.y; m.m02 = u.z; m.m03 = -Vector3.Dot(u, eye);
        m.m10 = v.x; m.m11 = v.y; m.m12 = v.z; m.m13 = -Vector3.Dot(v, eye);
        m.m20 = n.x; m.m21 = n.y; m.m22 = n.z; m.m23 = -Vector3.Dot(n, eye);
        return m;
    }

    // Perspective projection of the view frustum into the clip cube. Sets
    // w = -z_eye, so the later divide by w shrinks distant points.
    public static Matrix4x4 Perspective(float fovYDegrees, float aspect, float near, float far)
    {
        float f = 1f / Mathf.Tan(fovYDegrees * 0.5f * Mathf.Deg2Rad);
        Matrix4x4 m = Matrix4x4.zero;
        m.m00 = f / aspect;
        m.m11 = f;
        m.m22 = (far + near) / (near - far);
        m.m23 = 2f * far * near / (near - far);
        m.m32 = -1f;
        return m;
    }

    // Orthographic (parallel) projection of a box into the clip cube; w stays 1.
    public static Matrix4x4 Orthographic(float halfHeight, float aspect, float near, float far)
    {
        float halfWidth = halfHeight * aspect;
        Matrix4x4 m = Matrix4x4.identity;
        m.m00 = 1f / halfWidth;
        m.m11 = 1f / halfHeight;
        m.m22 = -2f / (far - near);
        m.m23 = -(far + near) / (far - near);
        return m;
    }

    // Viewport mapping: NDC [-1, 1]^2 -> pixel rectangle on the screen.
    public static Vector2 Viewport(Vector3 ndc, Rect viewport)
    {
        return new Vector2(
            viewport.x + (ndc.x + 1f) * 0.5f * viewport.width,
            viewport.y + (ndc.y + 1f) * 0.5f * viewport.height);
    }

    public static float MaxAbsDifference(Matrix4x4 a, Matrix4x4 b)
    {
        float max = 0f;
        for (int i = 0; i < 16; i++) max = Mathf.Max(max, Mathf.Abs(a[i] - b[i]));
        return max;
    }

    public static string Format(Matrix4x4 m)
    {
        var sb = new System.Text.StringBuilder();
        for (int r = 0; r < 4; r++)
        {
            sb.AppendFormat("  [{0,9:F4} {1,9:F4} {2,9:F4} {3,9:F4} ]\n", m[r, 0], m[r, 1], m[r, 2], m[r, 3]);
        }
        return sb.ToString();
    }
}
