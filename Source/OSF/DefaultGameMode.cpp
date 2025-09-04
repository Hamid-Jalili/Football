#include "DefaultGameMode.h"                // must be first
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"                    // TActorIterator
#include "GameFramework/PlayerController.h"

#include "Footballer.h"                     // real type & EFootballRole
#include "FootballTeam.h"

#define LOG(Category, Fmt, ...) UE_LOG(LogTemp, Category, TEXT("[GM] " Fmt), ##__VA_ARGS__)

// --------------------------

ADefaultGameMode::ADefaultGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Ensure pawn class (auto-load BP_Footballer if not set in BP_GameMode)
	if (!FootballerClass)
	{
		if (UClass* Loaded = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/BP_Footballer.BP_Footballer_C")))
		{
			FootballerClass = Loaded;
		}
		else
		{
			LOG(Error, "FootballerClass is null and /Game/BP_Footballer not found. Aborting spawn.");
			return;
		}
	}

	const int32 Count = FMath::Clamp(PlayersPerTeam, 1, 11);

	AFootballTeam* TeamA = EnsureTeamActor(0);
	AFootballTeam* TeamB = EnsureTeamActor(1);
	if (!TeamA || !TeamB)
	{
		LOG(Error, "Failed to ensure team actors.");
		return;
	}

	TArray<AFootballer*> TeamAPlayers, TeamBPlayers;
	TeamAPlayers.Reserve(Count);
	TeamBPlayers.Reserve(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		SpawnOne(0, i, TeamA, TeamAPlayers);
		SpawnOne(1, i, TeamB, TeamBPlayers);
	}

	LOG(Display, "Spawned TeamA=%d, TeamB=%d.", TeamAPlayers.Num(), TeamBPlayers.Num());

	// Possess human
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		AFootballer* ToPossess =
			(HumanTeamID == 0 && TeamAPlayers.IsValidIndex(5)) ? TeamAPlayers[5] :
			(HumanTeamID == 1 && TeamBPlayers.IsValidIndex(5)) ? TeamBPlayers[5] :
			(HumanTeamID == 0 ? (TeamAPlayers.Num() ? TeamAPlayers[0] : nullptr)
				: (TeamBPlayers.Num() ? TeamBPlayers[0] : nullptr));

		if (ToPossess)
		{
			PC->Possess(ToPossess);
			LOG(Display, "PlayerController possessed %s (Team %d).", *ToPossess->GetName(), HumanTeamID);
		}
		else
		{
			LOG(Warning, "Could not find a pawn to possess for human team %d.", HumanTeamID);
		}
	}

	// Give AI to the non-human team
	auto GiveAI = [](const TArray<AFootballer*>& Guys)
		{
			for (AFootballer* P : Guys)
			{
				if (P && !P->GetController())
				{
					P->SpawnDefaultController();
				}
			}
		};

	if (HumanTeamID == 0) { GiveAI(TeamBPlayers); }
	else { GiveAI(TeamAPlayers); }
}

// --------------------------

AFootballTeam* ADefaultGameMode::EnsureTeamActor(int32 TeamID)
{
	const FName Wanted = (TeamID == 0) ? FName(TEXT("FootballTeam0")) : FName(TEXT("FootballTeam1"));

	// Try to find a placed team first
	for (TActorIterator<AFootballTeam> It(GetWorld()); It; ++It)
	{
		if (It->GetFName() == Wanted)
		{
			return *It;
		}
	}

	// Otherwise spawn a simple one
	FActorSpawnParameters Params;
	Params.Name = Wanted;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFootballTeam* NewTeam = GetWorld()->SpawnActor<AFootballTeam>(
		AFootballTeam::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

	if (NewTeam)
	{
		NewTeam->TeamID = TeamID;
	}
	return NewTeam;
}

FVector ADefaultGameMode::FormationHome(int32 TeamID, int32 Index) const
{
	// 4-4-2 style (index 0 GK, 1..4 DEF, 5..8 MID, 9..10 FWD)
	static const float LineX[11] = { -7600, -6000, -6000, -6000, -6000, -2000, -2000, -2000, -2000,  1200,  1800 };
	static const float LineY[11] = { 0, -2400,  -800,   800,  2400, -2400,  -800,   800,  2400, -1000,  1000 };

	const int32 I = FMath::Clamp(Index, 0, 10);
	FVector L(LineX[I], LineY[I], 120.f);

	// Mirror X for right team and face the other way (we keep Y to keep symmetry across midline)
	if (TeamID == 1)
	{
		L.X *= -1.f;
	}
	return L;
}

EFootballRole ADefaultGameMode::RoleForIndex(int32 Index)
{
	switch (Index)
	{
	case 0:  return EFootballRole::GK;
	case 1:
	case 2:
	case 3:
	case 4:  return EFootballRole::DEF;
	case 5:
	case 6:
	case 7:
	case 8:  return EFootballRole::MID;
	default: return EFootballRole::FWD;
	}
}

void ADefaultGameMode::SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutList)
{
	if (!FootballerClass) return;

	const FVector Home = FormationHome(TeamID, Index);
	const FRotator Face = (TeamID == 0) ? FRotator(0.f, 0.f, 0.f)
		: FRotator(0.f, 180.f, 0.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFootballer* P = GetWorld()->SpawnActor<AFootballer>(FootballerClass, Home, Face, Params);
	if (!P) return;

	// Basic initialization – these members are on your AFootballer
	P->TeamID = TeamID;
	P->PlayerRole = RoleForIndex(Index);
	P->HomeLocation = Home;

	if (TeamActor)
	{
		TeamActor->RegisterPlayer(P, Index);
	}

	// AI for non-human team member (human team will be possessed by PlayerController)
	if (TeamID != HumanTeamID && !P->GetController())
	{
		P->SpawnDefaultController();
	}

	OutList.Add(P);
}
