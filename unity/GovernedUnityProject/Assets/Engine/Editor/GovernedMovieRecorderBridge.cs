#if UNITY_EDITOR && GOVERNED_UNITY_RECORDER
using System;
using System.IO;
using UnityEditor;
using UnityEditor.Recorder;
using UnityEditor.Recorder.Encoder;
using UnityEditor.Recorder.Input;
using UnityEngine;

/// <summary>
/// Unity Recorder bridge — H.264 MP4 container encode in Editor Play Mode.
/// </summary>
[InitializeOnLoad]
public static class GovernedMovieRecorderBridge
{
    static GovernedMovieRecorderBridge()
    {
        GovernedMovieCapture.TryContainerEncode = TryStart;
    }

    static GovernedMovieCapture.ContainerEncodeResult TryStart(
        string outputDir,
        string sessionName,
        float seconds,
        int fps,
        int width,
        int height)
    {
        var result = new GovernedMovieCapture.ContainerEncodeResult();
        if (!Application.isPlaying)
        {
            Debug.LogWarning("[MovieRecorder] Container encode requires Play Mode.");
            return result;
        }

        Directory.CreateDirectory(outputDir);
        var outFileNoExt = Path.Combine(outputDir, sessionName).Replace('\\', '/');

        var controllerSettings = ScriptableObject.CreateInstance<RecorderControllerSettings>();
        var movieSettings = ScriptableObject.CreateInstance<MovieRecorderSettings>();
        movieSettings.name = "GovernedMovie";
        movieSettings.Enabled = true;
        movieSettings.OutputFile = outFileNoExt;
        movieSettings.EncoderSettings = new CoreEncoderSettings
        {
            Codec = CoreEncoderSettings.OutputCodec.MP4,
        };
        movieSettings.ImageInputSettings = new GameViewInputSettings
        {
            OutputWidth = Math.Max(16, width),
            OutputHeight = Math.Max(16, height),
        };

        controllerSettings.AddRecorderSettings(movieSettings);
        controllerSettings.SetRecordModeToTimeInterval(0f, Mathf.Max(0.1f, seconds));
        controllerSettings.FrameRate = fps;
        controllerSettings.CapFrameRate = true;

        var controller = new RecorderController(controllerSettings);
        controller.PrepareRecording();
        if (!controller.StartRecording())
        {
            Debug.LogError("[MovieRecorder] StartRecording failed.");
            return result;
        }

        result.Ok = true;
        result.Format = "mp4";
        result.ContainerPath = outFileNoExt + ".mp4";
        Debug.Log($"[MovieRecorder] MP4 → {result.ContainerPath} ({seconds}s @ {fps}fps)");

        var endAt = EditorApplication.timeSinceStartup + seconds + 1.5;
        void Tick()
        {
            if (controller.IsRecording() && EditorApplication.timeSinceStartup < endAt)
            {
                EditorApplication.delayCall += Tick;
                return;
            }
            if (controller.IsRecording())
                controller.StopRecording();
        }
        EditorApplication.delayCall += Tick;
        return result;
    }
}
#endif
