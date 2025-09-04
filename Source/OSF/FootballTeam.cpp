#include "FootballTeam.h"

static FVector S_AnchorFromGrid(int32 TeamID, float PitchHalf, int32 Row, int32 Col, bool bAttack)
{
	static const float RowX[4] = { -0.92f, -0.60f, -0.20f, 0.20f };
	static const float ColY[4][4] =
	{
		{ 0.f, 0.f, 0.f, 0.f },
		{ -0.70f, -0.25f, 0.25f, 0.70f },
		{ -0.80f, -0.30f, 0.30f, 0.80f },
		{ -0.30f,  0.30f, 0.f,   0.f }
	};

	float X = RowX[Row] * PitchHalf;
	float Y = 0.f;
	if (Row == 1)      Y = ColY[1][Col] * PitchHalf * 0.45f;
	else if (Row == 2) Y = ColY[2][Col] * PitchHalf * 0.55f;
	else if (Row == 3) Y = ColY[3][Col] * PitchHalf * 0.30f;

	if (TeamID == 1) X = -X;
	if (bAttack)
	{
		const float Push = (TeamID == 0 ? 1.f : -1.f) * PitchHalf * 0.08f;
		X += Push;
	}
	return FVector(X, Y, 0.f);
}

FVector AFootballTeam::BaseAnchor(int32 TeamID, float PitchHalf, int32 PlayerIndex, bool bAttack)
{
	if (PlayerIndex == 0) return S_AnchorFromGrid(TeamID, PitchHalf, 0, 0, bAttack);
	if (PlayerIndex >= 1 && PlayerIndex <= 4) return S_AnchorFromGrid(TeamID, PitchHalf, 1, PlayerIndex - 1, bAttack);
	if (PlayerIndex >= 5 && PlayerIndex <= 8) return S_AnchorFromGrid(TeamID, PitchHalf, 2, PlayerIndex - 5, bAttack);
	return S_AnchorFromGrid(TeamID, PitchHalf, 3, PlayerIndex - 9, bAttack);
}

FVector AFootballTeam::GetAnchor(bool bAttacking, int32 PlayerIndex, const FVector& BallWorld) const
{
	FVector A = BaseAnchor(TeamID, PitchHalfLength, PlayerIndex, bAttacking);

	// Light “activity” based on ball position: players nearer the ball are nudged more.
	const float PitchHalf = PitchHalfLength;
	const float NormX = FMath::Clamp(BallWorld.X / (PitchHalf * 1.2f), -1.f, 1.f);
	const float NormY = FMath::Clamp(BallWorld.Y / (PitchHalf * 0.9f), -1.f, 1.f);
	const float Prox = 1.f - FMath::Clamp((FVector(A.X, A.Y, 0) - FVector(BallWorld.X, BallWorld.Y, 0)).Size() / (PitchHalf * 1.5f), 0.f, 1.f);

	const float Strafe = 350.f * Prox * NormY;                                     // drift laterally with ball
	float Push = 300.f * Prox * (TeamID == 0 ? 1.f : -1.f) * NormX;                  // drift forward/back with ball
	if (!bAttacking) Push *= -0.65f;                                               // pull deeper in defence

	A.Y += Strafe;
	A.X += Push;
	return A;
}
