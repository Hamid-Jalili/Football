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
// Tuning constants (knock/shot/pass safety & feel)
// ----------------------------------------------------------------------------
static constexpr float TOUCH_MIN_DIST_CM = 75.f;    // start of touch band
static constexpr float TOUCH_MAX_DIST_CM = 120.f;   // end of touch band
static constexpr float TOUCH_COOLDOWN_S = 0.35f;   // min time between impulses
static constexpr float INPUT_THRESHOLD2D = 0.05f;   // minimum input magnitude^2
static constexpr float ANGLE_DOT_MIN = 0.25f;   // must push roughly toward ball
static constexpr float MAX_BALL_SPEED_CM_S = 1800.f;  // hard cap ~18 m/s
static constexpr float MAX_KNOCK_IMPULSE = 600.f;   // safety impulse limit

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

    if (TargetingIndicator)       TargetingIndicator->SetVisibility(false);
    if (!ControlledByPlayer && PlayerControlIndicator)
        PlayerControlIndicator->SetVisibility(false);

    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = FootballAttributes.SprintSpeed * 10.f;
    }

    // Grace period to avoid instant touch on spawn
    LastKickTime = GetWorld()->GetTimeSeconds();
}

// Small helper: rate-limit ball touches
static bool CanTouchNow(const UWorld* World, float LastKickTime)
{
    return World && (World->GetTimeSeconds() - LastKickTime) > TOUCH_COOLDOWN_S;
}

void AFootballer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!DoneInitialSetup && HasAuthority())
    {
        // Placeholder for future AI possession logic
    }

    // Reset JustKickedBall once we're far enough from the ball
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
        if (CanKickBall())
        {
            GoingForPossession = true;
            ThinksHasPossession = true;

            if (PendingAction.Type != PendingActionType::FootballerActionNone)
            {
                switch (PendingAction.Type)
                {
                case PendingActionType::FootballerActionShot:
                    if (CanTouchNow(GetWorld(), LastKickTime))
                    {
                        ShootBall(PendingAction.Power, PendingAction.Direction);
                        JustKickedBall = true;
                        LastKickTime = GetWorld()->GetTimeSeconds();
                    }
                    break;
                case PendingActionType::FootballerActionPass:
                    if (CanTouchNow(GetWorld(), LastKickTime))
                    {
                        PassBall(PendingAction.Power, PendingAction.Direction);
                        JustKickedBall = true;
                        LastKickTime = GetWorld()->GetTimeSeconds();
                    }
                    break;
                default: break;
                }
            }
            else
            {
                // Default controlled touch/dribble only with real input, roughly toward ball
                const FVector MoveDir = DesiredMovement.GetSafeNormal2D();
                FVector ToBall = (Ball ? (Ball->GetActorLocation() - GetActorLocation()) : FVector::ZeroVector);
                ToBall.Z = 0.f;
                const FVector ToBallDir = ToBall.GetSafeNormal();

                const bool bHasInput = DesiredMovement.SizeSquared2D() > INPUT_THRESHOLD2D;
                const bool bTowardBall = FVector::DotProduct(MoveDir, ToBallDir) > ANGLE_DOT_MIN;

                if (bHasInput && bTowardBall && CanTouchNow(GetWorld(), LastKickTime))
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
        // AI tick placeholder
    }
}

// Avoid C4458: don't shadow AActor::InputComponent
void AFootballer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Bind inputs here if/when you add them
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
// Helpers & state
// ----------------------------------------------------------------------------

FVector AFootballer::DesiredMovementOrForwardVector()
{
    return (DesiredMovement.Size() <= FLT_EPSILON) ? GetActorForwardVector() : DesiredMovement;
}

bool AFootballer::CanKickBall()
{
    if (!Ball) return false;
    FVector Distance = GetActorLocation() - Ball->GetActorLocation();
    Distance.Z = 0.f;
    const float Dist = Distance.Size();
    // Touch band avoids impulses while overlapping at 0 cm
    return (Dist >= TOUCH_MIN_DIST_CM && Dist <= TOUCH_MAX_DIST_CM);
}

