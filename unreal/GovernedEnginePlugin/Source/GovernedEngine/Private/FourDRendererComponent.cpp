#include "FourDRendererComponent.h"
#include "DrawDebugHelpers.h"
#include "FrameProvenance.h"
#include "CssvRegistry.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"

UFourDRendererComponent::UFourDRendererComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SolidMesh = nullptr;
}

void UFourDRendererComponent::EnsureSolidMeshComponent()
{
	if (!SolidMesh && GetOwner())
	{
		SolidMesh = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("GovernedSolidMesh"));
		SolidMesh->SetupAttachment(GetOwner()->GetRootComponent());
		SolidMesh->RegisterComponent();
	}
	if (SolidMesh && GetOwner() && SolidMesh->GetOwner() == nullptr)
	{
		SolidMesh->SetupAttachment(GetOwner()->GetRootComponent());
	}
}

void UFourDRendererComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureSolidMeshComponent();
	if (SolidMesh && GetOwner() && GetOwner()->GetRootComponent())
	{
		SolidMesh->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}
	ReloadMesh();
}

void UFourDRendererComponent::SetSurface(const FString& InSurfaceId)
{
	SurfaceId = InSurfaceId;
	ReloadMesh();
}

int32 UFourDRendererComponent::SmokeSolidFrame()
{
	ReloadMesh();
	if (FacesFlat.Num() < 3) return 0;
	UpdateSolidMesh(0.5f);
	return FacesFlat.Num() / 3;
}

void UFourDRendererComponent::ReloadMesh()
{
	const FString FileName = SurfaceId + TEXT(".mesh.json");
	const TArray<FString> Candidates = {
		FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("GovernedEngine/Content/Surfaces"), FileName),
		FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Surfaces"), FileName),
		FPaths::Combine(FPaths::ProjectDir(), TEXT("../engine/surfaces/meshes"), FileName),
	};

	bool bLoaded = false;
	for (const FString& Path : Candidates)
	{
		if (FPaths::FileExists(Path) && LoadMeshFromJson(Path))
		{
			LoadedSurfaceId = SurfaceId;
			bLoaded = true;
			UE_LOG(LogTemp, Log, TEXT("[FourD] Loaded %s (%d verts, %d edges, %d tris)"),
				*SurfaceId, Verts4D.Num(), Edges.Num(), FacesFlat.Num() / 3);
			break;
		}
	}
	if (!bLoaded)
	{
		BuildTesseractFallback();
		LoadedSurfaceId = TEXT("tesseract");
		if (SurfaceId != TEXT("tesseract"))
		{
			UE_LOG(LogTemp, Warning, TEXT("[FourD] Mesh '%s' not found; tesseract fallback"), *SurfaceId);
		}
	}

	EnsureSolidMeshComponent();
	if (SolidMesh)
	{
		const bool bSolid = RenderMode == EGovernedRenderMode::Solid || RenderMode == EGovernedRenderMode::Both;
		SolidMesh->SetVisibility(bSolid);
	}
}

bool UFourDRendererComponent::LoadMeshFromJson(const FString& Path)
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *Path)) return false;

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

	const TArray<TSharedPtr<FJsonValue>>* VertsArr = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* EdgesArr = nullptr;
	if (!Root->TryGetArrayField(TEXT("vertices"), VertsArr) ||
		!Root->TryGetArrayField(TEXT("edges"), EdgesArr))
	{
		return false;
	}

	Verts4D.Reset();
	for (const TSharedPtr<FJsonValue>& V : *VertsArr)
	{
		const TSharedPtr<FJsonObject> Obj = V->AsObject();
		if (!Obj.IsValid()) continue;
		Verts4D.Add(FVector4(
			Obj->GetNumberField(TEXT("x")),
			Obj->GetNumberField(TEXT("y")),
			Obj->GetNumberField(TEXT("z")),
			Obj->GetNumberField(TEXT("w"))));
	}

	Edges.Reset();
	for (const TSharedPtr<FJsonValue>& E : *EdgesArr)
	{
		const TArray<TSharedPtr<FJsonValue>>* Pair = nullptr;
		if (!E->TryGetArray(Pair) || Pair->Num() < 2) continue;
		Edges.Add(TPair<int32, int32>(
			static_cast<int32>((*Pair)[0]->AsNumber()),
			static_cast<int32>((*Pair)[1]->AsNumber())));
	}

	FacesFlat.Reset();
	const TArray<TSharedPtr<FJsonValue>>* FacesArr = nullptr;
	if (Root->TryGetArrayField(TEXT("faces"), FacesArr))
	{
		for (const TSharedPtr<FJsonValue>& F : *FacesArr)
		{
			const TArray<TSharedPtr<FJsonValue>>* Tri = nullptr;
			if (!F->TryGetArray(Tri) || Tri->Num() < 3) continue;
			FacesFlat.Add(static_cast<int32>((*Tri)[0]->AsNumber()));
			FacesFlat.Add(static_cast<int32>((*Tri)[1]->AsNumber()));
			FacesFlat.Add(static_cast<int32>((*Tri)[2]->AsNumber()));
		}
	}

	return Verts4D.Num() > 0 && Edges.Num() > 0;
}

