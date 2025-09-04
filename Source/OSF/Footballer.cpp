// Footballer.cpp
#include "Footballer.h"   // must be first
#include "Ballsack.h"     // ADD THIS
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

AFootballer::AFootballer()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFootballer::BeginPlay()
{
	Super::BeginPlay();

	if (HomeLocation.IsNearlyZero())
	{
		HomeLocation = GetActorLocation();
	}
}

void AFootballer::SetDesiredLocation(const FVector& In)
{
	DesiredLocation = In;
}

void AFootballer::SetDesiredMovement(const FVector& In)
{
	DesiredMove = In;
}

void AFootballer::SetDesiredSprintStrength(float In)
{
	DesiredSprint = FMath::Clamp(In, 0.f, 1.f);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		// 420..700 speed like before
		Move->MaxWalkSpeed = 420.f + 280.f * DesiredSprint;
	}
}

ABallsack* AFootballer::FindBall() const
{
	if (UWorld* W = GetWorld())
	{
		if (UClass* BallBP = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/BP_Ball.BP_Ball_C")))
		{
			return Cast<ABallsack>(UGameplayStatics::GetActorOfClass(W, BallBP));
		}
	}
	return nullptr;
}

void AFootballer::ShootBall(float Power, const FVector& Dir)
{
	if (ABallsack* Ball = FindBall())
	{
		const FVector UseDir = Dir.IsNearlyZero() ? GetActorForwardVector() : Dir.GetSafeNormal();

		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Ball->GetRootComponent()))
		{
			if (Prim->IsSimulatingPhysics())
			{
				Prim->AddImpulse(UseDir * 2000.f * FMath::Clamp(Power, 0.f, 1.f), NAME_None, true);
			}
		}
	}
}

void AFootballer::PassBall(float Power, const FVector& Dir)
{
	ShootBall(Power, Dir);
}

FVector AFootballer::GetSeparationCorrection(const TArray<AFootballer*>& Neighbors, float Radius) const
{
	const FVector SelfLoc = GetActorLocation();
	const float R2 = Radius * Radius;

	FVector Accum = FVector::ZeroVector;
	int32 Count = 0;

	for (AFootballer* N : Neighbors)
	{
		if (!N || N == this) continue;
		const float D2 = FVector::DistSquared(SelfLoc, N->GetActorLocation());
		if (D2 > 1.f && D2 < R2)
		{
			const FVector Away = (SelfLoc - N->GetActorLocation()).GetSafeNormal() * (1.f - (D2 / R2));
			Accum += Away;
			++Count;
		}
	}

	return Count > 0 ? Accum / float(Count) : FVector::ZeroVector;
}
