// FootballerController.cpp

#include "FootballerController.h"
#include "Footballer.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

AFootballerController::AFootballerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableTouchEvents = false;
}

void AFootballerController::BeginPlay()
{
	Super::BeginPlay();
}

void AFootballerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledFootballer = Cast<AFootballer>(InPawn);
}

void AFootballerController::OnUnPossess()
{
	Super::OnUnPossess();
	ControlledFootballer = nullptr;
}

void AFootballerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	// Axis (legacy)
	InputComponent->BindAxis(TEXT("MoveForward"), this, &AFootballerController::AxisMoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &AFootballerController::AxisMoveRight);
	InputComponent->BindAxis(TEXT("SprintAxis"), this, &AFootballerController::AxisSprint);

	// Actions (FIFA-style you chose)
	InputComponent->BindAction(TEXT("Shoot"), IE_Pressed, this, &AFootballerController::ActionShoot);
	InputComponent->BindAction(TEXT("Pass"), IE_Pressed, this, &AFootballerController::ActionPass);
	InputComponent->BindAction(TEXT("Through"), IE_Pressed, this, &AFootballerController::ActionThrough);
	InputComponent->BindAction(TEXT("SwitchPlayer"), IE_Pressed, this, &AFootballerController::ActionSwitchPlayer);
}

void AFootballerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ControlledFootballer) return;

	// Build camera-relative move and push to the pawn
	const FVector DesiredMove = BuildDesiredMovement();
	ControlledFootballer->SetDesiredMovement(DesiredMove);
	ControlledFootballer->SetDesiredSprintStrength(FMath::Clamp(SprintAxis, 0.f, 1.f));
}

FVector AFootballerController::BuildDesiredMovement() const
{
	// Camera yaw (ignore pitch/roll)
	FRotator CamRot = FRotator::ZeroRotator;
	if (PlayerCameraManager)
	{
		CamRot = PlayerCameraManager->GetCameraRotation();
	}
	CamRot.Pitch = 0.f;
	CamRot.Roll = 0.f;

	const FVector CamForward = CamRot.Vector();                // X forward (world)
	const FVector CamRight = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y); // Y right (world)

	FVector Desired = CamForward * ForwardAxis + CamRight * RightAxis;
	Desired.Z = 0.f;
	Desired = Desired.GetClampedToMaxSize(1.f);
	return Desired;
}

void AFootballerController::AxisMoveForward(float Value)
{
	ForwardAxis = FMath::Clamp(Value, -1.f, 1.f);
}

void AFootballerController::AxisMoveRight(float Value)
{
	RightAxis = FMath::Clamp(Value, -1.f, 1.f);
}

void AFootballerController::AxisSprint(float Value)
{
	// Spacebar or RT gives 0..1 (keep it ≥0)
	SprintAxis = FMath::Max(0.f, Value);
}

void AFootballerController::ActionShoot()
{
	if (!ControlledFootballer) return;
	const FVector Dir = BuildDesiredMovement().GetSafeNormal();
	ControlledFootballer->ShootBall(1.f, Dir);
}

void AFootballerController::ActionPass()
{
	if (!ControlledFootballer) return;
	const FVector Dir = BuildDesiredMovement().GetSafeNormal();
	ControlledFootballer->PassBall(0.75f, Dir);
}

void AFootballerController::ActionThrough()
{
	if (!ControlledFootballer) return;
	const FVector Dir = BuildDesiredMovement().GetSafeNormal();
	// Light knock-on if we’re already in contact (or queues if not)
	ControlledFootballer->KnockBallOn(0.f, 6.f); // strength tuned in your pawn
	// Optional: also set a pending pass with smaller power:
	// ControlledFootballer->PassBall(0.55f, Dir);
}

void AFootballerController::ActionSwitchPlayer()
{
	AFootballer* Next = FindNextTeammate();
	if (Next)
	{
		SwitchToFootballer(Next);
	}
}

void AFootballerController::SwitchToFootballer(AFootballer* NewFootballer)
{
	if (!NewFootballer) return;

	// Release current
	if (ControlledFootballer)
	{
		ControlledFootballer->Server_LosePlayerControl();
	}

	// Possess and mark
	Possess(NewFootballer);
	ControlledFootballer = NewFootballer;
	NewFootballer->Server_GainPlayerControl(this);
}

AFootballer* AFootballerController::FindNextTeammate() const
{
	if (!ControlledFootballer || !ControlledFootballer->Team) return nullptr;

	AFootballer* First = nullptr;
	bool bReturnNext = false;

	for (TActorIterator<AFootballer> It(GetWorld()); It; ++It)
	{
		AFootballer* F = *It;
		if (!F || F->Team != ControlledFootballer->Team) continue;

		if (!First) First = F;

		if (bReturnNext)
			return F;

		if (F == ControlledFootballer)
			bReturnNext = true;
	}

	// wrap-around
	return First;
}
