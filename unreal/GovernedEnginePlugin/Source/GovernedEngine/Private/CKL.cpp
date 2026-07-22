#include "CKL.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Guid.h"

void FCKL::LoadPoliciesFromJson(const FString& JsonText)
{
	Policies.Reset();
	TArray<TSharedPtr<FJsonValue>> Arr;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Arr))
	{
		UE_LOG(LogTemp, Warning, TEXT("CKL: failed to parse policies JSON"));
		return;
	}
	for (const TSharedPtr<FJsonValue>& V : Arr)
	{
		const TSharedPtr<FJsonObject> Obj = V->AsObject();
		if (!Obj.IsValid()) continue;
		FPolicy P;
		P.Id = Obj->GetStringField(TEXT("id"));
		P.Scope = Obj->GetStringField(TEXT("scope"));
		P.Condition = Obj->GetStringField(TEXT("condition"));
		P.Rule = Obj->GetStringField(TEXT("rule"));
		P.Severity = Obj->GetStringField(TEXT("severity"));
		P.Message = Obj->GetStringField(TEXT("message"));
		if (Obj->HasField(TEXT("param")))
		{
			P.Param = Obj->GetStringField(TEXT("param"));
		}
		if (Obj->HasField(TEXT("modifier")))
		{
			P.Modifier = Obj->GetStringField(TEXT("modifier"));
		}
		const TArray<TSharedPtr<FJsonValue>>* RequireArr = nullptr;
		if (Obj->TryGetArrayField(TEXT("require"), RequireArr))
		{
			for (const TSharedPtr<FJsonValue>& Req : *RequireArr)
			{
				P.Require.Add(Req->AsString());
			}
		}
		Policies.Add(P);
	}
}

FPolicySet FCKL::GetPoliciesForWorld(const FWorldId& World) const
{
	FPolicySet Set;
	Set.WorldId = World.Value.IsEmpty() ? TEXT("*") : World.Value;
	Set.Policies = Policies;
	return Set;
}

TArray<FDecisionPrecedent> FCKL::GetPrecedents(const FIntentRecord& Intent) const
{
	TArray<FDecisionPrecedent> Out;
	for (const FDecisionPrecedent& P : Precedents)
	{
		if (P.IntentType.IsEmpty() || P.IntentType == Intent.Type || P.WorldId == Intent.World)
		{
			Out.Add(P);
		}
	}
	return Out;
}

void FCKL::RecordPrecedent(const FIntentRecord& Intent, const FDecision& Decision)
{
	FDecisionPrecedent P;
	P.Id = FString::Printf(TEXT("precedent-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
	P.IntentType = Intent.Type;
	P.WorldId = Intent.World;
	P.bAllowed = Decision.bAllowed;
	Precedents.Add(P);
}
