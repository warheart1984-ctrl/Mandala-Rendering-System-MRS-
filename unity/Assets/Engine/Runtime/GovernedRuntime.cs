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

/// <summary>Skeleton Unity tick loop. Status: skeleton — wire IntentService before use.</summary>
public class GovernedRuntime : MonoBehaviour
{
    void Update()
    {
        // var intents = IntentService.Collect();
        // foreach (var intent in intents) {
        //     var plan = ExecutionOrchestrator.Plan(intent);
        //     ExecutionOrchestrator.Execute(plan);
        // }
    }
}