void UFourDRendererComponent::BuildTesseractFallback()
{
	Verts4D.Empty();
	for (int x : { -1, 1 })
	for (int y : { -1, 1 })
	for (int z : { -1, 1 })
	for (int w : { -1, 1 })
		Verts4D.Add(FVector4(x, y, z, w));

	Edges.Empty();
	for (int32 i = 0; i < Verts4D.Num(); ++i)
	for (int32 j = i + 1; j < Verts4D.Num(); ++j)
	{
		int Diff = 0;
		if (Verts4D[i].X != Verts4D[j].X) Diff++;
		if (Verts4D[i].Y != Verts4D[j].Y) Diff++;
		if (Verts4D[i].Z != Verts4D[j].Z) Diff++;
		if (Verts4D[i].W != Verts4D[j].W) Diff++;
		if (Diff == 1) Edges.Add(TPair<int32, int32>(i, j));
	}

	// 48 triangles matching 4d-renderer tesseract faces
	FacesFlat.Reset();
	auto IndexOf = [this](float X, float Y, float Z, float W) -> int32
	{
		for (int32 i = 0; i < Verts4D.Num(); ++i)
		{
			if (FMath::IsNearlyEqual(Verts4D[i].X, X) && FMath::IsNearlyEqual(Verts4D[i].Y, Y) &&
				FMath::IsNearlyEqual(Verts4D[i].Z, Z) && FMath::IsNearlyEqual(Verts4D[i].W, W))
				return i;
		}
		return INDEX_NONE;
	};
	const float Vals[2] = { -1.f, 1.f };
	for (int32 D1 = 0; D1 < 4; ++D1)
	for (int32 D2 = D1 + 1; D2 < 4; ++D2)
	{
		TArray<int32> Fixed;
		for (int32 D = 0; D < 4; ++D)
			if (D != D1 && D != D2) Fixed.Add(D);
		for (float F0 : Vals)
		for (float F1 : Vals)
		{
			int32 Corners[4];
			int32 Ci = 0;
			for (float A : Vals)
			for (float B : Vals)
			{
				float C[4] = {};
				C[D1] = A; C[D2] = B; C[Fixed[0]] = F0; C[Fixed[1]] = F1;
				Corners[Ci++] = IndexOf(C[0], C[1], C[2], C[3]);
			}
			FacesFlat.Append({ Corners[0], Corners[1], Corners[2], Corners[1], Corners[3], Corners[2] });
		}
	}
}

void UFourDRendererComponent::UpdateSolidMesh(float T)
{
	EnsureSolidMeshComponent();
	if (!SolidMesh || FacesFlat.Num() < 3 || Verts4D.Num() == 0) return;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	Vertices.SetNum(Verts4D.Num());
	Normals.SetNum(Verts4D.Num());
	UV0.SetNum(Verts4D.Num());
	VertexColors.SetNum(Verts4D.Num());

	for (int32 i = 0; i < Verts4D.Num(); ++i)
	{
		const FVector4 R = Rotate4D(Verts4D[i], T);
		Vertices[i] = Project3DtoLocal(Project4Dto3D(R));
		Normals[i] = FVector::UpVector;
		UV0[i] = FVector2D(0.f, 0.f);
		const float Depth = FMath::GetMappedRangeValueClamped(FVector2D(-1.5f, 1.5f), FVector2D(0.f, 1.f), R.W);
		VertexColors[i] = FLinearColor::LerpUsingHSV(
			FLinearColor(0.12f, 0.2f, 0.31f, 0.85f),
			FLinearColor(0.77f, 0.54f, 0.35f, 0.95f),
			Depth);
	}
	Triangles = FacesFlat;

	SolidMesh->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
	SolidMesh->SetVisibility(
		RenderMode == EGovernedRenderMode::Solid || RenderMode == EGovernedRenderMode::Both);
}

