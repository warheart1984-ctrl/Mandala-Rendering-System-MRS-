using UnityEngine;

/// <summary>
/// Skeleton: Unity 4D tesseract renderer (drop into a Unity project).
/// Status: skeleton — not executed by the browser host.
/// </summary>
[ExecuteAlways]
public class FourDTesseractRenderer : MonoBehaviour
{
    public float d4 = 4f;
    public float d3 = 4f;
    public float scale = 2f;
    public float speed = 1f;

    Vector4[] verts4D;
    int[,] edges;

    void OnEnable()
    {
        BuildVertices4D();
        BuildEdges();
    }

    void BuildVertices4D()
    {
        verts4D = new Vector4[16];
        int i = 0;
        foreach (int x in new[] { -1, 1 })
        foreach (int y in new[] { -1, 1 })
        foreach (int z in new[] { -1, 1 })
        foreach (int w in new[] { -1, 1 })
            verts4D[i++] = new Vector4(x, y, z, w);
    }

    void BuildEdges()
    {
        edges = new int[32, 2];
        int idx = 0;
        for (int i = 0; i < verts4D.Length; i++)
        for (int j = i + 1; j < verts4D.Length; j++)
        {
            int diff = 0;
            for (int k = 0; k < 4; k++)
                if (verts4D[i][k] != verts4D[j][k]) diff++;
            if (diff == 1)
            {
                edges[idx, 0] = i;
                edges[idx, 1] = j;
                idx++;
            }
        }
    }

    Vector4 Rotate4D(Vector4 p, float t)
    {
        float a = t * 0.7f;
        float b = t * 1.1f;
        float c = t * 1.5f;
        float d = t * 2.0f;
        p = RotateXW(p, a);
        p = RotateYZ(p, b);
        p = RotateZW(p, c);
        p = RotateYW(p, d);
        return p;
    }

    Vector4 RotateXW(Vector4 p, float theta)
    {
        float c = Mathf.Cos(theta), s = Mathf.Sin(theta);
        float x = c * p.x - s * p.w;
        float w = s * p.x + c * p.w;
        return new Vector4(x, p.y, p.z, w);
    }

    Vector4 RotateYZ(Vector4 p, float theta)
    {
        float c = Mathf.Cos(theta), s = Mathf.Sin(theta);
        float y = c * p.y - s * p.z;
        float z = s * p.y + c * p.z;
        return new Vector4(p.x, y, z, p.w);
    }

    Vector4 RotateZW(Vector4 p, float theta)
    {
        float c = Mathf.Cos(theta), s = Mathf.Sin(theta);
        float z = c * p.z - s * p.w;
        float w = s * p.z + c * p.w;
        return new Vector4(p.x, p.y, z, w);
    }

    Vector4 RotateYW(Vector4 p, float theta)
    {
        float c = Mathf.Cos(theta), s = Mathf.Sin(theta);
        float y = c * p.y - s * p.w;
        float w = s * p.y + c * p.w;
        return new Vector4(p.x, y, p.z, w);
    }

    Vector3 Project4Dto3D(Vector4 p)
    {
        float k = d4 / (d4 - p.w);
        return new Vector3(k * p.x, k * p.y, k * p.z);
    }

    Vector3 Project3DtoWorld(Vector3 p)
    {
        float k = d3 / (d3 - p.z);
        return transform.position + new Vector3(k * p.x * scale, k * p.y * scale, 0f);
    }

    void OnDrawGizmos()
    {
        if (verts4D == null || edges == null) return;
        float t = Time.time * speed;
        var v3 = new Vector3[verts4D.Length];
        for (int i = 0; i < verts4D.Length; i++)
        {
            var r = Rotate4D(verts4D[i], t);
            var p3 = Project4Dto3D(r);
            v3[i] = Project3DtoWorld(p3);
        }
        for (int i = 0; i < edges.GetLength(0); i++)
        {
            int a = edges[i, 0];
            int b = edges[i, 1];
            var ra = Rotate4D(verts4D[a], t);
            float depth = Mathf.InverseLerp(-1f, 1f, ra.w);
            Color c = Color.Lerp(new Color(0.6f, 0.4f, 0.2f), new Color(0.7f, 0.7f, 0.7f), depth);
            c.a = Mathf.Lerp(0.3f, 1f, depth);
            Gizmos.color = c;
            Gizmos.DrawLine(v3[a], v3[b]);
        }
    }
}
