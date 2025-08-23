#include "NewActorComponent.h"

UNewActorComponent::UNewActorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNewActorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNewActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
