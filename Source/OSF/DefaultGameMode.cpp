#include "DefaultGameMode.h"

#include "TeamGameState.h"
#include "FootballTeam.h"
#include "Footballer.h"
#include "FootballerAIController.h"
#include "FootballerController.h"
#include "Ballsack.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

ADefaultGameMode::ADefaultGameMode()
{
	GameStateClass = ATeamGameState::StaticClass();
	DefaultPawnClass = nullptr; // we possess a Footballer
	PlayerControllerClass = AFootballerController::StaticClass();
	FootballerClass = AFootballer::StaticClass(); // fallback if not set in BP
}

static FVector AnchorFromGrid(int32 TeamID, float PitchHalf, int32 Row, int32 Col, bool bAttack)
{
	static const float RowX[4] = { -0.92f, -0.60f, -0.20f, 0.20f };
	static const float ColY[4][4] =
	{
		{ 0.f, 0.f, 0.f, 0.f },                           // GK
		{ -0.70f, -0.25f, 0.25f, 0.70f },                 // DEF
		{ -0.80f, -0.30f, 0.30f, 0.80f },                 // MID
		{ -0.30f,  0.30f, 0.f,   0.f }                    // FWD (2 used)
	};

	float X = RowX[Row] * PitchHalf;
	float Y = 0.f;
	if (Row == 1)      Y = ColY[1][Col] * PitchHalf * 0.45f;
	else if (Row == 2) Y = ColY[2][Col] * PitchHalf * 0.55f;
	else if (Row == 3) Y = ColY[3][Col] * PitchHalf * 0.30f;

	// Mirror for right team
	if (TeamID == 1) X = -X;

	// Small push toward attack
	if (bAttack)
	{
		const float Push = (TeamID == 0 ? 1.f : -1.f) * PitchHalf * 0.08f;
		X += Push;
	}

	return FVector(X, Y, 0.f);
}

static FVector FormationAnchor(int32 TeamID, float PitchHalf, int32 PlayerIndex, bool bAttack)
{
	if (PlayerIndex == 0) return AnchorFromGrid(TeamID, PitchHalf, 0, 0, bAttack); // GK
	if (PlayerIndex >= 1 && PlayerIndex <= 4) return AnchorFromGrid(TeamID, PitchHalf, 1, PlayerIndex - 1, bAttack);
	if (PlayerIndex >= 5 && PlayerIndex <= 8) return AnchorFromGrid(TeamID, PitchHalf, 2, PlayerIndex - 5, bAttack);
	return AnchorFromGrid(TeamID, PitchHalf, 3, PlayerIndex - 9, bAttack);            // 9..10
}

void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();
	BuildTeamsFromWorld();
}

