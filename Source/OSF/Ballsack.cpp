#include "Ballsack.h"
#include "Components/SphereComponent.h"
#include "Footballer.h"
#include "FootballTeam.h"

ABallsack::ABallsack()
{
	PrimaryActorTick.bCanEverTick = true;

	TouchCollision = CreateDefaultSubobject<USphereComponent>(TEXT("TouchCollision"));
	TouchCollision->InitSphereRadius(40.f);
	TouchCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TouchCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
	RootComponent = TouchCollision;

	TouchCollision->OnComponentBeginOverlap.AddDynamic(this, &ABallsack::OnBallBeginOverlap);

	// If you want physics kicks to move the ball, enable physics on the root:
	// TouchCollision->SetSimulatePhysics(true);
	// TouchCollision->SetEnableGravity(true);
}

void ABallsack::BeginPlay()
{
	Super::BeginPlay();
}

void ABallsack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABallsack::OnBallBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AFootballer* Footballer = Cast<AFootballer>(OtherActor))
	{
		LastTouchingFootballer = Footballer;
		PossessingTeamID = (Footballer->TeamRef ? Footballer->TeamRef->TeamID : -1);
	}
}
