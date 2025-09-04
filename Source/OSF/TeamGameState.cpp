#include "TeamGameState.h"
#include "Ballsack.h"                                // must include for Cast<ABallsack> and members
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

ATeamGameState::ATeamGameState() {}

ABallsack* ATeamGameState::GetBall() const
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	UClass* UseClass = BallClass ? *BallClass
		: StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/BP_Ball.BP_Ball_C"));

	if (!UseClass) return nullptr;

	return Cast<ABallsack>(UGameplayStatics::GetActorOfClass(W, UseClass));
}

void ATeamGameState::ResetBallToCentre() const
{
	if (ABallsack* Ball = const_cast<ATeamGameState*>(this)->GetBall())
	{
		const FVector Centre(0.f, 0.f, 100.f);
		Ball->SetActorLocation(Centre, false, nullptr, ETeleportType::TeleportPhysics);

		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Ball->GetRootComponent()))
		{
			Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}
}

void ATeamGameState::HandleGoal(int32 /*ScoringTeamID*/, bool /*bRightGoal*/)
{
	// Extend with score keeping / kick-off logic as needed
	ResetBallToCentre();
}

FVector ATeamGameState::GetBallLocationSafe(const UObject* WorldContext)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!W) return FVector::ZeroVector;

	UClass* BallBP = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/BP_Ball.BP_Ball_C"));
	if (!BallBP) return FVector::ZeroVector;

	if (AActor* Found = UGameplayStatics::GetActorOfClass(const_cast<UWorld*>(W), BallBP))
	{
		return Found->GetActorLocation();
	}
	return FVector::ZeroVector;
}
