// SovereignX.CIEMS.Engine.CSSV — canonical constitutional ledger (C#).
// Status: shared contract; browser JS CssvRegistry is authoritative for web host.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace SovereignX.CIEMS.Engine.CSSV
{
    public interface ICssvHost
    {
        string HostId { get; }
        string HostVersion { get; }
    }

    public interface ICssvRegistry
    {
        void RegisterArtifact(ArtifactRecord artifact);
        void RegisterTransition(TransitionRecord transition);
        void RegisterFrame(Runtime.FrameProvenance frame);
    }

    public sealed class ArtifactRecord
    {
        public string Id;
        public string Type;
        public string HostId;
        public object Payload;
    }

    public sealed class TransitionRecord
    {
        public string Id;
        public string FromStateId;
        public string ToStateId;
        public Runtime.IntentRecord Intent;
        public string Authority;
        public Runtime.EvidenceBundle Evidence;
        public Runtime.Decision Decision;
        public string HostId;
        public double TimeSeconds;
    }

    public sealed class CssvLedger
    {
        public List<object> Artifacts = new List<object>();
        public List<object> Transitions = new List<object>();
        public List<object> Frames = new List<object>();

        public static CssvLedger Load(string root)
        {
            var ledger = new CssvLedger();
            var artifactsPath = Path.Combine(root, "artifacts.json");
            if (File.Exists(artifactsPath))
            {
                var json = File.ReadAllText(artifactsPath);
                // Host projects may use Newtonsoft; minimal load defers to host.
            }
            ledger.Transitions.AddRange(LoadNdjson(Path.Combine(root, "transitions.ndjson")));
            ledger.Frames.AddRange(LoadNdjson(Path.Combine(root, "frames.ndjson")));
            return ledger;
        }

        static IEnumerable<object> LoadNdjson(string path)
        {
            if (!File.Exists(path)) yield break;
            foreach (var line in File.ReadLines(path))
            {
                if (string.IsNullOrWhiteSpace(line)) continue;
                yield return line;
            }
        }
    }

    public sealed class CqlQuery
    {
        public string Select;
        public string From;
        public CqlExpr Where;
        public CqlOrderBy OrderBy;
        public int? Limit;
    }

    public abstract class CqlExpr { }

    public sealed class CqlBinaryExpr : CqlExpr
    {
        public string Field;
        public string Op;
        public string Value;
    }

    public sealed class CqlLogicalExpr : CqlExpr
    {
        public string Op;
        public CqlExpr Left;
        public CqlExpr Right;
    }

    public sealed class CqlOrderBy
    {
        public string Field;
        public string Direction = "ASC";
    }

    /// <summary>Host mirror of Node cqlInterpreter — evaluates over in-memory ledger rows.</summary>
    public sealed class CqlInterpreter
    {
        readonly CssvLedger _ledger;

        public CqlInterpreter(CssvLedger ledger) => _ledger = ledger ?? new CssvLedger();

        public IEnumerable<string> Execute(CqlQuery query)
        {
            IEnumerable<string> source = query.From switch
            {
                "artifact" => _ledger.Artifacts.Cast<string>(),
                "transition" => _ledger.Transitions.Cast<string>(),
                "frame" => _ledger.Frames.Cast<string>(),
                _ => throw new InvalidOperationException($"Unknown FROM: {query.From}")
            };
            return source.Take(query.Limit ?? int.MaxValue);
        }
    }
}
