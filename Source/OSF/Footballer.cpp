#include "Footballer.h"
#include "OSF.h"
#include "FootballerController.h"
#include "FootballerAIController.h"
#include "Goal.h"

#include "EngineUtils.h"                      // TActorIterator
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

// ----------------------------------------------------------------------------
// Sample attribute presets
// ----------------------------------------------------------------------------

AFootballer::FootballAttributeInfo FA_Sample_Walcott() {
    AFootballer::FootballAttributeInfo attrs;
    attrs.SprintSpeed = 95;
    return attrs;
}

AFootballer::FootballAttributeInfo FA_Sample_Oezil() {
    AFootballer::FootballAttributeInfo attrs;
    attrs.SprintSpeed = 75;
    return attrs;
}

AFootballer::FootballAttributeInfo FA_Sample_Arteta() {
    AFootballer::FootballAttributeInfo attrs;
    attrs.SprintSpeed = 40;
    return attrs;
}

// ----------------------------------------------------------------------------
// AFootballer
// ----------------------------------------------------------------------------

AFootballer::AFootballer()
{
    PrimaryActorTick.bCanEverTick = true;
    FootballAttributes = FA_Sample_Walcott();

    // Initialize gameplay state
    PendingAction.Type = PendingActionType::FootballerActionNone;
    PendingAction.Power = 0.f;
    PendingAction.Direction = FVector::ZeroVector;

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
}

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
    // DOREPLIFETIME(AFootballer, FootballAttributes);
    DOREPLIFETIME(AFootballer, DisplayName);
    DOREPLIFETIME(AFootballer, Team);
    DOREPLIFETIME(AFootballer, AIController);
    // DOREPLIFETIME(AFootballer, PendingAction);
    DOREPLIFETIME(AFootballer, ControlledByPlayer);
    DOREPLIFETIME(AFootballer, CachedTeammates);
    DOREPLIFETIME(AFootballer, DoneInitialSetup);
}

void AFootballer::BeginPlay()
{
    Super::BeginPlay();

    Ball = ABallsack::GetWorldBall(GetWorld());
    verify(Ball != nullptr);

    if (TargetingIndicator) TargetingIndicator->SetVisibility(false);
    if (!ControlledByPlayer && PlayerControlIndicator)
        PlayerControlIndicator->SetVisibility(false);

    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = FootballAttributes.SprintSpeed * 10.f;
    }

    // Prevent immediate kick after spawn
    LastKickTime = GetWorld()->GetTimeSeconds();
}

// Cooldown helper
static bool CanTouchNow(const UWorld* World, float LastKickTime, float CooldownSeconds = 0.35f)
{
    return World && (World->GetTimeSeconds() - LastKickTime) > CooldownSeconds;
}

void AFootballer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!DoneInitialSetup && HasAuthority())
    {
        // Placeholder for future AI logic
    }

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
        if (CanKickBall() && CanTouchNow(GetWorld(), LastKickTime))
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
            }
            else
            {
                // Only dribble if input exists and toward ball
                const FVector MoveDir = DesiredMovement.GetSafeNormal2D();
                FVector ToBall = Ball ? (Ball->GetActorLocation() - GetActorLocation()) : FVector::ZeroVector;
                ToBall.Z = 0.f;
                const FVector ToBallDir = ToBall.GetSafeNormal();

                const bool bHasInput = DesiredMovement.SizeSquared2D() > 0.05f;
                const bool bFacingBall = FVector::DotProduct(MoveDir, ToBallDir) > 0.2f;

                if (bHasInput && bFacingBall)
                {
                    KnockBallOn(DeltaTime, 5.f + 5.f * DesiredSprintStrength);
                    JustKickedBall = true;
                    LastKickTime = GetWorld()->GetTimeSeconds();
                }
            }

            ClearPendingAction();
        }
        else
        {
            ThinksHasPossession = false;
        }

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
    else
    {
        // TODO: AI tick
    }
}

// ---------------- Input ----------------

void AFootballer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// ---------------- Indicators ----------------

void AFootballer::ShowTargetingIndicator()
{
    if (TargetingIndicator) TargetingIndicator->SetVisibility(true);
}
void AFootballer::HideTargetingIndicator()
{
    if (TargetingIndicator) TargetingIndicator->SetVisibility(false);
}

// ---------------- Control ----------------

void AFootballer::Server_GainPlayerControl_Implementation(AFootballerController* NewController)
{
    if (AIController) { AIController->UnPossess(); }
    FootballerController = NewController;
    ControlledByPlayer = true;
}
bool AFootballer::Server_GainPlayerControl_Validate(AFootballerController* /*NewController*/) { return true; }

void AFootballer::Server_LosePlayerControl_Implementation()
{
    if (AIController) { AIController->Possess(this); }
    FootballerController = nullptr;
    ControlledByPlayer = false;
}
bool AFootballer::Server_LosePlayerControl_Validate() { return true; }

