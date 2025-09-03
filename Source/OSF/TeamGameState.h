#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TeamGameState.generated.h"

UCLASS()
class OSF_API ATeamGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATeamGameState();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Refs")
	class ABallsack* Ball = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Refs")
	TArray<class AFootballTeam*> Teams;

	/** 0 TeamA, 1 TeamB, -1 None */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Possession")
	int32 PossessingTeamID = -1;

	UFUNCTION(BlueprintCallable, Category = "Possession")
	int32 GetPossessingTeamID() const { return PossessingTeamID; }

protected:
	virtual void BeginPlay() override;

private:
	void RefreshTeamsIfEmpty();
};
