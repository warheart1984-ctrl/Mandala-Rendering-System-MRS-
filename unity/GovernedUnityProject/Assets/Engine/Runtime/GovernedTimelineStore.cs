using System;
using System.Collections.Generic;
using UnityEngine;

/// <summary>In-memory timeline JSON DTOs. Status: skeleton loader.</summary>
[Serializable]
public class TimelineDto
{
    public string id;
    public string name;
    public float durationSec;
    public TimelineTrackDto[] tracks;
}

[Serializable]
public class TimelineTrackDto
{
    public string id;
    public string binding;
    public TimelineClipDto[] clips;
}

[Serializable]
public class TimelineClipDto
{
    public string id;
    public float startSec;
    public float durationSec;
    public string action;
    public TimelinePayloadDto payload;
}

[Serializable]
public class TimelinePayloadDto
{
    public string param;
    public float from;
    public float to;
    public string[] planes;
}

/// <summary>Loads timeline JSON from StreamingAssets / TextAsset. Status: skeleton.</summary>
public class GovernedTimelineStore : MonoBehaviour
{
    public TextAsset timelineAsset;
    public string streamingRelativePath = "timelines/opening_4d_reveal.timeline.json";

    TimelineDto _cached;

    public TimelineDto Load()
    {
        if (_cached != null) return _cached;
        string json = null;
        if (timelineAsset != null)
            json = timelineAsset.text;
        else
        {
            var path = System.IO.Path.Combine(Application.streamingAssetsPath, streamingRelativePath);
            if (System.IO.File.Exists(path))
                json = System.IO.File.ReadAllText(path);
        }
        if (string.IsNullOrEmpty(json))
            throw new InvalidOperationException("Timeline JSON not found");
        _cached = JsonUtility.FromJson<TimelineDto>(json);
        return _cached;
    }

    public void ClearCache() => _cached = null;
}
