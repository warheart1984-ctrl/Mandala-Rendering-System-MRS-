#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralMeshComponent.h"
#include "FourDRendererComponent.generated.h"

UENUM(BlueprintType)
enum class EGovernedRenderMode : uint8
{
	Wireframe UMETA(DisplayName = "Wireframe"),
	Solid UMETA(DisplayName = "Solid"),
	Both UMETA(DisplayName = "Both"),
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOVERNEDENGINE_API UFourDRendererComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFourDRendererComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|4D")
	float D4 = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|4D")
	float D3 = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|4D")
	float Scale = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|4D")
	float Speed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|4D")
	FString SurfaceId = TEXT("tesseract");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|4D")
	EGovernedRenderMode RenderMode = EGovernedRenderMode::Both;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Provenance")
	FString CurrentIntentId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Provenance")
	FString CurrentTimelineId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Governed|Provenance")
	FString CurrentWorldId;

	UPROPERTY(VisibleAnywhere, Category = "Governed|4D")
	TObjectPtr<UProceduralMeshComponent> SolidMesh;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Governed|4D")
	void SetSurface(const FString& InSurfaceId);

	/** PIE/automation smoke: reload + one solid frame; returns triangle count. */
	UFUNCTION(BlueprintCallable, Category = "Governed|4D")
	int32 SmokeSolidFrame();

private:
	TArray<FVector4> Verts4D;
	TArray<TPair<int32, int32>> Edges;
	TArray<int32> FacesFlat;
	FString LoadedSurfaceId;

	void ReloadMesh();
	void BuildTesseractFallback();
	bool LoadMeshFromJson(const FString& Path);
	void UpdateSolidMesh(float T);
	void EnsureSolidMeshComponent();
	FVector4 Rotate4D(const FVector4& P, float T) const;
	FVector4 RotateXW(const FVector4& P, float Theta) const;
	FVector4 RotateYZ(const FVector4& P, float Theta) const;
	FVector4 RotateZW(const FVector4& P, float Theta) const;
	FVector4 RotateYW(const FVector4& P, float Theta) const;
	FVector Project4Dto3D(const FVector4& P) const;
	FVector Project3DtoLocal(const FVector& P) const;
};
