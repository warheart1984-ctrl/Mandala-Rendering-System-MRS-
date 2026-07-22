// ConstitutionalKnowledgeLayer.cs — loads policies; stores precedents.
// Mirrors engine/governance/ConstitutionalKnowledgeLayer.js.
// Status: host mirror — Unity copies under Assets/Engine/Shared/ may use JsonUtility.
// This file has no Unity dependency (plain C# / docs contract).

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using SovereignX.CIEMS.Engine.Runtime;

namespace SovereignX.CIEMS.Engine.Governance
{
    public sealed class ConstitutionalKnowledgeLayer : IConstitutionalKnowledgeLayer
    {
        readonly List<PolicyRule> _policies = new List<PolicyRule>();
        readonly List<PrecedentRecord> _precedents = new List<PrecedentRecord>();

        public ConstitutionalKnowledgeLayer(IEnumerable<PolicyRule> policies)
        {
            if (policies != null)
                _policies.AddRange(policies);
        }

        /// <summary>
        /// Parse default.policies.json array with a pragmatic field extractor
        /// (sufficient for the fixed policy schema; not a general JSON parser).
        /// </summary>
        public static ConstitutionalKnowledgeLayer LoadFromJsonArray(string jsonArray)
        {
            var list = new List<PolicyRule>();
            if (string.IsNullOrWhiteSpace(jsonArray))
                return new ConstitutionalKnowledgeLayer(list);

            foreach (Match obj in Regex.Matches(jsonArray, @"\{[^{}]+\}"))
            {
                var block = obj.Value;
                var id = Field(block, "id");
                if (string.IsNullOrEmpty(id)) continue;
                list.Add(new PolicyRule
                {
                    Id = id,
                    Scope = Field(block, "scope"),
                    Condition = Field(block, "condition"),
                    Rule = Field(block, "rule"),
                    Severity = Field(block, "severity"),
                    Message = Field(block, "message"),
                });
            }
            return new ConstitutionalKnowledgeLayer(list);
        }

        public static ConstitutionalKnowledgeLayer LoadFromFile(string path)
        {
            return LoadFromJsonArray(File.ReadAllText(path));
        }

        static string Field(string block, string key)
        {
            var m = Regex.Match(block, "\"" + Regex.Escape(key) + "\"\\s*:\\s*\"([^\"]*)\"");
            return m.Success ? m.Groups[1].Value : null;
        }

        public PolicySet GetPoliciesForWorld(string worldId)
        {
            return new PolicySet
            {
                WorldId = worldId ?? "*",
                Policies = new List<PolicyRule>(_policies),
                LoadedAt = DateTime.UtcNow.ToString("o"),
            };
        }

        public IReadOnlyList<PrecedentRecord> GetPrecedents(IntentRecord intent)
        {
            var type = intent?.Type ?? intent?.Kind;
            var result = new List<PrecedentRecord>();
            foreach (var p in _precedents)
            {
                if (string.IsNullOrEmpty(type) ||
                    p.IntentType == type ||
                    p.WorldId == intent?.World)
                    result.Add(p);
            }
            return result;
        }

        public PrecedentRecord RecordPrecedent(IntentRecord intent, Decision decision, float driftScore = 0f)
        {
            var row = new PrecedentRecord
            {
                Id = "precedent-" + (_precedents.Count + 1),
                IntentType = intent?.Type ?? intent?.Kind,
                WorldId = intent?.World,
                IntentId = intent?.Id,
                Decision = decision?.Verdict ?? (object)decision?.Ok,
                DriftScore = driftScore,
                At = DateTime.UtcNow.ToString("o"),
            };
            _precedents.Add(row);
            return row;
        }
    }
}
