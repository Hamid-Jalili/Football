#include "Footballer.h"
#include "FootballTeam.h"
#include "Ballsack.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

AFootballer::AFootballer()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFootballer::BeginPlay()
{
	Super::BeginPlay();
	ApplyTeamMaterial();
}

void AFootballer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCloseControl(DeltaSeconds);
}

int32 AFootballer::GetTeamID() const
{
	return TeamRef ? TeamRef->TeamID : -1;
}

void AFootballer::ApplyTeamMaterial()
{
	if (!GetMesh()) return;

	UMaterialInterface* Mat = nullptr;
	if (TeamRef)
	{
		Mat = (TeamRef->TeamID == 0) ? TeamAMaterial : TeamBMaterial;
	}
	if (!Mat) return;

	const int32 Slots = GetMesh()->GetNumMaterials();
	for (int32 i = 0; i < Slots; ++i)
	{
		GetMesh()->SetMaterial(i, Mat);
	}
}

void AFootballer::SetDesiredMovement(const FVector& DesiredMoveWorld)
{
	LastDesiredMove = DesiredMoveWorld;

	FVector Dir = DesiredMoveWorld; Dir.Z = 0.f;
	if (!Dir.IsNearlyZero())
	{
		Dir.Normalize();
		AddMovementInput(Dir, 1.f + DesiredSprintStrength);
		SetActorRotation(FRotator(0.f, Dir.Rotation().Yaw, 0.f));
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		const float BaseSpeed = 420.f, SprintAdd = 280.f;
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

void AFootballer::DisableCloseControl(float Duration)
{
	CloseControlReenableTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, Duration);
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
				Prim->AddImpulse(DirectionWorld.GetSafeNormal() * FMath::Clamp(Power, 0.f, 1.f) * 2200.f, NAME_None, true);
				DisableCloseControl(0.5f);
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
				Prim->AddImpulse(DirectionWorld.GetSafeNormal() * FMath::Clamp(Power, 0.f, 1.f) * 1400.f, NAME_None, true);
				DisableCloseControl(0.3f);
			}
		}
	}
}

void AFootballer::UpdateCloseControl(float DeltaSeconds)
{
	const bool bHumanControlled = Cast<APlayerController>(GetController()) != nullptr;
	if (!bHumanControlled) return;
	if (GetWorld()->GetTimeSeconds() < CloseControlReenableTime) return;

	ABallsack* Ball = FindBall(); if (!Ball) return;
	UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Ball->GetRootComponent()); if (!Prim) return;

	const FVector MyLoc = GetActorLocation(), BallLoc = Ball->GetActorLocation();
	if (FVector::Dist2D(MyLoc, BallLoc) > ControlRadius) return;

	FVector MoveDir = LastDesiredMove; MoveDir.Z = 0.f;
	if (MoveDir.IsNearlyZero()) MoveDir = GetActorForwardVector();
	MoveDir = MoveDir.GetSafeNormal();

	const FVector Target = MyLoc + MoveDir * ControlOffsetForward;

	// XY steering, preserve Z
	const FVector ToTarget = (Target - BallLoc);
	FVector DesiredVelXY = ToTarget * ControlResponsiveness; DesiredVelXY.Z = 0.f;

	FVector NewVel = Prim->GetPhysicsLinearVelocity();
	NewVel.X = DesiredVelXY.X; NewVel.Y = DesiredVelXY.Y;

	const float PlanarSpeed = FVector(NewVel.X, NewVel.Y, 0.f).Size();
	if (PlanarSpeed > MaxBallSpeed)
	{
		const FVector Dir = FVector(NewVel.X, NewVel.Y, 0.f).GetSafeNormal();
		NewVel.X = Dir.X * MaxBallSpeed; NewVel.Y = Dir.Y * MaxBallSpeed;
	}

	Prim->SetPhysicsLinearVelocity(NewVel);
	Prim->AddForce(FVector(0.f, 0.f, -GroundAdhesion), NAME_None, true);
}

/* ------- Temporary target helpers --------- */
void AFootballer::SetTempMoveTarget(const FVector& Location, float DurationSeconds)
{
	TempTargetLocation = Location;
	TempTargetUntilTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, DurationSeconds);
}

bool AFootballer::HasTempMoveTarget() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < TempTargetUntilTime;
}

bool AFootballer::GetTempMoveTarget(FVector& Out) const
{
	if (HasTempMoveTarget())
	{
		Out = TempTargetLocation;
		return true;
	}
	return false;
}
