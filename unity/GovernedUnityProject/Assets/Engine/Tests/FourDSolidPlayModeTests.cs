using System.Collections;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

/// <summary>
/// EditMode + PlayMode smoke for solid 4D mesh path.
/// Test Runner → EditMode / PlayMode. CI without Unity: npm run test:solid-play.
/// </summary>
public class FourDSolidPlayModeTests
{
    [Test]
    public void Tesseract_SolidSmoke_HasTriangles()
    {
        var go = new GameObject("solid-smoke");
        try
        {
            var r = go.AddComponent<FourDTesseractRenderer>();
            r.surfaceId = "tesseract";
            r.renderMode = FourDTesseractRenderer.RenderMode.Solid;
            var tris = r.SmokeSolidFrame();
            Assert.Greater(tris, 0, "Expected solid triangles for tesseract");
            Assert.AreEqual(48, tris, "Tesseract should expose 48 triangles");
        }
        finally
        {
            Object.DestroyImmediate(go);
        }
    }

    [Test]
    public void Clifford_MeshLoads_WithFaces()
    {
        Assert.IsTrue(
            SurfaceMeshLoader.TryLoadFull("clifford-torus", out var mesh),
            "clifford-torus.mesh.json should load");
        Assert.Greater(mesh.Verts.Length, 16);
        Assert.Greater(mesh.FaceCount, 0);
    }

    [UnityTest]
    public IEnumerator PlayMode_LateUpdate_SolidMeshNonEmpty()
    {
        var go = new GameObject("solid-play");
        var r = go.AddComponent<FourDTesseractRenderer>();
        r.surfaceId = "tesseract";
        r.renderMode = FourDTesseractRenderer.RenderMode.Solid;
        yield return null;
        yield return null;
        var mf = go.GetComponent<MeshFilter>();
        Assert.IsNotNull(mf);
        Assert.IsNotNull(mf.sharedMesh);
        Assert.Greater(mf.sharedMesh.triangles.Length, 0);
        Object.Destroy(go);
    }
}
