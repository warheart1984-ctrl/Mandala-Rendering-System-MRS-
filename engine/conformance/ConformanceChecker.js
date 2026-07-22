/**
 * ConformanceChecker — engine-agnostic evaluator.
 *
 * A RuntimeAdapter supplies capability probes for each domain.
 * The checker runs the canonical profile against the adapter and
 * returns a ConformanceReport (pass/fail per check + summary).
 *
 * This file is the single source of truth for conformance evaluation.
 * Each host (browser, Unity, Unreal) implements a RuntimeAdapter.
 */

/**
 * @typedef {Object} CheckResult
 * @property {string}  id
 * @property {string}  domain
 * @property {boolean} pass
 * @property {string}  [reason]
 */

/**
 * @typedef {Object} ConformanceReport
 * @property {string}   runtime       - e.g. "browser", "unity", "unreal"
 * @property {string}   profileVersion
 * @property {string}   timestamp
 * @property {boolean}  compliant     - true iff every check passes
 * @property {number}   total
 * @property {number}   passed
 * @property {number}   failed
 * @property {CheckResult[]} results
 */

/**
 * A RuntimeAdapter is a plain object keyed by check id.
 * Each key maps to an async function that returns { pass, reason? }.
 *
 * @typedef {Object.<string, () => Promise<{pass: boolean, reason?: string}>>} RuntimeAdapter
 */

/**
 * Evaluate a conformance profile against a runtime adapter.
 *
 * @param {string}         runtimeName
 * @param {object}         profile       - parsed default.conformance-profile.json
 * @param {RuntimeAdapter} adapter
 * @returns {Promise<ConformanceReport>}
 */
export async function evaluateConformance(runtimeName, profile, adapter) {
  const results = [];

  for (const check of profile.checks) {
    const probe = adapter[check.id];
    if (!probe) {
      results.push({
        id: check.id,
        domain: check.domain,
        pass: false,
        reason: "No probe registered for this check.",
      });
      continue;
    }
    try {
      const { pass, reason } = await probe();
      results.push({ id: check.id, domain: check.domain, pass, reason });
    } catch (err) {
      results.push({
        id: check.id,
        domain: check.domain,
        pass: false,
        reason: `Probe threw: ${err.message ?? err}`,
      });
    }
  }

  const passed = results.filter((r) => r.pass).length;
  return {
    runtime: runtimeName,
    profileVersion: profile.version,
    timestamp: new Date().toISOString(),
    compliant: passed === results.length,
    total: results.length,
    passed,
    failed: results.length - passed,
    results,
  };
}

/**
 * Pretty-print a conformance report to a string table.
 */
export function formatReport(report) {
  const lines = [
    `═══ Conformance Report: ${report.runtime} ═══`,
    `Profile v${report.profileVersion}  |  ${report.timestamp}`,
    `Result: ${report.compliant ? "✅ COMPLIANT" : "❌ NON-COMPLIANT"}  (${report.passed}/${report.total})`,
    "",
  ];

  const domains = [...new Set(report.results.map((r) => r.domain))];
  for (const domain of domains) {
    const checks = report.results.filter((r) => r.domain === domain);
    const domainPass = checks.every((c) => c.pass);
    lines.push(`  [${domainPass ? "✓" : "✗"}] ${domain}`);
    for (const c of checks) {
      const mark = c.pass ? "✓" : "✗";
      const suffix = c.reason && !c.pass ? ` — ${c.reason}` : "";
      lines.push(`      ${mark} ${c.id}${suffix}`);
    }
  }

  lines.push("");
  return lines.join("\n");
}
