using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;
using UnityEngine;

/// <summary>
/// Loads 4d-renderer exported mesh JSON (StreamingAssets/surfaces or engine/surfaces/meshes).
/// Includes triangle faces for solid draw.
/// </summary>
public static class SurfaceMeshLoader
{
    public struct LoadedMesh
    {
        public Vector4[] Verts;
        public int[,] Edges;
        public int[] Faces; // flat triples: i,j,k,…
        public int FaceCount => Faces == null ? 0 : Faces.Length / 3;
    }

    public static bool TryLoad(string surfaceId, out Vector4[] verts, out int[,] edges)
    {
        var ok = TryLoadFull(surfaceId, out var mesh);
        verts = mesh.Verts;
        edges = mesh.Edges;
        return ok;
    }

    public static bool TryLoadFull(string surfaceId, out LoadedMesh mesh)
    {
        mesh = default;
        var path = ResolvePath(surfaceId);
        if (path == null) return false;

        var json = File.ReadAllText(path);
        var list = new List<Vector4>();
        foreach (Match m in Regex.Matches(
            json,
            "\\{\\s*\"x\"\\s*:\\s*([-0-9.eE+]+)\\s*,\\s*\"y\"\\s*:\\s*([-0-9.eE+]+)\\s*,\\s*\"z\"\\s*:\\s*([-0-9.eE+]+)\\s*,\\s*\"w\"\\s*:\\s*([-0-9.eE+]+)\\s*\\}"))
        {
            list.Add(new Vector4(
                float.Parse(m.Groups[1].Value, CultureInfo.InvariantCulture),
                float.Parse(m.Groups[2].Value, CultureInfo.InvariantCulture),
                float.Parse(m.Groups[3].Value, CultureInfo.InvariantCulture),
                float.Parse(m.Groups[4].Value, CultureInfo.InvariantCulture)));
        }

        var edgesIdx = json.IndexOf("\"edges\"", StringComparison.Ordinal);
        var facesIdx = json.IndexOf("\"faces\"", StringComparison.Ordinal);
        if (edgesIdx < 0 || list.Count == 0) return false;

        var edgesSectionEnd = facesIdx > edgesIdx ? facesIdx : json.Length;
        var edgesSection = json.Substring(edgesIdx, edgesSectionEnd - edgesIdx);
        var edgeList = new List<(int, int)>();
        foreach (Match m in Regex.Matches(edgesSection, "\\[\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\]"))
            edgeList.Add((int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value)));

        var faceFlat = new List<int>();
        if (facesIdx >= 0)
        {
            var facesSection = json.Substring(facesIdx);
            foreach (Match m in Regex.Matches(
                facesSection,
                "\\[\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\]"))
            {
                faceFlat.Add(int.Parse(m.Groups[1].Value));
                faceFlat.Add(int.Parse(m.Groups[2].Value));
                faceFlat.Add(int.Parse(m.Groups[3].Value));
            }
        }

        if (edgeList.Count == 0) return false;

        var edges = new int[edgeList.Count, 2];
        for (var i = 0; i < edgeList.Count; i++)
        {
            edges[i, 0] = edgeList[i].Item1;
            edges[i, 1] = edgeList[i].Item2;
        }

        mesh = new LoadedMesh
        {
            Verts = list.ToArray(),
            Edges = edges,
            Faces = faceFlat.ToArray(),
        };
        return true;
    }

    static string ResolvePath(string surfaceId)
    {
        var path = Path.Combine(Application.streamingAssetsPath, "surfaces", surfaceId + ".mesh.json");
        if (File.Exists(path)) return path;
        var alt = Path.GetFullPath(Path.Combine(
            Application.dataPath, "../../../engine/surfaces/meshes", surfaceId + ".mesh.json"));
        return File.Exists(alt) ? alt : null;
    }
}
