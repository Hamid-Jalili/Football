#include "Footballer.h"
#include "OSF.h"
#include "FootballerController.h"
#include "FootballerAIController.h"
#include "Goal.h"

#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// ----------------------------------------------------------------------------
// Sample attribute presets
// ----------------------------------------------------------------------------
static AFootballer::FootballAttributeInfo FA_Sample_Walcott()
{
	AFootballer::FootballAttributeInfo A; A.SprintSpeed = 95; return A;
}

// ----------------------------------------------------------------------------
// Construction
// ----------------------------------------------------------------------------
AFootballer::AFootballer()
{
	PrimaryActorTick.bCanEverTick = true;
	FootballAttributes = FA_Sample_Walcott();

	DesiredMovement = FVector::ZeroVector;
	DesiredSprintStrength = 0.f;

	GoingForPossession = false;
	WaitingForPass = false;
	ThinksHasPossession = false;
	JustKickedBall = false;
	LastKickTime = 0.f;

	FootballerController = nullptr;
	AIController = nullptr;
	Team = nullptr;
	Ball = nullptr;

	// --- Visibility belt & suspenders: never spawn hidden --------------------
	SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->SetVisibility(true, true);
		M->SetHiddenInGame(false, true);
		M->SetOwnerNoSee(false);
		M->SetOnlyOwnerSee(false);
		M->SetRenderInMainPass(true);
		M->SetCastHiddenShadow(false);
		M->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

// ----------------------------------------------------------------------------
// Replication
// ----------------------------------------------------------------------------
void AFootballer::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFootballer, DesiredSprintStrength);
	DOREPLIFETIME(AFootballer, DesiredMovement);
	DOREPLIFETIME(AFootballer, Ball);
	DOREPLIFETIME(AFootballer, GoingForPossession);
	DOREPLIFETIME(AFootballer, ThinksHasPossession);
	DOREPLIFETIME(AFootballer, WaitingForPass);
	DOREPLIFETIME(AFootballer, JustKickedBall);
	DOREPLIFETIME(AFootballer, LastKickTime);
	DOREPLIFETIME(AFootballer, FootballerController);
	DOREPLIFETIME(AFootballer, DisplayName);
	DOREPLIFETIME(AFootballer, Team);
	DOREPLIFETIME(AFootballer, AIController);
	DOREPLIFETIME(AFootballer, ControlledByPlayer);
	DOREPLIFETIME(AFootballer, CachedTeammates);
	DOREPLIFETIME(AFootballer, DoneInitialSetup);
}

// ----------------------------------------------------------------------------
// AActor
// ----------------------------------------------------------------------------
void AFootballer::BeginPlay()
{
	Super::BeginPlay();

	// World ball reference
	Ball = ABallsack::GetWorldBall(GetWorld());
	verify(Ball != nullptr);

	// Indicators
	if (TargetingIndicator) TargetingIndicator->SetVisibility(false);
	if (!ControlledByPlayer && PlayerControlIndicator) PlayerControlIndicator->SetVisibility(false);

	// Movement tuning
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = FootballAttributes.SprintSpeed * 10.f; // keep project’s original relation
		Move->BrakingFrictionFactor = 1.0f;
		Move->bUseSeparateBrakingFriction = true;
		Move->BrakingFriction = 3.f;
	}

	// Never hidden
	SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->SetHiddenInGame(false, true);
		M->SetOwnerNoSee(false);
		M->SetOnlyOwnerSee(false);
	}

	// small grace so we don't treat spawn-touch as a legal touch
	LastKickTime = GetWorld()->GetTimeSeconds();
}

bool AFootballer::CanTouchNow(const UWorld* World, float LastTouch, float CooldownSeconds)
{
	return (World && (World->GetTimeSeconds() - LastTouch) > CooldownSeconds);
}

FVector AFootballer::MakeValidDesired(FVector Movement, float Sprint01)
{
	FVector M = Movement;
	if (!M.Normalize()) return FVector::ZeroVector;
	Sprint01 = FMath::Clamp(Sprint01, 0.f, 1.f);
	return M * (0.5f + 0.5f * Sprint01);
}

