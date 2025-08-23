#include "Ballsack.h"
#include "EngineUtils.h"                    // TActorIterator
#include "Components/StaticMeshComponent.h" // UStaticMeshComponent

// Sets default values
ABallsack::ABallsack()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABallsack::BeginPlay()
{
	Super::BeginPlay();

	// Ensure the ball has proper physics/collision settings
	if (UStaticMeshComponent* SM = GetStaticMeshComponent())
	{
		// Physics
		SM->SetSimulatePhysics(true);
		SM->SetEnableGravity(true);

		// Collision
		SM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		SM->SetCollisionObjectType(ECC_PhysicsBody);
		SM->SetCollisionResponseToAllChannels(ECR_Block);
		SM->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);   // block players
		SM->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

		// Continuous Collision Detection to reduce tunneling through players/feet
		SM->BodyInstance.bUseCCD = true;

		// Light damping to smooth jitter but keep ball lively
		SM->SetLinearDamping(0.2f);
		SM->SetAngularDamping(0.2f);

		// Optional: clamp max angular velocity a bit (helps stability)
		SM->BodyInstance.MaxAngularVelocity = 400.f;
	}
}

// Called every frame
void ABallsack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

ABallsack* ABallsack::GetWorldBall(UWorld* World)
{
	if (!World) return nullptr;

	for (TActorIterator<ABallsack> It(World); It; ++It)
	{
		return *It; // first one found
	}
	return nullptr;
}
