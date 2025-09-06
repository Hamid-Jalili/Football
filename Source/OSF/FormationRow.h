// FormationRow.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Footballer.h" // for EFootballRole
#include "FormationRow.generated.h"

/** DataTable row: one player slot in a formation (local space, centered on pitch). */
USTRUCT(BlueprintType)
struct FFormationRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Squad order 0..N-1 (used to sort before spawning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Index = 0;

	/** Local position relative to pitch centre. X = forward to opponent goal, Y = right. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector LocalPosition = FVector::ZeroVector;

	/** Optional initial role. If you don’t want to override roles, just leave defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFootballRole Role = EFootballRole::MID;
};
