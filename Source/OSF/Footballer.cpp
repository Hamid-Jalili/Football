// Footballer.cpp
#include "Footballer.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"

AFootballer::AFootballer()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFootballer::SetDesiredMovement(const FVector& WorldDirection)
{
	DesiredMove = WorldDirection;
}

/** Public: can be used by Team/GameMode after TeamID is known. */
void AFootballer::ApplyTeamMaterial(int32 InTeamID)
{
	// Cache TeamID if caller passed a value (we don't force if -1).
	if (InTeamID >= 0)
	{
		TeamID = InTeamID;
	}

	UMaterialInterface* Chosen = nullptr;
	if (TeamID == 0)
	{
		Chosen = Team0Material;
	}
	else if (TeamID == 1)
	{
		Chosen = Team1Material;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Footballer] No Mesh found to apply team material for %s"), *GetName());
		return;
	}

	if (Chosen)
	{
		const int32 Slots = MeshComp->GetNumMaterials();
		const int32 Slot = FMath::Clamp(TeamMaterialSlot, 0, FMath::Max(0, Slots - 1));
		MeshComp->SetMaterial(Slot, Chosen);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Footballer] No material configured for Team %d on %s (Team0Material/Team1Material not set)"), TeamID, *GetName());
	}
}

void AFootballer::SetTeamAndApply(int32 InTeamID)
{
	TeamID = InTeamID;
	ApplyTeamMaterial(InTeamID);
}

/* ======== Controller shims (compile helpers, no feature removed) ======== */

void AFootballer::SetDesiredSprintStrength(float Strength)
{
	DesiredSprintStrength = FMath::Clamp(Strength, 0.f, 1.f);
	// (Optional) UE_LOG(LogTemp, Verbose, TEXT("[Footballer] Sprint=%.2f on %s"), DesiredSprintStrength, *GetName());
}

void AFootballer::ShootBall(float Power, const FVector& Direction)
{
	// Stub to satisfy controller; real shooting is handled elsewhere.
	// (Optional) UE_LOG(LogTemp, Verbose, TEXT("[Footballer] ShootBall Power=%.2f Dir=%s"), Power, *Direction.ToString());
}

void AFootballer::PassBall(float Power, const FVector& Direction)
{
	// Stub to satisfy controller; real passing is handled elsewhere.
	// (Optional) UE_LOG(LogTemp, Verbose, TEXT("[Footballer] PassBall Power=%.2f Dir=%s"), Power, *Direction.ToString());
}
