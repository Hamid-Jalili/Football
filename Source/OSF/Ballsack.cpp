#include "Ballsack.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Footballer.h"
#include "FootballTeam.h"

ABallsack::ABallsack()
{
    PrimaryActorTick.bCanEverTick = true;

    TouchCollision = CreateDefaultSubobject<USphereComponent>(TEXT("TouchCollision"));
    TouchCollision->InitSphereRadius(40.f);
    TouchCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    TouchCollision->SetCollisionResponseToAllChannels(ECR_Block);
    TouchCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TouchCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    RootComponent = TouchCollision;

    TouchCollision->OnComponentBeginOverlap.AddDynamic(this, &ABallsack::OnBallBeginOverlap);
}

void ABallsack::BeginPlay()
{
    Super::BeginPlay();

    if (!TouchCollision->IsSimulatingPhysics())
    {
        TouchCollision->SetSimulatePhysics(true);
        TouchCollision->SetEnableGravity(true);
        TouchCollision->SetLinearDamping(0.02f);
        TouchCollision->SetAngularDamping(0.3f);
        TouchCollision->BodyInstance.bUseCCD = true;
    }
}

void ABallsack::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ABallsack::ResetToCenter(const FVector& CentreWorld)
{
    if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(TouchCollision))
    {
        Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
    SetActorLocation(CentreWorld, false, nullptr, ETeleportType::ResetPhysics);
}

void ABallsack::OnBallBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (AFootballer* Footballer = Cast<AFootballer>(OtherActor))
    {
        LastTouchingFootballer = Footballer;
        PossessingTeamID = (Footballer->TeamRef ? Footballer->TeamRef->TeamID : -1);
    }
}
