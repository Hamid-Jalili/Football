#include "Footballer.h"
#include "FootballTeam.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Ballsack.h"
#include "Components/PrimitiveComponent.h"

AFootballer::AFootballer()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFootballer::BeginPlay()
{
	Super::BeginPlay();

	if (TeamRef)
	{
		if (TeamRef->TeamID == 0 && TeamAMaterial)
		{
			GetMesh()->SetMaterial(0, TeamAMaterial);
		}
		else if (TeamRef->TeamID == 1 && TeamBMaterial)
		{
			GetMesh()->SetMaterial(0, TeamBMaterial);
		}
	}
}

int32 AFootballer::GetTeamID() const
{
	return TeamRef ? TeamRef->TeamID : -1;
}

void AFootballer::SetDesiredMovement(const FVector& DesiredMoveWorld)
{
	FVector Dir = DesiredMoveWorld;
	Dir.Z = 0.f;
	if (!Dir.IsNearlyZero())
	{
		Dir.Normalize();
		AddMovementInput(Dir, 1.f + DesiredSprintStrength);
	}

	// Face movement direction
	if (!Dir.IsNearlyZero())
	{
		const FRotator TargetYaw = Dir.Rotation();
		SetActorRotation(FRotator(0.f, TargetYaw.Yaw, 0.f));
	}

	// Adjust speed by sprint
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		const float BaseSpeed = 420.f;
		const float SprintAdd = 280.f;
		Move->MaxWalkSpeed = BaseSpeed + SprintAdd * FMath::Clamp(DesiredSprintStrength, 0.f, 1.f);
	}
}

void AFootballer::SetDesiredSprintStrength(float InStrength)
{
	DesiredSprintStrength = FMath::Clamp(InStrength, 0.f, 1.f);
}

ABallsack* AFootballer::FindBall() const
{
	return Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass()));
}

void AFootballer::ShootBall(float Power, const FVector& DirectionWorld)
{
	if (ABallsack* Ball = FindBall())
	{
		const FVector ToBall = Ball->GetActorLocation() - GetActorLocation();
		if (ToBall.SizeSquared2D() < FMath::Square(250.f))
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Ball->GetRootComponent()))
			{
				const FVector Impulse = DirectionWorld.GetSafeNormal() * FMath::Clamp(Power, 0.f, 1.f) * 2200.f;
				Prim->AddImpulse(Impulse, NAME_None, true);
			}
		}
	}
}

void AFootballer::PassBall(float Power, const FVector& DirectionWorld)
{
	if (ABallsack* Ball = FindBall())
	{
		const FVector ToBall = Ball->GetActorLocation() - GetActorLocation();
		if (ToBall.SizeSquared2D() < FMath::Square(250.f))
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Ball->GetRootComponent()))
			{
				const FVector Impulse = DirectionWorld.GetSafeNormal() * FMath::Clamp(Power, 0.f, 1.f) * 1400.f;
				Prim->AddImpulse(Impulse, NAME_None, true);
			}
		}
	}
}