void ADefaultGameMode::BuildTeamsFromWorld()
{
	ATeamGameState* TGS = Cast<ATeamGameState>(GameState);
	if (!TGS) return;

	// Create teams
	AFootballTeam* TeamA = GetWorld()->SpawnActor<AFootballTeam>();
	AFootballTeam* TeamB = GetWorld()->SpawnActor<AFootballTeam>();
	TeamA->TeamID = 0; TeamB->TeamID = 1;
	TeamA->PitchHalfLength = PitchHalfLength;
	TeamB->PitchHalfLength = PitchHalfLength;

	// Gather any pre-placed footballers (BP or native)
	TArray<AFootballer*> All;
	for (TActorIterator<AFootballer> It(GetWorld()); It; ++It) All.Add(*It);

	// If none, auto-spawn a full 11v11 using FootballerClass
	if (All.Num() == 0)
	{
		const float z = 100.f;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		UClass* SpawnCls = FootballerClass ? *FootballerClass : AFootballer::StaticClass();

		for (int32 i = 0; i < PlayersPerTeam; ++i)
		{
			// Left/Blue
			{
				const FVector Pos = FormationAnchor(0, PitchHalfLength, i, true);
				const FTransform T(FRotator::ZeroRotator, FVector(Pos.X, Pos.Y, z));
				if (AFootballer* P = GetWorld()->SpawnActor<AFootballer>(SpawnCls, T, Params)) All.Add(P);
			}
			// Right/Red
			{
				const FVector Pos = FormationAnchor(1, PitchHalfLength, i, true);
				const FTransform T(FRotator::ZeroRotator, FVector(Pos.X, Pos.Y, z));
				if (AFootballer* P = GetWorld()->SpawnActor<AFootballer>(SpawnCls, T, Params)) All.Add(P);
			}
		}
	}

	// Deterministic order
	All.Sort([](const AFootballer& A, const AFootballer& B)
		{
			if (A.GetActorLocation().Y == B.GetActorLocation().Y)
				return A.GetActorLocation().X < B.GetActorLocation().X;
			return A.GetActorLocation().Y < B.GetActorLocation().Y;
		});

	// Partition by half
	int32 aIdx = 0, bIdx = 0;
	for (AFootballer* P : All)
	{
		if (!P) continue;
		const bool bLeftSide = (P->GetActorLocation().X < 0.f);
		AFootballTeam* Team = bLeftSide ? TeamA : TeamB;

		P->TeamRef = Team;
		P->PlayerIndex = bLeftSide ? aIdx++ : bIdx++;
		Team->Players.Add(P);

		P->ApplyTeamMaterial();
		EnsureAIPossession(P);
	}
	if (TeamA->Players.Num() > PlayersPerTeam) TeamA->Players.SetNum(PlayersPerTeam);
	if (TeamB->Players.Num() > PlayersPerTeam) TeamB->Players.SetNum(PlayersPerTeam);

	// Register with GameState
	TGS->Teams.Empty();
	TGS->Teams.Add(TeamA);
	TGS->Teams.Add(TeamB);

	// Decide kickoff team by nearest-to-ball
	ABallsack* Ball = Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass()));
	int32 StartTeam = 0;
	if (Ball)
	{
		auto NearSq = [&](AFootballTeam* T)->double
			{
				double Best = TNumericLimits<double>::Max();
				for (AFootballer* P : T->Players)
					if (P) Best = FMath::Min(Best, FVector::DistSquared(P->GetActorLocation(), Ball->GetActorLocation()));
				return Best;
			};
		StartTeam = (NearSq(TeamA) <= NearSq(TeamB)) ? 0 : 1;
	}
	TGS->SetPossessingTeamID(StartTeam);

	// Possess the human team (Blue=0 / Red=1) – pick CM (index 6) or nearest to centre
	AFootballTeam* HumanTeam = (HumanTeamID == 0) ? TeamA : TeamB;
	AFootballer* Best = nullptr;
	auto FindIdx = [](AFootballTeam* T, int32 I)->AFootballer*
		{
			if (!T) return nullptr;
			for (AFootballer* P : T->Players) if (P && P->PlayerIndex == I) return P;
			return nullptr;
		};
	Best = FindIdx(HumanTeam, 6);
	if (!Best)
	{
		double BestDist = TNumericLimits<double>::Max();
		for (AFootballer* P : HumanTeam->Players)
		{
			if (!P) continue;
			const double D = FMath::Abs((double)P->GetActorLocation().X);
			if (D < BestDist) { BestDist = D; Best = P; }
		}
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (Best) { PC->Possess(Best); PC->SetViewTarget(Best); }
	}
}

void ADefaultGameMode::EnsureAIPossession(AFootballer* P)
{
	if (!P) return;
	if (Cast<APlayerController>(P->GetController())) return; // human will possess later

	if (P->AIControllerClass != AFootballerAIController::StaticClass())
	{
		P->AIControllerClass = AFootballerAIController::StaticClass();
	}
	P->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (!P->GetController())
	{
		if (AFootballerAIController* AI = GetWorld()->SpawnActor<AFootballerAIController>())
		{
			AI->Possess(P);
		}
	}
}
