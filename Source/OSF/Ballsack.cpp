#include "Ballsack.h"
#include "Footballer.h"                     // to read TeamID
#include "Components/PrimitiveComponent.h"

ABallsack::ABallsack()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABallsack::SetPossessingFootballer(AFootballer* Footballer)
{
	PossessingTeamID = Footballer ? Footballer->TeamID : -1;
}