void AFootballer::OnRep_ControlledByPlayer()
{
    if (PlayerControlIndicator)
    {
        PlayerControlIndicator->SetVisibility(ControlledByPlayer);
    }
}
bool AFootballer::IsControlledByPlayer()
{
    return ControlledByPlayer;
}

// ---------------- Pending Actions ----------------

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

// ---------------- Helpers ----------------

FVector AFootballer::DesiredMovementOrForwardVector()
{
    return (DesiredMovement.Size() <= FLT_EPSILON) ? GetActorForwardVector() : DesiredMovement;
}

static float MIN_DISTANCE_FOR_TOUCH = 100.f;
bool AFootballer::CanKickBall()
{
    if (!Ball) return false;
    FVector Distance = GetActorLocation() - Ball->GetActorLocation();
    Distance.Z = 0.f;
    return Distance.Size() < MIN_DISTANCE_FOR_TOUCH;
}

// ---------------- Dribbling ----------------

void AFootballer::KnockBallOn_Implementation(float /*DeltaSeconds*/, float Strength)
{
    if (!Ball) return;
    if (DesiredMovement.SizeSquared2D() < 0.05f) return;

    FVector ToBall = Ball->GetActorLocation() - GetActorLocation();
    ToBall.Z = 0.f;
    const FVector ToBallDir = ToBall.GetSafeNormal();
    const FVector MoveDir = DesiredMovement.GetSafeNormal2D();
    if (FVector::DotProduct(MoveDir, ToBallDir) < 0.2f) return;

    const FVector BallVel = Ball->GetVelocity();
    const FVector DesiredVel = MoveDir * Strength * 100.f;

    FVector Needed = DesiredVel - BallVel;

    const float MaxImpulse = 800.f;
    if (Needed.Size() > MaxImpulse)
    {
        Needed *= (MaxImpulse / Needed.Size());
    }

    if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
    {
        SM->AddImpulse(Needed, NAME_None, true);
    }
}
bool AFootballer::KnockBallOn_Validate(float, float) { return true; }

// ---------------- Shooting ----------------

static const float MAX_POWER = 1.0f;
static const float MIN_POWER = 0.5f;

void AFootballer::ShootBall_Implementation(float Power, FVector DesiredDirection)
{
    if (!Ball) return;
    if (!CanKickBall()) { SetPendingShot(Power, DesiredDirection); return; }

    float MinDistance = FLT_MAX;
    AGoal* CloserGoal = nullptr;
    for (TActorIterator<AGoal> It(GetWorld()); It; ++It)
    {
        AGoal* Goal = *It;
        const float Dist = GetDistanceTo(Goal);
        if (Dist < MinDistance) { MinDistance = Dist; CloserGoal = Goal; }
    }
    if (!CloserGoal) return;

    FVector LeftDir = CloserGoal->LeftPost->GetComponentLocation() - Ball->GetActorLocation();
    FVector RightDir = CloserGoal->RightPost->GetComponentLocation() - Ball->GetActorLocation();
    LeftDir.Normalize(); RightDir.Normalize();

    FVector Target;
    if (FMath::Acos(LeftDir | DesiredDirection) < FMath::Acos(RightDir | DesiredDirection))
    {
        Target = CloserGoal->LeftPost->GetComponentLocation(); Target.Y += 100.f; Target.Z = 100.f;
    }
    else { Target = CloserGoal->RightPost->GetComponentLocation(); Target.Y -= 100.f; Target.Z = 100.f; }

    DesiredDirection = Target - GetActorLocation();
    DesiredDirection.Z = 0.f; DesiredDirection.Normalize();

    Power = FMath::Clamp(Power, MIN_POWER, MAX_POWER);
    FVector Impulse = DesiredDirection * (5000.f * Power);
    Impulse.Z = 700.f * Power;

    if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
    {
        SM->AddImpulse(Impulse, NAME_None, true);
    }

    ThinksHasPossession = false;
    GoingForPossession = true;
    JustKickedBall = true;
    LastKickTime = GetWorld()->GetTimeSeconds();
}
bool AFootballer::ShootBall_Validate(float, FVector) { return true; }

// ---------------- Passing ----------------

TArray<AFootballer*> AFootballer::Teammates()
{
    if (CachedTeammates.Num() != 0) return CachedTeammates;

    TArray<AFootballer*> Result;
    if (!Team) return Result;

    for (TActorIterator<AFootballer> It(GetWorld()); It; ++It)
    {
        AFootballer* F = *It;
        if (F && F != this && F->Team == Team) { Result.Add(F); }
    }

    CachedTeammates = Result;
    return Result;
}

