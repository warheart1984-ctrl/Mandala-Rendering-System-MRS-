#include "GovernedWorldLoader.h"
#include "FourDRendererComponent.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"

namespace
{
	float ConfigFloat(const TMap<FString, FString>& Config, const TCHAR* Key, float DefaultValue)
	{
		if (const FString* V = Config.Find(Key))
		{
			return FCString::Atof(**V);
		}
		return DefaultValue;
	}

	void ParseComponentConfig(const TSharedPtr<FJsonObject>& Obj, FGovernedComponentDto& Out)
	{
		if (!Obj.IsValid()) return;
		Obj->TryGetStringField(TEXT("type"), Out.Type);
		const TSharedPtr<FJsonObject>* ConfigObj = nullptr;
		if (Obj->TryGetObjectField(TEXT("config"), ConfigObj) && ConfigObj && ConfigObj->IsValid())
		{
			for (const auto& Pair : (*ConfigObj)->Values)
			{
				if (Pair.Value->Type == EJson::Number)
				{
					Out.Config.Add(Pair.Key, FString::SanitizeFloat(Pair.Value->AsNumber()));
				}
				else if (Pair.Value->Type == EJson::String)
				{
					Out.Config.Add(Pair.Key, Pair.Value->AsString());
				}
				else if (Pair.Value->Type == EJson::Array)
				{
					const TArray<TSharedPtr<FJsonValue>>& Arr = Pair.Value->AsArray();
					for (int32 i = 0; i < Arr.Num(); ++i)
					{
						if (Arr[i]->Type == EJson::Number)
						{
							Out.Config.Add(
								FString::Printf(TEXT("%s.%d"), *Pair.Key, i),
								FString::SanitizeFloat(Arr[i]->AsNumber()));
						}
					}
				}
			}
		}
	}
}

FGovernedWorldDto FGovernedWorldDto::FromJson(const FString& Json)
{
	FGovernedWorldDto World;
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return World;
	}

	Root->TryGetStringField(TEXT("id"), World.Id);
	Root->TryGetStringField(TEXT("name"), World.Name);
	Root->TryGetStringField(TEXT("constitution"), World.Constitution);

	const TArray<TSharedPtr<FJsonValue>>* Entities = nullptr;
	if (Root->TryGetArrayField(TEXT("entities"), Entities))
	{
		for (const TSharedPtr<FJsonValue>& Val : *Entities)
		{
			const TSharedPtr<FJsonObject> Obj = Val->AsObject();
			if (!Obj.IsValid()) continue;
			FGovernedEntityDto E;
			Obj->TryGetStringField(TEXT("id"), E.Id);
			Obj->TryGetStringField(TEXT("name"), E.Name);
			Obj->TryGetStringField(TEXT("parent"), E.Parent);
			const TArray<TSharedPtr<FJsonValue>>* Comps = nullptr;
			if (Obj->TryGetArrayField(TEXT("components"), Comps))
			{
				for (const TSharedPtr<FJsonValue>& CVal : *Comps)
				{
					FGovernedComponentDto C;
					ParseComponentConfig(CVal->AsObject(), C);
					E.Components.Add(C);
				}
			}
			World.Entities.Add(E);
		}
	}
	return World;
}

FGovernedWorldDto FGovernedWorldLoader::LoadFromFile(const FString& Path)
{
	FString Json;
	FFileHelper::LoadFileToString(Json, *Path);
	return FGovernedWorldDto::FromJson(Json);
}

void FGovernedWorldLoader::InstantiateWorld(const FGovernedWorldDto& WorldDto, UWorld* World)
{
	if (!World) return;
	TMap<FString, AActor*> IdToActor;

	for (const FGovernedEntityDto& Entity : WorldDto.Entities)
	{
		FActorSpawnParameters Params;
		Params.Name = FName(*Entity.Name);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		if (Actor)
		{
			IdToActor.Add(Entity.Id, Actor);
			Actor->Tags.Add(*Entity.Id);
		}
	}

	for (const FGovernedEntityDto& Entity : WorldDto.Entities)
	{
		if (Entity.Parent.IsEmpty()) continue;
		AActor** Parent = IdToActor.Find(Entity.Parent);
		AActor** Child = IdToActor.Find(Entity.Id);
		if (Parent && Child && *Parent && *Child)
		{
			(*Child)->AttachToActor(*Parent, FAttachmentTransformRules::KeepWorldTransform);
		}
	}

	for (const FGovernedEntityDto& Entity : WorldDto.Entities)
	{
		AActor** Found = IdToActor.Find(Entity.Id);
		if (!Found || !*Found) continue;
		AActor* Actor = *Found;

		for (const FGovernedComponentDto& Comp : Entity.Components)
		{
			if (Comp.Type == TEXT("Transform"))
			{
				Actor->SetActorLocation(FVector(
					ConfigFloat(Comp.Config, TEXT("position.0"), 0.f),
					ConfigFloat(Comp.Config, TEXT("position.1"), 0.f),
					ConfigFloat(Comp.Config, TEXT("position.2"), 0.f)));
			}
			else if (Comp.Type == TEXT("Camera"))
			{
				UCameraComponent* Cam = NewObject<UCameraComponent>(Actor, TEXT("GovernedCamera"));
				Cam->RegisterComponent();
				if (Actor->GetRootComponent())
				{
					Cam->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				}
				Cam->SetFieldOfView(ConfigFloat(Comp.Config, TEXT("fov"), 60.f));
			}
			else if (Comp.Type == TEXT("FourDRenderer"))
			{
				UFourDRendererComponent* Renderer = NewObject<UFourDRendererComponent>(Actor);
				Renderer->RegisterComponent();
				Renderer->D4 = ConfigFloat(Comp.Config, TEXT("d4"), 4.f);
				Renderer->D3 = ConfigFloat(Comp.Config, TEXT("d3"), 4.f);
				Renderer->Speed = ConfigFloat(Comp.Config, TEXT("speed"), 1.f);
				Renderer->Scale = ConfigFloat(Comp.Config, TEXT("scale"), 100.f);
			}
		}
	}
}
