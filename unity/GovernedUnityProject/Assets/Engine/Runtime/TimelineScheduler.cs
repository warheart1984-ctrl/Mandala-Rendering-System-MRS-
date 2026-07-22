using UnityEngine;

/// <summary>Advances timeline clock. Status: skeleton — driven by TimelineExecutor.</summary>
public class TimelineScheduler : MonoBehaviour
{
    public float TimeSec { get; private set; }
    public float DurationSec { get; set; } = 12f;
    public bool Playing { get; private set; }

    public void Play() => Playing = true;
    public void Pause() => Playing = false;

    public void ResetClock()
    {
        TimeSec = 0f;
        Playing = false;
    }

    public void Seek(float t)
    {
        TimeSec = Mathf.Clamp(t, 0f, DurationSec);
    }

    void Update()
    {
        if (!Playing) return;
        TimeSec += Time.deltaTime;
        if (TimeSec >= DurationSec)
        {
            TimeSec = DurationSec;
            Playing = false;
        }
    }
}