// ----------------------------------------------------------------------------
// Dribbling
// ----------------------------------------------------------------------------

void AFootballer::KnockBallOn_Implementation(float /*DeltaSeconds*/, float Strength)
{
    if (!Ball) return;

    // Require meaningful input & roughly toward the ball
    if (DesiredMovement.SizeSquared2D() < INPUT_THRESHOLD2D) return;
    FVector ToBall = Ball->GetActorLocation() - GetActorLocation();
    ToBall.Z = 0.f;
    const FVector ToBallDir = ToBall.GetSafeNormal();
    const FVector MoveDir = DesiredMovement.GetSafeNormal2D();
    if (FVector::DotProduct(MoveDir, ToBallDir) < ANGLE_DOT_MIN) return;

    if (!CanTouchNow(GetWorld(), LastKickTime)) return;

    // Current ball speed
    const FVector BallVel = Ball->GetVelocity();
    const float BallSpeed = BallVel.Size2D();
    if (BallSpeed >= MAX_BALL_SPEED_CM_S) return;

    // Target ground speed for a controlled knock
    const float TargetSpeed = FMath::Min(MAX_BALL_SPEED_CM_S, (Strength * 100.f));
    const FVector DesiredVel = MoveDir * TargetSpeed;
    FVector Needed = DesiredVel - BallVel;

    // Safety clamp to avoid “rocket” first touches
    const float NeedSize = Needed.Size();
    if (NeedSize > MAX_KNOCK_IMPULSE)
    {
        Needed *= (MAX_KNOCK_IMPULSE / NeedSize);
    }

    if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
    {
        SM->AddImpulse(Needed, NAME_None, true);
        // Clamp resulting linear velocity
        FVector V = SM->GetPhysicsLinearVelocity();
        const float S = V.Size2D();
        if (S > MAX_BALL_SPEED_CM_S)
        {
            const FVector Capped = V.GetSafeNormal2D() * MAX_BALL_SPEED_CM_S;
            SM->SetPhysicsLinearVelocity(FVector(Capped.X, Capped.Y, V.Z), false);
        }
    }
}
bool AFootballer::KnockBallOn_Validate(float /*DeltaSeconds*/, float /*Strength*/) { return true; }

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

    float MinDistance = FLT_MAX;
    AGoal* CloserGoal = nullptr;
    for (TActorIterator<AGoal> It(GetWorld()); It; ++It)
    {
        AGoal* Goal = *It;
        const float Dist = GetDistanceTo(Goal);
        if (Dist < MinDistance)
        {
            MinDistance = Dist;
            CloserGoal = Goal;
        }
    }
    if (!CloserGoal) return;

    FVector LeftDir = CloserGoal->LeftPost->GetComponentLocation() - Ball->GetActorLocation();
    FVector RightDir = CloserGoal->RightPost->GetComponentLocation() - Ball->GetActorLocation();
    LeftDir.Normalize();
    RightDir.Normalize();

    FVector Target;
    if (FMath::Acos(LeftDir | DesiredDirection) < FMath::Acos(RightDir | DesiredDirection))
    {
        Target = CloserGoal->LeftPost->GetComponentLocation();
        Target.Y += 100.f;
        Target.Z = 100.f;
    }
    else
    {
        Target = CloserGoal->RightPost->GetComponentLocation();
        Target.Y -= 100.f;
        Target.Z = 100.f;
    }

    DesiredDirection = Target - GetActorLocation();
    DesiredDirection.Z = 0.f;
    DesiredDirection.Normalize();

    Power = FMath::Clamp(Power, MIN_POWER, MAX_POWER);
    const float XY = 5000.f * Power;
    const float Z = 700.f * Power;

    DesiredDirection.X *= XY;
    DesiredDirection.Y *= XY;
    DesiredDirection.Z = Z;

    if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
    {
        SM->AddImpulse(DesiredDirection, NAME_None, true);
        // Cap speed
        FVector V = SM->GetPhysicsLinearVelocity();
        const float S = V.Size2D();
        if (S > MAX_BALL_SPEED_CM_S)
        {
            const FVector Capped = V.GetSafeNormal2D() * MAX_BALL_SPEED_CM_S;
            SM->SetPhysicsLinearVelocity(FVector(Capped.X, Capped.Y, V.Z), false);
        }
    }

    ThinksHasPossession = false;
    GoingForPossession = true;
    JustKickedBall = true;
    LastKickTime = GetWorld()->GetTimeSeconds();
}
bool AFootballer::ShootBall_Validate(float /*a*/, FVector /*b*/) { return true; }

