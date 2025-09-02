#include "FootballerController.h"
#include "Kismet/KismetMathLibrary.h"

AFootballerController::AFootballerController()
{
	bShowMouseCursor = false;
}

void AFootballerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledFootballer = Cast<AFootballer>(InPawn);
}

void AFootballerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	InputComponent->BindAxis(TEXT("MoveForward"), this, &AFootballerController::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &AFootballerController::MoveRight);

	InputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AFootballerController::SprintPressed);
	InputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AFootballerController::SprintReleased);

	InputComponent->BindAction(TEXT("Shoot"), IE_Pressed, this, &AFootballerController::Shoot);
	InputComponent->BindAction(TEXT("Pass"), IE_Pressed, this, &AFootballerController::Pass);
	InputComponent->BindAction(TEXT("KnockOn"), IE_Pressed, this, &AFootballerController::KnockOn);
}

FVector AFootballerController::BuildDesiredMove() const
{
	const float Yaw =
		(bUseCameraYaw && PlayerCameraManager)
		? PlayerCameraManager->GetCameraRotation().Yaw
		: GetControlRotation().Yaw;

	const FRotator YawRot(0.f, Yaw, 0.f);
	const FVector CamFwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector CamRight = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	const FVector Desired = (CamFwd * AxisForward) + (CamRight * AxisRight);
	return FVector(Desired.X, Desired.Y, 0.f);
}

void AFootballerController::MoveForward(float Val)
{
	AxisForward = Val;
	if (!ControlledFootballer) return;
	ControlledFootballer->SetDesiredMovement(BuildDesiredMove());
}

void AFootballerController::MoveRight(float Val)
{
	AxisRight = Val;
	if (!ControlledFootballer) return;
	ControlledFootballer->SetDesiredMovement(BuildDesiredMove());
}

void AFootballerController::SprintPressed()
{
	if (ControlledFootballer) ControlledFootballer->SetDesiredSprintStrength(1.f);
}

void AFootballerController::SprintReleased()
{
	if (ControlledFootballer) ControlledFootballer->SetDesiredSprintStrength(0.f);
}

void AFootballerController::Shoot()
{
	if (!ControlledFootballer) return;
	const FVector Dir = BuildDesiredMove().IsNearlyZero()
		? GetPawn()->GetActorForwardVector()
		: BuildDesiredMove().GetSafeNormal();
	ControlledFootballer->ShootBall(1.f, Dir);
}

void AFootballerController::Pass()
{
	if (!ControlledFootballer) return;
	const FVector Dir = BuildDesiredMove().IsNearlyZero()
		? GetPawn()->GetActorForwardVector()
		: BuildDesiredMove().GetSafeNormal();
	ControlledFootballer->PassBall(0.75f, Dir);
}

void AFootballerCon
