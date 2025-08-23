#include "Ballsack.h"            // <-- MUST be FIRST
#include "MyActorComponent.h"

UMyActorComponent::UMyActorComponent()
{
    PrimaryComponentTick.bCanEverTick = true; // replaces old patterns
}

void UMyActorComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UMyActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
