#pragma once

#ifndef GOVERNEDENGINE_API
#define GOVERNEDENGINE_API
#endif

#include "CoreMinimal.h"

/**
 * SovereignX::CIEMS::Engine::CSSV — constitutional ledger (C++ mirror).
 * Status: interface — Node JS CssvRegistry is authoritative for browser.
 */

namespace SovereignX::CIEMS::Engine::CSSV
{
	struct FArtifactRecord
	{
		FString Id;
		FString Type;
		FString HostId;
		TSharedPtr<void> Payload;
	};

	struct FTransitionRecord
	{
		FString Id;
		FString FromStateId;
		FString ToStateId;
		FString Authority;
		FString HostId;
		double TimeSeconds = 0.0;
	};

	class ICssvHost
	{
	public:
		virtual FString GetHostId() const = 0;
		virtual FString GetHostVersion() const = 0;
		virtual ~ICssvHost() = default;
	};

	class ICssvRegistry
	{
	public:
		virtual void RegisterArtifact(const FArtifactRecord& Artifact) = 0;
		virtual void RegisterTransition(const FTransitionRecord& Transition) = 0;
		virtual void RegisterFrame(const struct FFrameProvenance& Frame) = 0;
		virtual ~ICssvRegistry() = default;
	};

	struct FCssvLedger
	{
		TArray<FString> Artifacts;
		TArray<FString> Transitions;
		TArray<FString> Frames;
	};

	struct FCqlQuery
	{
		FString Select;
		FString From;
		int32 Limit = 0;
	};

	class FCqlInterpreter
	{
	public:
		explicit FCqlInterpreter(const FCssvLedger& InLedger) : Ledger(InLedger) {}

		TArray<FString> Execute(const FCqlQuery& Query) const
		{
			if (Query.From == TEXT("frame")) return Ledger.Frames;
			if (Query.From == TEXT("transition")) return Ledger.Transitions;
			return Ledger.Artifacts;
		}

	private:
		const FCssvLedger& Ledger;
	};
}
