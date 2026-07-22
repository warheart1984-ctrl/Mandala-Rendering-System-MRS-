// EvidenceBundle.cs — host mirror of evidence passed to CKL/GK.
// Status: interface / host mirror.

using System.Collections.Generic;

namespace SovereignX.CIEMS.Engine.Runtime
{
    public sealed class EvidenceBundle
    {
        public string Id;
        public string Kind;
        public string Timestamp;
        public string WorldId;
        public string TimelineId;
        public Dictionary<string, object> Fields = new Dictionary<string, object>();
    }
}