AFootballer* AFootballer::FindPassTarget(float Power, FVector DesiredDirection)
{
    const float DesiredAngle = FMath::Atan2(DesiredDirection.Y, DesiredDirection.X);

    AFootballer* Closest = nullptr;
    float BestScore = FLT_MAX;

    const TArray<AFootballer*> Mates = Teammates();
    for (AFootballer* Other : Mates)
    {
        if (!Other) continue;
        const FVector ToOther = Other->GetActorLocation() - GetActorLocation();
        const float AngleToOther = FMath::Atan2(ToOther.Y, ToOther.X);
        const float Angular = FMath::Abs(DesiredAngle - AngleToOther);
        const float Linear = ToOther.Size();
        const float Score = Angular * 800.f + Linear * (1.f - Power);
        if (Score < BestScore) { BestScore = Score; Closest = Other; }
    }
    return Closest;
}

void AFootballer::PassBall_Implementation(float Power, FVector DesiredDirection)
{
    if (!Ball) return;
    if (!CanKickBall()) { SetPendingPass(Power, DesiredDirection); return; }

    Power = FMath::Clamp(Power, MIN_POWER, MAX_POWER);
    AFootballer* Receiver = FindPassTarget(Power, DesiredDirection);
    if (!Receiver) return;

    FVector Pass = Receiver->GetActorLocation() - Ball->GetActorLocation();
    Pass.Z = 0.f;
    const float Size = Pass.Size();
    const float MIN_PASS_LENGTH = 500.f;
    const float MAX_PASS_LENGTH = 3000.f;
    Pass *= FMath::Clamp(Size, MIN_PASS_LENGTH, MAX_PASS_LENGTH) / Size;
    Pass *= 1.5f;

    FVector Needed = Pass - Ball->GetVelocity();

    if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
    {
        SM->AddImpulse(Needed, NAME_None, true);
    }

    ThinksHasPossession = false;
    GoingForPossession = false;
    JustKickedBall = true;
    LastKickTime = GetWorld()->GetTimeSeconds();

    if (FootballerController) { FootballerController->SwitchToFootballer(Receiver); }
    Receiver->GoingForPossession = false;
    Receiver->WaitingForPass = true;
}
bool AFootballer::PassBall_Validate(float, FVector) { return true; }

// ---------------- Movement ----------------

void AFootballer::MoveToBallForKick(FVector, float)
{
    if (!Ball) return;
    const FVector CharPos = GetActorLocation();
    const FVector TargetPos = Ball->GetActorLocation();
    FVector ToTarget = TargetPos - CharPos; ToTarget.Z = 0.f;
    float Distance = ToTarget.Size();
    float MaxSpeed = GetMovementComponent() ? GetMovementComponent()->GetMaxSpeed() : 600.f;
    float Time = (MaxSpeed > 0.f) ? Distance / MaxSpeed : 0.f;
    FVector Predicted = TargetPos + Ball->GetVelocity() * Time;
    FVector MoveDir = (Predicted - CharPos); MoveDir.Z = 0.f;
    if (MoveDir.Size() > 1.f) MoveDir.Normalize();
    AddMovementInput(MoveDir);
}

void AFootballer::MoveToBallForPass(FVector DesiredMove, float)
{
    if (!Ball) return;
    float DesiredAngle = FMath::Atan2(DesiredMove.Y, DesiredMove.X);
    FVector ToBall = Ball->GetActorLocation() - GetActorLocation();
    ToBall.Z = 0.f;
    float AngleToBall = FMath::Atan2(ToBall.Y, ToBall.X);
    if (ToBall.Size() > FLT_EPSILON && ToBall.Size() < 50.f)
    {
        if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent()) { SM->SetPhysicsLinearVelocity(FVector::ZeroVector); }
    }
    ToBall.Normalize();
    if (DesiredMove.Size() > FLT_EPSILON && FMath::Abs(AngleToBall - DesiredAngle) < PI * 0.75f) AddMovementInput(ToBall);
    else AddMovementInput(ToBall / 3.f);
}

void AFootballer::FreeMoveDesired()
{
    FVector Move = DesiredMovement * (0.5f + 0.5f * DesiredSprintStrength);
    AddMovementInput(Move);
}

static FVector calculateValidMovement(FVector Movement, int SprintStrength)
{
    Movement.Normalize();
    hope(SprintStrength <= 1);
    return Movement * (0.5f + 0.5f * SprintStrength);
}

void AFootballer::Server_SetDesiredMovement_Implementation(FVector Movement)
{
    DesiredMovement = calculateValidMovement(Movement, DesiredSprintStrength);
}
bool AFootballer::Server_SetDesiredMovement_Validate(FVector) { return true; }

void AFootballer::SetDesiredMovement(FVector Movement)
{
    DesiredMovement = calculateValidMovement(Movement, DesiredSprintStrength);
    Server_SetDesiredMovement(Movement);
}

void AFootballer::SetDesiredSprintStrength_Implementation(const float Strength)
{
    DesiredSprintStrength = simpleclamp(Strength, 0.f, 1.f);
}
bool AFootballer::SetDesiredSprintStrength_Validate(float) { return true; }

void AFootballer::SetGoingForPossession_Implementation(bool bGoing)
{
    GoingForPossession = bGoing;
}
bool AFootballer::SetGoingForPossession_Validate(bool) { return true; }
