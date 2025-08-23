// TeamGameState.cpp — UE5.3-friendly

#include "TeamGameState.h"              // must be first
#include "Ballsack.h"
#include "Footballer.h"
#include "FootballTeam.h"

#include "UObject/ConstructorHelpers.h" // FClassFinder / FObjectFinder
#include "Materials/MaterialInstanceConstant.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"          // DOREPLIFETIME

// File-scope statics
static TSubclassOf<AFootballer> GFootballerBPClass = nullptr;
static UMaterialInstanceConstant* GOrangeMI = nullptr;

ATeamGameState::ATeamGameState()
{
	// Find BPFootballer (expects the Blueprint asset to live at /Game/Blueprints/BPFootballer)
	{
		static ConstructorHelpers::FClassFinder<AFootballer> BPClass(TEXT("/Game/Blueprints/BPFootballer"));
		if (BPClass.Succeeded())
		{
			GFootballerBPClass = BPClass.Class;
		}
	}

	// Find orange material instance
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInstanceConstant> Mat(
			TEXT("/Game/Materials/MI_Template_BaseOrange.MI_Template_BaseOrange"));
		if (Mat.Succeeded())
		{
			GOrangeMI = Mat.Object;
		}
	}
}

void ATeamGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATeamGameState, HomeTeam);
	DOREPLIFETIME(ATeamGameState, AwayTeam);
	DOREPLIFETIME(ATeamGameState, Ball);
}

void ATeamGameState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// Your server-side init here (left as in your original)
	}
}

/* ======= Everything below mirrors your original logic, but swaps
		   footballerClass -> GFootballerBPClass and
		   orangeMesh      -> GOrangeMI                                 ======= */

struct FFootballerInfo { FString Name; };

struct FPosition
{
	int IndexInFormation;
	FString Name;
	FVector2D Location; // X=0 goal line .. 1 midfield; Y=-1 left flank .. 1 right flank
};

struct FFormation
{
	FString Name;
	FPosition Positions[11];
};

static FFormation Formation4231 =
{
	TEXT("4-2-3-1"),
	{
		{ 0,  "GK",  { 0.0, 0.5 } },

		{ 1,  "RB",  { 0.3, 1 } },
		{ 2,  "RCB", { 0.2, 0.75 } },
		{ 3,  "LCB", { 0.2, 0.25 } },
		{ 4,  "LB",  { 0.3, 0 } },

		{ 5,  "RDM", { 0.45, 0.75 } },
		{ 6,  "LDM", { 0.55, 0.25 } },

		{ 7,  "RAM", { 0.8, 1 } },
		{ 8,  "CAM", { 0.7, 0.6 } },
		{ 9,  "LAM", { 0.8, 0 } },

		{ 10, "CF",  { 1, 0.4 } },
	}
};

static FFormation FormationDebug =
{
	TEXT("D-E-B-U-G"),
	{
		{ 0,  "GK",  { 0.1,  0 } },

		{ 1,  "RB",  { 0.3,  0.8 } },
		{ 2,  "RCB", { 0.3, -0.5 } },
		{ 3,  "LCB", { 0.3, -0.5 } },
		{ 4,  "LB",  { 0.3, -0.8 } },

		{ 5,  "RDM", { 0.5,  0.5 } },
		{ 6,  "LDM", { 0.5, -0.5 } },

		{ 7,  "RAM", { 0.8,  0.8 } },
		{ 8,  "CAM", { 0.8,  0 } },
		{ 9,  "LAM", { 0.8, -0.8 } },

		{ 10, "CF",  { 1,    0.4 } },
	}
};

void ATeamGameState::LoadSampleState()
{
	static const int32 NUM_PLAYERS = 11;

	static FFootballerInfo HomePlayers[11] =
	{
		{ TEXT("Szczczczczczcz") },

		{ TEXT("Douche") },
		{ TEXT("BFG") },
		{ TEXT("Gabby") },
		{ TEXT("Nachos") },

		{ TEXT("Cock") },
		{ TEXT("Santa Gorgonzola") },

		{ TEXT("Sign Da Ting") },
		{ TEXT("Tattoo Haram") },
		{ TEXT("Wiz Khalifa - No Sleep") },

		{ TEXT("Goat Cheese") }
	};

	static FFootballerInfo AwayPlayers[11] =
	{
		{ TEXT("Newer") },

		{ TEXT("Lame") },
		{ TEXT("Boatingle") },
		{ TEXT("Artines") },
		{ TEXT("God") },

		{ TEXT("Shabby") },
		{ TEXT("Vidal Sassoon") },

		{ TEXT("Robbery I") },
		{ TEXT("Giraffe") },
		{ TEXT("Robbery II") },

		{ TEXT("Lrzwrzszcszczewski") }
	};

	for (int32 i = 0; i < NUM_PLAYERS; ++i)
	{
		const FFootballerInfo Home = HomePlayers[i];
		const FFootballerInfo Away = AwayPlayers[i];

		AFootballer* HomeFootballer = PlayerFromProperties(HomeTeam, i, Home.Name);
		if (HomeFootballer && GOrangeMI)
		{
			if (UMeshComponent* Mesh = HomeFootballer->GetMesh())
			{
				Mesh->SetMaterial(0, GOrangeMI);
			}
		}

		if (HomeTeam) { HomeTeam->Footballers.Add(HomeFootballer); }
		if (AwayTeam) { AwayTeam->Footballers.Add(PlayerFromProperties(AwayTeam, i, Away.Name)); }
	}
}

static float HalfFieldWidth = 2750.f;
static float HalfFieldLength = 4755.f;

AFootballer* ATeamGameState::PlayerFromProperties(AFootballTeam* Team, int32 Index, const FString Name)
{
	const FPosition Pos = FormationDebug.Positions[Index];

	FVector Location(0, 0, 100);

	// X: 0 goalie … 1 striker; Away is +X, Home is -X.
	float CorrectX = 1.f - Pos.Location.X;
	if (Team == HomeTeam) { CorrectX *= -1.f; }

	Location.X = CorrectX * HalfFieldLength * 0.9f;
	Location.Y = Pos.Location.Y * HalfFieldWidth * 0.9f;

	FTransform SpawnTM;
	SpawnTM.SetLocation(Location);

	AActor* Spawned = nullptr;
	if (GFootballerBPClass)
	{
		Spawned = GetWorld()->SpawnActor(GFootballerBPClass, &SpawnTM);
	}
	else
	{
		// Fallback: spawn native class if BP not found
		Spawned = GetWorld()->SpawnActor<AFootballer>(AFootballer::StaticClass(), SpawnTM);
	}

	AFootballer* Footballer = Cast<AFootballer>(Spawned);
	if (Footballer)
	{
		Footballer->DisplayName = Name;
		Footballer->Team = Team;
		Footballer->SetActorLocation(Location);
	}
	return Footballer;
}
