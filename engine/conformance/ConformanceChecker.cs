// IConformanceChecker — engine-agnostic conformance evaluator.
// Each host (Unity, Unreal) implements IRuntimeAdapter.
// Status: interface — conformance evaluation is portable.

using System;
using System.Collections.Generic;
using System.Linq;

namespace SovereignX.CIEMS.Engine.Conformance
{
    public sealed class CheckResult
    {
        public string Id;
        public string Domain;
        public bool Pass;
        public string Reason;
    }

    public sealed class ConformanceReport
    {
        public string Runtime;
        public string ProfileVersion;
        public string Timestamp;
        public bool Compliant;
        public int Total;
        public int Passed;
        public int Failed;
        public List<CheckResult> Results = new List<CheckResult>();
    }

    /// <summary>
    /// Each host registers probes by check id.
    /// A probe returns (pass, reason).
    /// </summary>
    public interface IRuntimeAdapter
    {
        IReadOnlyDictionary<string, Func<(bool pass, string reason)>> Probes { get; }
    }

    public static class ConformanceChecker
    {
        public static ConformanceReport Evaluate(
            string runtimeName,
            IReadOnlyList<ConformanceCheckDef> checks,
            IRuntimeAdapter adapter)
        {
            var results = new List<CheckResult>();

            foreach (var check in checks)
            {
                if (!adapter.Probes.TryGetValue(check.Id, out var probe))
                {
                    results.Add(new CheckResult
                    {
                        Id = check.Id,
                        Domain = check.Domain,
                        Pass = false,
                        Reason = "No probe registered for this check."
                    });
                    continue;
                }

                try
                {
                    var (pass, reason) = probe();
                    results.Add(new CheckResult
                    {
                        Id = check.Id,
                        Domain = check.Domain,
                        Pass = pass,
                        Reason = reason
                    });
                }
                catch (Exception ex)
                {
                    results.Add(new CheckResult
                    {
                        Id = check.Id,
                        Domain = check.Domain,
                        Pass = false,
                        Reason = $"Probe threw: {ex.Message}"
                    });
                }
            }

            int passed = results.Count(r => r.Pass);
            return new ConformanceReport
            {
                Runtime = runtimeName,
                ProfileVersion = "1.0",
                Timestamp = DateTime.UtcNow.ToString("o"),
                Compliant = passed == results.Count,
                Total = results.Count,
                Passed = passed,
                Failed = results.Count - passed,
                Results = results
            };
        }
    }

    public sealed class ConformanceCheckDef
    {
        public string Id;
        public string Domain;
        public string Description;
        public string Severity;
    }
}
