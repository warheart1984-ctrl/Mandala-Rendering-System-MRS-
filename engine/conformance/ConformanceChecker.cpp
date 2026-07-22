#include "ConformanceChecker.h"
#include "Misc/DateTime.h"

FConformanceReport FConformanceChecker::Evaluate(
	const FString& RuntimeName,
	const TArray<FConformanceCheckDef>& Checks,
	const FRuntimeAdapter& Adapter)
{
	FConformanceReport Report;
	Report.Runtime = RuntimeName;
	Report.ProfileVersion = TEXT("1.0");
	Report.Timestamp = FDateTime::UtcNow().ToIso8601();

	for (const auto& Check : Checks)
	{
		const FConformanceProbe* Probe = Adapter.Probes.Find(Check.Id);
		if (!Probe)
		{
			Report.Results.Add({Check.Id, Check.Domain, false, TEXT("No probe registered for this check.")});
			continue;
		}

		auto [bPass, Reason] = (*Probe)();
		Report.Results.Add({Check.Id, Check.Domain, bPass, Reason});
	}

	Report.Total = Report.Results.Num();
	Report.Passed = 0;
	for (const auto& R : Report.Results)
	{
		if (R.bPass) ++Report.Passed;
	}
	Report.Failed = Report.Total - Report.Passed;
	Report.bCompliant = Report.Passed == Report.Total;
	return Report;
}
