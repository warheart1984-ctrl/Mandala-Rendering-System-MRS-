// Timeline store — load + cache *.timeline.json by id.

using System.Collections.Generic;
using System.IO;
using SovereignX.CIEMS.Engine.Timeline;
using UnityEngine;

namespace SovereignX.CIEMS.Engine.Timeline
{
    public static class GovernedTimelineStore
    {
        static readonly Dictionary<string, GovernedTimelineDto> Cache =
            new Dictionary<string, GovernedTimelineDto>();

        public static GovernedTimelineDto Load(string timelineId, string basePath)
        {
            if (Cache.TryGetValue(timelineId, out var cached))
                return cached;

            var path = Path.Combine(basePath, timelineId + ".timeline.json");
            if (!File.Exists(path))
            {
                Debug.LogWarning($"[GovernedTimelineStore] missing {path}");
                return null;
            }

            var json = File.ReadAllText(path);
            var dto = JsonUtility.FromJson<GovernedTimelineDto>(json);
            if (dto != null)
                Cache[timelineId] = dto;
            return dto;
        }

        public static void ClearCache() => Cache.Clear();
    }
}