// ----------------------------------------------------------------------------
// Tick
// ----------------------------------------------------------------------------
void AFootballer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Reset "just kicked" once we’re no longer in touch range
	if (JustKickedBall && !CanKickBall())
	{
		JustKickedBall = false;
	}

	if (PlayerControlIndicator)
	{
		PlayerControlIndicator->SetVisibility(ControlledByPlayer);
	}

	if (ControlledByPlayer)
	{
		const bool bInKickRange = CanKickBall();
		const bool bTouchGate = CanTouchNow(GetWorld(), LastKickTime, 0.30f);

		if (bInKickRange && bTouchGate)
		{
			GoingForPossession = true;
			ThinksHasPossession = true;

			if (PendingAction.Type != PendingActionType::FootballerActionNone)
			{
				switch (PendingAction.Type)
				{
				case PendingActionType::FootballerActionShot:
					ShootBall(PendingAction.Power, PendingAction.Direction);
					JustKickedBall = true;
					LastKickTime = GetWorld()->GetTimeSeconds();
					break;
				case PendingActionType::FootballerActionPass:
					PassBall(PendingAction.Power, PendingAction.Direction);
					JustKickedBall = true;
					LastKickTime = GetWorld()->GetTimeSeconds();
					break;
				default: break;
				}
				ClearPendingAction();
			}
			else
			{
				// Only knock if the user is actually trying to move roughly toward the ball
				const FVector MoveDir = DesiredMovement.GetSafeNormal2D();
				FVector ToBall = (Ball ? (Ball->GetActorLocation() - GetActorLocation()) : FVector::ZeroVector);
				ToBall.Z = 0.f;
				const FVector ToBallDir = ToBall.GetSafeNormal();

				const bool bHasInput = DesiredMovement.SizeSquared2D() > 0.05f;
				const bool bTowardBall = FVector::DotProduct(MoveDir, ToBallDir) > 0.25f;

				if (bHasInput && bTowardBall)
				{
					KnockBallOn(DeltaTime, 5.f + 5.f * DesiredSprintStrength);
					JustKickedBall = true;
					LastKickTime = GetWorld()->GetTimeSeconds();
				}
			}
		}
		else
		{
			ThinksHasPossession = false;
		}

		// Movement intent
		if (GoingForPossession)
		{
			MoveToBallForKick(DesiredMovement, DeltaTime);
		}
		else if (WaitingForPass)
		{
			MoveToBallForPass(DesiredMovement, DeltaTime);
		}
		else
		{
			FreeMoveDesired();
		}
	}
}

// ----------------------------------------------------------------------------
// Input
// ----------------------------------------------------------------------------
void AFootballer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// old (non-Enhanced) input: controller feeds SetDesiredMovement / SetDesiredSprintStrength
}

// ----------------------------------------------------------------------------
// Indicators
// ----------------------------------------------------------------------------
void AFootballer::ShowTargetingIndicator()
{
	if (TargetingIndicator) TargetingIndicator->SetVisibility(true);
}

void AFootballer::HideTargetingIndicator()
{
	if (TargetingIndicator) TargetingIndicator->SetVisibility(false);
}

// ----------------------------------------------------------------------------
// Control (RPCs)
// ----------------------------------------------------------------------------
void AFootballer::Server_GainPlayerControl_Implementation(AFootballerController* NewController)
{
	if (AIController) { AIController->UnPossess(); }
	FootballerController = NewController;
	ControlledByPlayer = true;

	// make sure we’re visible for the owning player
	SetActorHiddenInGame(false);
	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->SetHiddenInGame(false, true);
		M->SetOwnerNoSee(false);
		M->SetOnlyOwnerSee(false);
	}
}

void AFootballer::Server_LosePlayerControl_Implementation()
{
	if (AIController) { AIController->Possess(this); }
	FootballerController = nullptr;
	ControlledByPlayer = false;
}

void AFootballer::OnRep_ControlledByPlayer()
{
	if (PlayerControlIndicator)
	{
		PlayerControlIndicator->SetVisibility(ControlledByPlayer);
	}
}

// ----------------------------------------------------------------------------
// Pending actions
// ----------------------------------------------------------------------------
void AFootballer::ClearPendingAction()
{
	PendingAction.Type = PendingActionType::FootballerActionNone;
	PendingAction.Power = 0.f;
	PendingAction.Direction = FVector::ZeroVector;
}

void AFootballer::SetPendingShot(float Power, FVector DesiredDirection)
{
	PendingAction.Type = PendingActionType::FootballerActionShot;
	PendingAction.Power = Power;
	PendingAction.Direction = DesiredDirection;
}

