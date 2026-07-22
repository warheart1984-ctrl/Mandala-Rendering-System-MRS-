using System.Collections.Generic;
using UnityEngine;

/// <summary>Resolves timeline binding names to scene objects. Status: skeleton (Play Mode).</summary>
public class BindingResolver : MonoBehaviour
{
    readonly Dictionary<string, Object> _map = new Dictionary<string, Object>();

    public void Rebuild()
    {
        _map.Clear();
        foreach (var r in FindObjectsOfType<FourDTesseractRenderer>())
        {
            if (!string.IsNullOrEmpty(r.bindingName))
                _map[r.bindingName] = r;
        }
        foreach (var cam in FindObjectsOfType<Camera>())
        {
            var tag = cam.gameObject.name;
            if (tag == "Main Camera" || cam.CompareTag("MainCamera"))
                _map["camera-main"] = cam;
            var binder = cam.GetComponent<GovernedBinding>();
            if (binder != null && !string.IsNullOrEmpty(binder.bindingName))
                _map[binder.bindingName] = cam;
        }
        foreach (var b in FindObjectsOfType<GovernedBinding>())
        {
            if (!string.IsNullOrEmpty(b.bindingName))
                _map[b.bindingName] = b.gameObject;
        }
    }

    public T Resolve<T>(string binding) where T : Object
    {
        if (string.IsNullOrEmpty(binding)) return null;
        if (_map.Count == 0) Rebuild();
        if (_map.TryGetValue(binding, out var obj) && obj is T t)
            return t;
        return null;
    }
}

/// <summary>Optional marker for camera-main / other bindings.</summary>
public class GovernedBinding : MonoBehaviour
{
    public string bindingName;
}
