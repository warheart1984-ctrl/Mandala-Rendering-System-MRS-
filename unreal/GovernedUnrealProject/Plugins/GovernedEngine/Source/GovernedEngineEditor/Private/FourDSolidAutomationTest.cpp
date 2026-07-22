#include "Misc/AutomationTest.h"
#include "FourDRendererComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Editor.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGovernedFourDSolidSmokeTest,
	"GovernedEngine.FourD.SolidSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGovernedFourDSolidSmokeTest::RunTest(const FString& Parameters)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		AddError(TEXT("No editor world for solid smoke"));
		return false;
	}

	FActorSpawnParameters Params;
	Params.Name = TEXT("GovernedSolidSmokeActor");
	AActor* Actor = World->SpawnActor<AActor>(Params);
	if (!Actor)
	{
		AddError(TEXT("Failed to spawn smoke actor"));
		return false;
	}

	UFourDRendererComponent* Comp = NewObject<UFourDRendererComponent>(Actor, TEXT("FourD"));
	Actor->AddInstanceComponent(Comp);
	Comp->RegisterComponent();
	Comp->SurfaceId = TEXT("tesseract");
	Comp->RenderMode = EGovernedRenderMode::Solid;

	const int32 Tris = Comp->SmokeSolidFrame();
	TestTrue(TEXT("tesseract solid triangles > 0"), Tris > 0);
	TestEqual(TEXT("tesseract has 48 triangles"), Tris, 48);

	Actor->Destroy();
	return true;
}

#endif