FVector4 UFourDRendererComponent::RotateXW(const FVector4& P, float Theta) const
{
	float C = FMath::Cos(Theta), S = FMath::Sin(Theta);
	return FVector4(C * P.X - S * P.W, P.Y, P.Z, S * P.X + C * P.W);
}

FVector4 UFourDRendererComponent::RotateYZ(const FVector4& P, float Theta) const
{
	float C = FMath::Cos(Theta), S = FMath::Sin(Theta);
	return FVector4(P.X, C * P.Y - S * P.Z, S * P.Y + C * P.Z, P.W);
}

FVector4 UFourDRendererComponent::RotateZW(const FVector4& P, float Theta) const
{
	float C = FMath::Cos(Theta), S = FMath::Sin(Theta);
	return FVector4(P.X, P.Y, C * P.Z - S * P.W, S * P.Z + C * P.W);
}

FVector4 UFourDRendererComponent::RotateYW(const FVector4& P, float Theta) const
{
	float C = FMath::Cos(Theta), S = FMath::Sin(Theta);
	return FVector4(P.X, C * P.Y - S * P.W, P.Z, S * P.Y + C * P.W);
}

FVector4 UFourDRendererComponent::Rotate4D(const FVector4& P, float T) const
{
	FVector4 R = RotateXW(P, T * 0.7f);
	R = RotateYZ(R, T * 1.1f);
	R = RotateZW(R, T * 1.5f);
	R = RotateYW(R, T * 2.0f);
	return R;
}

FVector UFourDRendererComponent::Project4Dto3D(const FVector4& P) const
{
	float K = D4 / (D4 - P.W);
	return FVector(K * P.X, K * P.Y, K * P.Z);
}

FVector UFourDRendererComponent::Project3DtoLocal(const FVector& P) const
{
	float K = D3 / (D3 - P.Z);
	return FVector(K * P.X * Scale, K * P.Y * Scale, 0.f);
}

void UFourDRendererComponent::TickComponent(
	float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Verts4D.Num() == 0) ReloadMesh();
	if (Verts4D.Num() == 0 || !GetWorld() || !GetOwner()) return;

	const float T = GetWorld()->GetTimeSeconds() * Speed;

	if (RenderMode == EGovernedRenderMode::Solid || RenderMode == EGovernedRenderMode::Both)
	{
		UpdateSolidMesh(T);
	}

	if (RenderMode == EGovernedRenderMode::Wireframe || RenderMode == EGovernedRenderMode::Both)
	{
		TArray<FVector> V3;
		V3.SetNum(Verts4D.Num());
		for (int32 i = 0; i < Verts4D.Num(); ++i)
		{
			V3[i] = GetOwner()->GetActorLocation() + Project3DtoLocal(Project4Dto3D(Rotate4D(Verts4D[i], T)));
		}
		for (const auto& E : Edges)
		{
			if (!V3.IsValidIndex(E.Key) || !V3.IsValidIndex(E.Value)) continue;
			FVector4 RA = Rotate4D(Verts4D[E.Key], T);
			float Depth = FMath::GetMappedRangeValueClamped(FVector2D(-1.f, 1.f), FVector2D(0.f, 1.f), RA.W);
			FColor C;
			C.R = static_cast<uint8>(FMath::Lerp(80.f, 200.f, Depth));
			C.B = static_cast<uint8>(FMath::Lerp(200.f, 80.f, Depth));
			C.G = 120;
			C.A = static_cast<uint8>(FMath::Lerp(80.f, 255.f, Depth));
			DrawDebugLine(GetWorld(), V3[E.Key], V3[E.Value], C, false, 0.f, 0, 1.f);
		}
	}

	FFrameProvenance Frame;
	Frame.IntentId = CurrentIntentId;
	Frame.TimelineId = CurrentTimelineId;
	Frame.WorldId = CurrentWorldId;
	Frame.TimeSeconds = GetWorld()->GetTimeSeconds();
	Frame.Parameters.Add(TEXT("speed"), Speed);
	Frame.Parameters.Add(TEXT("d4"), D4);
	Frame.Parameters.Add(TEXT("d3"), D3);
	FProvenanceRecorder::Record(Frame);
	FCssvRegistry::Get().RegisterFrame(Frame);
}
