#include "GameplayComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UGameplayComponent::UGameplayComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGameplayComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGameplayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// NOTE: Removed "const" to match header declaration and fix C2511.
bool UGameplayComponent::CanKickBall(ACharacter* Character, AActor* Ball)
{
	if (!Character || !Ball)
	{
		return false;
	}

	FVector Distance = Character->GetActorLocation() - Ball->GetActorLocation();
	Distance.Z = 0.f;
	return Distance.Size() < 100.f;
}

void UGameplayComponent::MoveToBallForKick(ACharacter* Character, AActor* Ball, FVector DesiredEndDirection, float DeltaSeconds)
{
	if (!Character || !Ball)
	{
		return;
	}

	const FVector CharPos = Character->GetActorLocation();
	const FVector BallPos = Ball->GetActorLocation();

	// Rough prediction of where the ball will be by the time the character reaches it.
	FVector ToBall = BallPos - CharPos;
	ToBall.Z = 0.f;
	const float Distance = ToBall.Size();

	float MaxSpeed = 600.f;
	if (const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		MaxSpeed = MoveComp->MaxWalkSpeed;
	}

	const float TimeToTarget = (MaxSpeed > 0.f) ? (Distance / MaxSpeed) : 0.f;
	FVector PredictedBall = BallPos + Ball->GetVelocity() * TimeToTarget;
	PredictedBall.Z = CharPos.Z;

	FVector MoveDir = PredictedBall - CharPos;
	if (!MoveDir.IsNearlyZero())
	{
		MoveDir.Z = 0.f;
		MoveDir.Normalize();
		Character->AddMovementInput(MoveDir, 1.f);
	}

	// Face the current ball position
	FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(CharPos, BallPos);
	LookAt.Pitch = 0.f;
	LookAt.Roll = 0.f;
	Character->SetActorRotation(LookAt);
}
