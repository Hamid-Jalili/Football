#include "Ballsack.h"            // <-- MUST be FIRST
#include "OSF.h"

ABallsack::ABallsack()
{
    PrimaryActorTick.bCanEverTick = true;

    BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
    SetRootComponent(BallMesh);

    // Enable physics
    BallMesh->SetSimulatePhysics(true);
    BallMesh->SetNotifyRigidBodyCollision(true);

    // Set collision
    BallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BallMesh->SetCollisionObjectType(ECC_PhysicsBody);
    BallMesh->SetCollisionResponseToAllChannels(ECR_Block);

    // Optional tuning
    BallMesh->SetLinearDamping(0.15f);
    BallMesh->SetAngularDamping(0.3f);
}

void ABallsack::BeginPlay()
{
    Super::BeginPlay();
}

void ABallsack::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}