// ----------------------------------------------------------------------------
// Passing
// ----------------------------------------------------------------------------

TArray<AFootballer*> AFootballer::Teammates()
{
    if (CachedTeammates.Num() != 0) return CachedTeammates;

    TArray<AFootballer*> Result;
    if (!Team) return Result;

    for (TActorIterator<AFootballer> It(GetWorld()); It; ++It)
    {
        AFootballer* F = *It;
        if (F && F != this && F->Team == Team)
        {
            Result.Add(F);
        }
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
        if (Score < BestScore && Other != this)
        {
            BestScore = Score;
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
    const float MIN_PASS_LENGTH = 500.f;    // 5m
    const float MAX_PASS_LENGTH = 3000.f;   // 30m
    const float Clamped = FMath::Clamp(Size, MIN_PASS_LENGTH, MAX_PASS_LENGTH);

    Pass *= (Clamped / Size);
    Pass *= 1.5f; // general boost

    const FVector BallVel = Ball->GetVelocity();
    const FVector Needed = Pass - BallVel;

    if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
    {
        SM->AddImpulse(Needed, NAME_None, true);
        // Cap speed
        FVector V = SM->GetPhysicsLinearVelocity();
        const float S = V.Size2D();
        if (S > MAX_BALL_SPEED_CM_S)
        {
            const FVector Capped = V.GetSafeNormal2D() * MAX_BALL_SPEED_CM_S;
            SM->SetPhysicsLinearVelocity(FVector(Capped.X, Capped.Y, V.Z), false);
        }
    }

    ThinksHasPossession = false;
    GoingForPossession = false;
    JustKickedBall = true;
    LastKickTime = GetWorld()->GetTimeSeconds();

    if (FootballerController)
    {
        FootballerController->SwitchToFootballer(Receiver);
    }

    Receiver->GoingForPossession = false; // after controller switch
    Receiver->WaitingForPass = true;
}
bool AFootballer::PassBall_Validate(float /*Power*/, FVector /*Direction*/) { return true; }

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
    if (MoveDir.Size() > 1.f) { MoveDir.Normalize(); }

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
    if (const float Dist = ToBall.Size(); Dist > FLT_EPSILON && Dist < 50.f)
    {
        if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
        {
            SM->SetPhysicsLinearVelocity(FVector::ZeroVector);
        }
    }

    if (DesiredMove.Size() > FLT_EPSILON && FMath::Abs(AngleToBall - DesiredAngle) < PI * 0.75f)
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
bool AFootballer::Server_SetDesiredMovement_Validate(FVector /*Movement*/) { return true; }

void AFootballer::SetDesiredMovement(FVector Movement)
{
    DesiredMovement = calculateValidMovement(Movement, DesiredSprintStrength);
    Server_SetDesiredMovement(Movement);
}

void AFootballer::SetDesiredSprintStrength_Implementation(const float Strength)
{
    DesiredSprintStrength = simpleclamp(Strength, 0.f, 1.f);
}
bool AFootballer::SetDesiredSprintStrength_Validate(float /*Strength*/) { return true; }

void AFootballer::SetGoingForPossession_Implementation(bool bGoing)
{
    GoingForPossession = bGoing;
}
bool AFootballer::SetGoingForPossession_Validate(bool /*bGoing*/) { return true; }
