#include "TestCppComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

UTestCppComponent::UTestCppComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTestCppComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTestCppComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UTestCppComponent::MoveAlongSpiralPath(ACharacter* Character, AActor* Target, FVector DesiredEndDirection, float DeltaTime)
{
	if (!Character || !Target) return;

	// Simple circular motion around the target as a placeholder
	const float Radius = 100.f;
	const float Speed = 1.f;
	const float Angle = GetWorld() ? GetWorld()->GetTimeSeconds() * Speed : 0.f;

	const FVector Center = Target->GetActorLocation();
	const FVector Offset = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
	Character->SetActorLocation(Center + Offset);
}
