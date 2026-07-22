// Shared movie-export contract (C#). Hosts write PNG sequences + manifest JSON.
// Status: contract — Unity/Unreal implement capture; browser uses WebM via MediaRecorder.

using System.Collections.Generic;

namespace SovereignX.CIEMS.Engine.Runtime
{
    public sealed class MovieExportManifest
    {
        public string Format = "png-sequence";
        public string HostId;
        public string IntentId;
        public string WorldId;
        public string TimelineId;
        public string DecisionId;
        public string OutputDir;
        public string Basename;
        public double Seconds;
        public int Fps;
        public int FrameCount;
        public int Width;
        public int Height;
        public List<string> FrameFiles = new List<string>();
        public string ManifestPath;
        public string ProvenancePath;
        public string MuxHint;
        public string ContainerPath;
        public string CreatedAt;
    }
}
