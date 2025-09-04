#include "FootballerController.h"
#include "Footballer.h"
#include "FootballTeam.h"          // <-- for TeamRef and Players array
#include "Ballsack.h"              // <-- for ABallsack
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

AFootballerController::AFootballerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = false;
}

void AFootballerController::BeginPlay()
{
	Super::BeginPlay();
	ControlledFootballer = Cast<AFootballer>(GetPawn());
}

void AFootballerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	InputComponent->BindAxis("MoveForward", this, &AFootballerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AFootballerController::MoveRight);

	InputComponent->BindAction("Sprint", IE_Pressed, this, &AFootballerController::SprintPressed);
	InputComponent->BindAction("Sprint", IE_Released, this, &AFootballerController::SprintReleased);

	InputComponent->BindAction("Shoot", IE_Pressed, this, &AFootballerController::ShootPressed);
	InputComponent->BindAction("Pass", IE_Pressed, this, &AFootballerController::PassPressed);

	InputComponent->BindAction("SwitchPlayer", IE_Pressed, this, &AFootballerController::SwitchToNearestTeammate);
}

void AFootballerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ControlledFootballer)
	{
		ControlledFootballer = Cast<AFootballer>(GetPawn());
	}
	if (!ControlledFootballer) return;

	const FVector DesiredMove = BuildDesiredMove();
	ControlledFootballer->SetDesiredSprintStrength(bSprint ? 1.f : 0.f);
	ControlledFootballer->SetDesiredMovement(DesiredMove);
}

void AFootballerController::MoveForward(float Value)
{
	MoveInput.X = FMath::Clamp(Value, -1.f, 1.f);
}

void AFootballerController::MoveRight(float Value)
{
	MoveInput.Y = FMath::Clamp(Value, -1.f, 1.f);
}

void AFootballerController::SprintPressed()
{
	bSprint = true;
}

void AFootballerController::SprintReleased()
{
	bSprint = false;
}

void AFootballerController::ShootPressed()
{
	if (!ControlledFootballer) return;

	// Shoot straight ahead in the player's facing direction.
	FVector Dir = ControlledFootballer->GetActorForwardVector();
	Dir.Z = 0.f;
	Dir.Normalize();
	ControlledFootballer->ShootBall(1.f, Dir);
}

void AFootballerController::PassPressed()
{
	if (!ControlledFootballer) return;

	// Default pass forward; if strong lateral input, pass sideways relative to player.
	const FVector Fwd = ControlledFootballer->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = ControlledFootballer->GetActorRightVector().GetSafeNormal2D();

	FVector Dir = Fwd;
	if (FMath::Abs(MoveInput.Y) > 0.5f)
	{
		Dir = (MoveInput.Y > 0.f) ? Right : -Right;
	}
	ControlledFootballer->PassBall(0.75f, Dir);
}

void AFootballerController::SwitchToNearestTeammate()
{
	if (!ControlledFootballer || !ControlledFootballer->TeamRef) return;

	ABallsack* Ball = Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass()));
	if (!Ball) return;

	const FVector BallLoc = Ball->GetActorLocation();
	AFootballer* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();

	for (AFootballer* Mate : ControlledFootballer->TeamRef->Players)
	{
		if (!Mate || Mate == ControlledFootballer) continue;
		const float D = FVector::DistSquared(Mate->GetActorLocation(), BallLoc);
		if (D < BestDist)
		{
			BestDist = D;
			Best = Mate;
		}
	}

	if (Best)
	{
		Possess(Best);
		ControlledFootballer = Best;
	}
}

FVector AFootballerController::BuildDesiredMove() const
{
	// Move relative to camera yaw so WASD is screen-aligned.
	const FRotator ViewRot = PlayerCameraManager ? PlayerCameraManager->GetCameraRotation() : GetControlRotation();
	const FRotator YawRot(0.f, ViewRot.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	return (Forward * MoveInput.X + Right * MoveInput.Y);
}
