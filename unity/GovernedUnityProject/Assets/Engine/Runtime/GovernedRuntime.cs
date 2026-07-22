using System;
using System.Collections.Generic;
using UnityEngine;

/// <summary>Skeleton governed entity. Status: skeleton.</summary>
public class GovernedObject
{
    public Guid Id;
    public List<GovernedComponent> Components = new List<GovernedComponent>();
    public string Lineage;
}

public abstract class GovernedComponent
{
    public GovernedObject Owner;
    public abstract void OnIntent(object intent);
    public abstract void OnUpdate(float dt);
}

/// <summary>Optional tick host. Prefer IslIntentBootstrap + ExecutionOrchestratorHost.</summary>
public class GovernedRuntime : MonoBehaviour
{
    void Update()
    {
        // Reserved for continuous intent collection — not wired in Phase-1 skeleton.
    }
}
