#include "CssvRegistry.h"
#include "Dom/JsonObject.h"
#include "Misc/DateTime.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FCssvRegistry& FCssvRegistry::Get()
{
	static FCssvRegistry Instance;
	return Instance;
}

void FCssvRegistry::RegisterArtifact(const FString& Id, const FString& ArtifactType, const TSharedPtr<FJsonObject>& Payload)
{
	TSharedPtr<FJsonObject> Record = MakeShared<FJsonObject>();
	Record->SetStringField(TEXT("type"), TEXT("artifact"));
	Record->SetStringField(TEXT("id"), Id);
	Record->SetStringField(TEXT("artifactType"), ArtifactType);
	Record->SetStringField(TEXT("host"), HostId);
	Record->SetNumberField(TEXT("timestamp"), FDateTime::UtcNow().ToUnixTimestamp());
	if (Payload.IsValid())
	{
		Record->SetObjectField(TEXT("payload"), Payload);
	}
	Artifacts.Add(MakeShared<FJsonValueObject>(Record));
}

void FCssvRegistry::RegisterTransition(
	const FString& Id,
	const FIntentRecord& Intent,
	const FEvidenceBundle& Evidence,
	const FDecision& Decision,
	const FString& Authority,
	double TimeSeconds)
{
	const FString ToState = FString::Printf(TEXT("state-%d"), Transitions.Num() + 1);
	TSharedPtr<FJsonObject> Record = MakeShared<FJsonObject>();
	Record->SetStringField(TEXT("type"), TEXT("transition"));
	Record->SetStringField(TEXT("id"), Id);
	Record->SetStringField(TEXT("from"), StateId);
	Record->SetStringField(TEXT("to"), ToState);

	TSharedPtr<FJsonObject> IntentObj = MakeShared<FJsonObject>();
	IntentObj->SetStringField(TEXT("id"), Intent.Id);
	IntentObj->SetStringField(TEXT("type"), Intent.Type.IsEmpty() ? Intent.Kind : Intent.Type);
	IntentObj->SetStringField(TEXT("actor"), Intent.Actor);
	IntentObj->SetStringField(TEXT("world"), Intent.World);
	IntentObj->SetStringField(TEXT("timeline"), Intent.TimelineId);
	Record->SetObjectField(TEXT("intent"), IntentObj);

	TSharedPtr<FJsonObject> DecisionObj = MakeShared<FJsonObject>();
	DecisionObj->SetBoolField(TEXT("allowed"), Decision.bAllowed);
	DecisionObj->SetStringField(TEXT("verdict"), Decision.Verdict);
	DecisionObj->SetStringField(TEXT("reason"), Decision.Reason);
	Record->SetObjectField(TEXT("decision"), DecisionObj);

	Record->SetStringField(TEXT("authority"), Authority.IsEmpty() ? HostId : Authority);
	Record->SetStringField(TEXT("host"), HostId);
	Record->SetNumberField(TEXT("timestamp"), TimeSeconds > 0.0 ? TimeSeconds : FDateTime::UtcNow().ToUnixTimestamp());
	StateId = ToState;
	Transitions.Add(MakeShared<FJsonValueObject>(Record));
}

void FCssvRegistry::RegisterFrame(const FFrameProvenance& Frame)
{
	TSharedPtr<FJsonObject> Record = MakeShared<FJsonObject>();
	Record->SetStringField(TEXT("type"), TEXT("frame"));
	Record->SetStringField(TEXT("intent"), Frame.IntentId);
	Record->SetStringField(TEXT("timeline"), Frame.TimelineId);
	Record->SetStringField(TEXT("world"), Frame.WorldId);
	Record->SetStringField(TEXT("host"), HostId);
	Record->SetNumberField(TEXT("timestamp"), Frame.TimeSeconds);
	Record->SetNumberField(TEXT("frameIndex"), FrameIndex++);

	TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
	for (const TPair<FString, double>& Pair : Frame.Parameters)
	{
		Params->SetNumberField(Pair.Key, Pair.Value);
	}
	Record->SetObjectField(TEXT("params"), Params);
	Frames.Add(MakeShared<FJsonValueObject>(Record));
}

FString FCssvRegistry::ExportJson() const
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetArrayField(TEXT("artifacts"), Artifacts);
	Root->SetArrayField(TEXT("transitions"), Transitions);
	Root->SetArrayField(TEXT("frames"), Frames);

	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return Out;
}
