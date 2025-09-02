#include "Footballer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

AFootballer::AFootballer()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;
		Move->RotationRate = FRotator(0.f, 720.f, 0.f);
		Move->MaxWalkSpeed = WalkSpeed;
	}
}

void AFootballer::BeginPlay()
{
	Super::BeginPlay();
}

void AFootballer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!DesiredMoveWorld.IsNearlyZero(1e-3f))
	{
		const FVector Move2D = FVector(DesiredMoveWorld.X, DesiredMoveWorld.Y, 0.f).GetClampedToMaxSize(1.f);
		AddMovementInput(Move2D, 1.f);

		if (bFaceMovement)
		{
			const FRotator Target = Move2D.Rotation();
			const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), Target, DeltaSeconds, 10.f);
			SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));
		}
	}
}

/* -------- Desired movement input -------- */

void AFootballer::SetDesiredMovement(const FVector& MovementWorld)
{
	DesiredMoveWorld = MovementWorld.GetClampedToMaxSize(1.f);
}

void AFootballer::SetDesiredMovement2D(const FVector2D& Movement2D)
{
	const FVector As3D(Movement2D.X, Movement2D.Y, 0.f);
	SetDesiredMovement(As3D);
}

void AFootballer::SetDesiredSprintStrength(float InStrength)
{
	DesiredSprint = FMath::Clamp(InStrength, 0.f, 1.f);

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		const float TargetSpeed = FMath::Lerp(WalkSpeed, SprintSpeed, DesiredSprint);
		Move->MaxWalkSpeed = TargetSpeed;
	}
}

/* -------- Ball interactions (stubs) -------- */

void AFootballer::ShootBall(float /*Strength*/, const FVector& /*Direction*/) {}
void AFootballer::PassBall(float /*Strength*/, const FVector& /*Direction*/) {}
void AFootballer::KnockBallOn(float /*ParamA*/, float /*ParamB*/) {}

/* -------- RPCs -------- */

void AFootballer::Server_LosePlayerControl_Implementation()
{
	bHasBall = false;
}

void AFootballer::Server_GainPlayerControl_Implementation(AController* NewController)
{
	if (NewController && NewController->GetPawn() != this)
	{
		NewController->Possess(this);
	}
}

/* -------- Replication -------- */

void AFootballer::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Uncomment when needed:
	// DOREPLIFETIME(AFootballer, Team);
	// DOREPLIFETIME(AFootballer, bHasBall);
	// DOREPLIFETIME(AFootballer, bIsGoalkeeper);
	// DOREPLIFETIME(AFootballer, DesiredMoveWorld);
	// DOREPLIFETIME(AFootballer, DesiredSprint);
}