void AFootballer::SetPendingPass(float Power, FVector DesiredDirection)
{
	PendingAction.Type = PendingActionType::FootballerActionPass;
	PendingAction.Power = Power;
	PendingAction.Direction = DesiredDirection;
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------
FVector AFootballer::DesiredMovementOrForwardVector()
{
	return (DesiredMovement.SizeSquared2D() <= KINDA_SMALL_NUMBER) ? GetActorForwardVector() : DesiredMovement;
}

static float GMinDistanceForTouch = 100.f;

bool AFootballer::CanKickBall()
{
	if (!Ball) return false;
	FVector D = GetActorLocation() - Ball->GetActorLocation();
	D.Z = 0.f;
	return D.Size() < GMinDistanceForTouch;
}

// ----------------------------------------------------------------------------
// Dribbling
// ----------------------------------------------------------------------------
void AFootballer::KnockBallOn_Implementation(float /*DeltaSeconds*/, float Strength)
{
	if (!Ball) return;

	// Require meaningful forward input
	if (DesiredMovement.SizeSquared2D() < 0.05f) return;

	// only when roughly toward the ball
	FVector ToBall = Ball->GetActorLocation() - GetActorLocation();
	ToBall.Z = 0.f;
	const FVector ToBallDir = ToBall.GetSafeNormal();
	const FVector MoveDir = DesiredMovement.GetSafeNormal2D();
	if (FVector::DotProduct(MoveDir, ToBallDir) < 0.25f) return;

	const FVector BallVel = Ball->GetVelocity();
	const FVector DesiredVel = MoveDir * Strength * 100.f;
	FVector Needed = DesiredVel - BallVel;

	// Clamp to prevent huge blasts
	const float MaxImpulse = 800.f;
	const float NeedSize = Needed.Size();
	if (NeedSize > MaxImpulse) Needed *= (MaxImpulse / NeedSize);

	if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
	{
		SM->AddImpulse(Needed, NAME_None, true);
	}
}

// ----------------------------------------------------------------------------
// Shooting
// ----------------------------------------------------------------------------
static const float MAX_POWER = 1.0f;
static const float MIN_POWER = 0.5f;

void AFootballer::ShootBall_Implementation(float Power, FVector DesiredDirection)
{
	if (!Ball) return;

	if (!CanKickBall())
	{
		SetPendingShot(Power, DesiredDirection);
		return;
	}

	// Pick nearest goal
	float MinDist = FLT_MAX;
	AGoal* TargetGoal = nullptr;
	for (TActorIterator<AGoal> It(GetWorld()); It; ++It)
	{
		const float Dist = GetDistanceTo(*It);
		if (Dist < MinDist) { MinDist = Dist; TargetGoal = *It; }
	}
	if (!TargetGoal) return;

	FVector LeftDir = TargetGoal->LeftPost->GetComponentLocation() - Ball->GetActorLocation();
	FVector RightDir = TargetGoal->RightPost->GetComponentLocation() - Ball->GetActorLocation();
	LeftDir.Normalize(); RightDir.Normalize();

	FVector Aim;
	if (FMath::Acos(LeftDir | DesiredDirection) < FMath::Acos(RightDir | DesiredDirection))
	{
		Aim = TargetGoal->LeftPost->GetComponentLocation();
		Aim.Y += 100.f; Aim.Z = 100.f;
	}
	else
	{
		Aim = TargetGoal->RightPost->GetComponentLocation();
		Aim.Y -= 100.f; Aim.Z = 100.f;
	}

	FVector Dir = Aim - GetActorLocation();
	Dir.Z = 0.f; Dir.Normalize();

	Power = FMath::Clamp(Power, MIN_POWER, MAX_POWER);
	const float XY = 5000.f * Power;
	const float Z = 700.f * Power;

	Dir.X *= XY; Dir.Y *= XY; Dir.Z = Z;

	if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
	{
		SM->AddImpulse(Dir, NAME_None, true);
	}

	ThinksHasPossession = false;
	GoingForPossession = true;
	JustKickedBall = true;
	LastKickTime = GetWorld()->GetTimeSeconds();
}

// ----------------------------------------------------------------------------
// Passing
// ----------------------------------------------------------------------------
TArray<AFootballer*> AFootballer::Teammates()
{
	if (CachedTeammates.Num() != 0) return CachedTeammates;

	TArray<AFootballer*> Out;
	if (!Team) return Out;

	for (TActorIterator<AFootballer> It(GetWorld()); It; ++It)
	{
		AFootballer* F = *It;
		if (F && F != this && F->Team == Team) Out.Add(F);
	}
	CachedTeammates = Out;
	return Out;
}

AFootballer* AFootballer::FindPassTarget(float Power, FVector DesiredDirection)
{
	const float DesiredAngle = FMath::Atan2(DesiredDirection.Y, DesiredDirection.X);

	AFootballer* Closest = nullptr;
	float Best = FLT_MAX;

	const TArray<AFootballer*> Mates = Teammates();
	for (AFootballer* Other : Mates)
	{
		if (!Other) continue;

		const FVector ToOther = Other->GetActorLocation() - GetActorLocation();
		const float AngleToOther = FMath::Atan2(ToOther.Y, ToOther.X);

		const float Angular = FMath::Abs(DesiredAngle - AngleToOther);
		const float Linear = ToOther.Size();

		const float Score = Angular * 800.f + Linear * (1.f - Power);
		if (Score < Best && Other != this)
		{
			Best = Score;
			Closest = Other;
		}
	}
	return Closest;
}

void AFootballer::PassBall_Implementation(float Power, FVector DesiredDirection)
{
	if (!Ball) return;

	if (!CanKickBall())
	{
		SetPendingPass(Power, DesiredDirection);
		return;
	}

	Power = FMath::Clamp(Power, MIN_POWER, MAX_POWER);

	AFootballer* Receiver = FindPassTarget(Power, DesiredDirection);
	if (!Receiver) return;

	FVector Pass = Receiver->GetActorLocation() - Ball->GetActorLocation();
	Pass.Z = 0.f;

	const float Size = Pass.Size();
	const float MIN_PASS_LENGTH = 500.f;   // 5m
	const float MAX_PASS_LENGTH = 3000.f;  // 30m
	const float Clamped = FMath::Clamp(Size, MIN_PASS_LENGTH, MAX_PASS_LENGTH);

	Pass *= (Clamped / Size);
	Pass *= 1.5f;

	const FVector BallVel = Ball->GetVelocity();
	const FVector Needed = Pass - BallVel;

	if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
	{
		SM->AddImpulse(Needed, NAME_None, true);
	}

	ThinksHasPossession = false;
	GoingForPossession = false;
	JustKickedBall = true;
	LastKickTime = GetWorld()->GetTimeSeconds();

	if (FootballerController)
	{
		FootballerController->SwitchToFootballer(Receiver);
	}

	Receiver->GoingForPossession = false;
	Receiver->WaitingForPass = true;
}

// ----------------------------------------------------------------------------
// Movement
// ----------------------------------------------------------------------------
void AFootballer::MoveToBallForKick(FVector /*DesiredEndDirection*/, float /*DeltaSeconds*/)
{
	if (!Ball) return;

	const FVector CharPos = GetActorLocation();
	const FVector TargetPos = Ball->GetActorLocation();

	FVector ToTarget = TargetPos - CharPos;
	ToTarget.Z = 0.f;

	const float Distance = ToTarget.Size();
	const float MaxSpeed = (GetMovementComponent() && GetMovementComponent()->GetMaxSpeed() > 0.f)
		? GetMovementComponent()->GetMaxSpeed()
		: 600.f;

	const float Time = (MaxSpeed > 0.f) ? Distance / MaxSpeed : 0.f;
	const FVector Predicted = TargetPos + Ball->GetVelocity() * Time;

	FVector DesiredPos = Predicted;
	DesiredPos.Z = CharPos.Z;

	FVector MoveDir = DesiredPos - CharPos;
	if (MoveDir.Size() > 1.f) MoveDir.Normalize();

	AddMovementInput(MoveDir);
}

void AFootballer::MoveToBallForPass(FVector DesiredMove, float /*DeltaSeconds*/)
{
	if (!Ball) return;

	const float DesiredAngle = FMath::Atan2(DesiredMove.Y, DesiredMove.X);
	FVector ToBall = Ball->GetActorLocation() - GetActorLocation();
	ToBall.Z = 0.f;

	const float AngleToBall = FMath::Atan2(ToBall.Y, ToBall.X);

	// Trap if close enough
	if (const float Dist = ToBall.Size(); Dist > KINDA_SMALL_NUMBER && Dist < 50.f)
	{
		if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
		{
			SM->SetPhysicsLinearVelocity(FVector::ZeroVector);
		}
	}

	if (DesiredMove.Size() > KINDA_SMALL_NUMBER && FMath::Abs(AngleToBall - DesiredAngle) < PI * 0.75f)
	{
		ToBall.Normalize();
		AddMovementInput(ToBall);
	}
	else
	{
		ToBall.Normalize();
		AddMovementInput(ToBall / 3.f);
	}
}

void AFootballer::FreeMoveDesired()
{
	AddMovementInput(DesiredMovement * (0.5f + 0.5f * DesiredSprintStrength));
}

// ----------------------------------------------------------------------------
// Client-side input mirroring
// ----------------------------------------------------------------------------
void AFootballer::Server_SetDesiredMovement_Implementation(FVector Movement)
{
	DesiredMovement = MakeValidDesired(Movement, DesiredSprintStrength);
}

void AFootballer::SetDesiredMovement(FVector Movement)
{
	DesiredMovement = MakeValidDesired(Movement, DesiredSprintStrength);
	Server_SetDesiredMovement(Movement);
}

void AFootballer::SetDesiredSprintStrength_Implementation(const float Strength)
{
	DesiredSprintStrength = FMath::Clamp(Strength, 0.f, 1.f);
}

void AFootballer::SetGoingForPossession_Implementation(bool bGoing)
{
	GoingForPossession = bGoing;
}
