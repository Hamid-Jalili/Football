// FootballerController.cpp
#include "FootballerController.h"
#include "Footballer.h"

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
}

AFootballer* AFootballerController::GetControlledFootballer() const
{
	return Cast<AFootballer>(GetPawn());
}

void AFootballerController::MoveForward(float Value)
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		const FVector Desired = FVector(1, 0, 0) * Value;
		Me->SetDesiredMovement(Desired + Me->GetDesiredMove() * 0.f);
	}
}

void AFootballerController::MoveRight(float Value)
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		const FVector Desired = FVector(0, 1, 0) * Value;
		Me->SetDesiredMovement(Desired + Me->GetDesiredMove() * 0.f);
	}
}

void AFootballerController::SprintPressed()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		Me->SetDesiredSprintStrength(1.f);
	}
}

void AFootballerController::SprintReleased()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		Me->SetDesiredSprintStrength(0.f);
	}
}

void AFootballerController::ShootPressed()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		Me->ShootBall(1.f, Me->GetActorForwardVector());
	}
}

void AFootballerController::PassPressed()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		Me->PassBall(0.75f, Me->GetActorForwardVector());
	}
}
