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
// Tunables
// ----------------------------------------------------------------------------
static float MIN_DISTANCE_FOR_TOUCH = 120.f;   // possession radius
static float LOSE_POSSESSION_DISTANCE = 180.f;   // drop possession if > this
static float TOUCH_COOLDOWN = 0.30f;   // seconds between “touches”
static float MAX_DRIBBLE_IMPULSE = 160.f;   // gentle carry cap

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

    bHasPossession = false;
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
    DOREPLIFETIME(AFootballer, bHasPossession);
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

    // Prevent first-tick auto touch
    LastKickTime = GetWorld()->GetTimeSeconds();
    bHasPossession = false;
    JustKickedBall = false;
}

bool AFootballer::HasMeaningfulInput() const
{
    return DesiredMovement.SizeSquared2D() > 0.05f;
}

void AFootballer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Reset spam gate when far from ball
    if (JustKickedBall && !CanKickBall())
    {
        JustKickedBall = false;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const bool bTouchReady = (Now - LastKickTime) > TOUCH_COOLDOWN;

    // Maintain possession
    if (CanKickBall() && bTouchReady)
    {
        bHasPossession = true;
    }
    else
    {
        if (Ball)
        {
            FVector D = GetActorLocation() - Ball->GetActorLocation();
            D.Z = 0.f;
            if (D.Size() > LOSE_POSSESSION_DISTANCE)
            {
                bHasPossession = false;
            }
        }
        else
        {
            bHasPossession = false;
        }
    }

    if (PlayerControlIndicator)
    {
        PlayerControlIndicator->SetVisibility(ControlledByPlayer);
    }

    if (ControlledByPlayer)
    {
        if (bHasPossession && HasMeaningfulInput())
        {
            // Execute queued actions explicitly; otherwise dribble
            if (PendingAction.Type == PendingActionType::FootballerActionShot)
            {
                ShootBall(PendingAction.Power, PendingAction.Direction);
                JustKickedBall = true;
                LastKickTime = Now;
                ClearPendingAction();
            }
            else if (PendingAction.Type == PendingActionType::FootballerActionPass)
            {
                PassBall(PendingAction.Power, PendingAction.Direction);
                JustKickedBall = true;
                LastKickTime = Now;
                ClearPendingAction();
            }
            else
            {
                DribbleBall(DeltaTime);   // gentle keep-in-front; no blasts
            }

            GoingForPossession = true;
            ThinksHasPossession = true;
        }
        else
        {
            ThinksHasPossession = false;

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
    else
    {
        // AI tick placeholder
    }
}

// Avoid C4458: don't shadow AActor::InputComponent
void AFootballer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // bind inputs here if/when you add them
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
    return Distance.Size() < MIN_DISTANCE_FOR_TOUCH;
}

// ----------------------------------------------------------------------------
// Dribbling (gentle keep-in-front)
// ----------------------------------------------------------------------------

void AFootballer::DribbleBall_Implementation(float DeltaSeconds)
{
    if (!Ball) return;

    // Keep ball a bit in front of the feet
    FVector Fwd = GetActorForwardVector(); Fwd.Z = 0.f; Fwd.Normalize();
    const FVector DesiredBallPos = GetActorLocation() + Fwd * 80.f;

    FVector ToTarget = DesiredBallPos - Ball->GetActorLocation();
    ToTarget.Z = 0.f;

    // If ball already too fast, damp it a little
    const FVector Vel = Ball->GetVelocity();
    if (Vel.Size() > 1200.f)
    {
        if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
        {
            SM->SetPhysicsLinearVelocity(Vel * 0.65f);
        }
    }

    // Only nudge if off-line/behind
    if (ToTarget.Size() > 30.f)
    {
        FVector Impulse = ToTarget * 5.f;
        Impulse = Impulse.GetClampedToMaxSize(MAX_DRIBBLE_IMPULSE);
        Impulse *= (0.6f + 0.6f * FMath::Clamp(DesiredSprintStrength, 0.f, 1.f));

        if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
        {
            SM->AddImpulse(Impulse, NAME_None, true);
        }
    }
}

// Keep legacy KnockBallOn for explicit use (not called automatically)
void AFootballer::KnockBallOn_Implementation(float /*deltaSeconds*/, float Strength)
{
    if (!Ball) return;

    // Require meaningful input and roughly facing the ball
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
    const float NeededSize = Needed.Size();
    if (NeededSize > MaxImpulse)
    {
        Needed *= (MaxImpulse / NeededSize);
    }

    if (UStaticMeshComponent* SM = Ball->GetStaticMeshComponent())
    {
        SM->AddImpulse(Needed, NAME_None, true);
    }
}
bool AFootballer::KnockBallOn_Validate(float /*deltaSeconds*/, float /*Strength*/) { return true; }

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
    const float MIN_PASS_LENGTH = 500.f;   // 5m
    const float MAX_PASS_LENGTH = 3000.f;  // 30m
    const float Clamped = FMath::Clamp(Size, MIN_PASS_LENGTH, MAX_PASS_LENGTH);

    Pass *= (Clamped / Size);
    Pass *= 1.5f; // general boost

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
