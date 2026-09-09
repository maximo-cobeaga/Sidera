#include "Presentation/AstraeonBodyAnimation.h"

namespace AstraeonBodyAnimation
{
	constexpr float StandingSpeedCms = 10.0f;
	constexpr float RunSpeedThresholdCms = 620.0f;

	const FName Idle(TEXT("Idle"));
	const FName JumpStart(TEXT("Jump_Start"));
	const FName JumpLoop(TEXT("Jump_Loop"));
	const FName JumpLand(TEXT("Jump_Land"));

	// El eje dominante decide la dirección. Con la diagonal exacta gana el avance, que es
	// lo que el jugador siente que está haciendo cuando pulsa dos teclas.
	FName DirectionalSuffix(const FVector2D& LocalDirection)
	{
		if (FMath::Abs(LocalDirection.X) >= FMath::Abs(LocalDirection.Y))
		{
			return LocalDirection.X >= 0.0f ? FName(TEXT("F")) : FName(TEXT("B"));
		}
		return LocalDirection.Y >= 0.0f ? FName(TEXT("R")) : FName(TEXT("L"));
	}
}

float FAstraeonBodyAnimation::GetStandingSpeedCms()
{
	return AstraeonBodyAnimation::StandingSpeedCms;
}

float FAstraeonBodyAnimation::GetRunSpeedThresholdCms()
{
	return AstraeonBodyAnimation::RunSpeedThresholdCms;
}

FName FAstraeonBodyAnimation::GetClipIdForAction(EAstraeonBodyAction Action)
{
	switch (Action)
	{
	case EAstraeonBodyAction::Scan: return FName(TEXT("Scan"));
	case EAstraeonBodyAction::Interact: return FName(TEXT("Interact"));
	case EAstraeonBodyAction::Pickup: return FName(TEXT("Pickup"));
	case EAstraeonBodyAction::ToolUse: return FName(TEXT("Tool_Use"));
	case EAstraeonBodyAction::Consume: return FName(TEXT("UseTool"));
	// Presentar el resonador es enseñar algo, no usarlo: el clip de inspección es el que
	// levanta el objeto a la vista.
	case EAstraeonBodyAction::Present: return FName(TEXT("Inspect"));
	default: return NAME_None;
	}
}

FAstraeonBodyClip FAstraeonBodyAnimation::Choose(const FAstraeonBodyAnimationState& State)
{
	using namespace AstraeonBodyAnimation;

	FAstraeonBodyClip Result;

	// Una acción tapa la locomoción, pero no el aire: caer mientras se escanea tiene que
	// verse como caer, o el personaje flota en pose de escaneo.
	if (!State.bFalling && State.ActionSecondsRemaining > 0.0f)
	{
		const FName ActionClip = GetClipIdForAction(State.Action);
		if (!ActionClip.IsNone())
		{
			Result.ClipId = ActionClip;
			Result.bLoop = false;
			return Result;
		}
	}

	if (State.bFalling)
	{
		// El despegue es corto y sólo se ve al principio del salto; después manda la caída.
		Result.ClipId = State.TakeoffSecondsRemaining > 0.0f ? JumpStart : JumpLoop;
		Result.bLoop = Result.ClipId == JumpLoop;
		return Result;
	}

	if (State.LandingSecondsRemaining > 0.0f)
	{
		Result.ClipId = JumpLand;
		Result.bLoop = false;
		return Result;
	}

	if (State.SpeedCms < StandingSpeedCms)
	{
		Result.ClipId = Idle;
		Result.bLoop = true;
		return Result;
	}

	const FString Prefix = State.SpeedCms >= RunSpeedThresholdCms ? TEXT("Run_") : TEXT("Walk_");
	Result.ClipId = FName(*(Prefix + DirectionalSuffix(State.LocalDirection).ToString()));
	Result.bLoop = true;
	return Result;
}

TArray<FName> FAstraeonBodyAnimation::GetAllClipIds()
{
	TArray<FName> Ids = {
		AstraeonBodyAnimation::Idle,
		AstraeonBodyAnimation::JumpStart,
		AstraeonBodyAnimation::JumpLoop,
		AstraeonBodyAnimation::JumpLand
	};
	for (const TCHAR* Prefix : { TEXT("Walk_"), TEXT("Run_") })
	{
		for (const TCHAR* Suffix : { TEXT("F"), TEXT("B"), TEXT("L"), TEXT("R") })
		{
			Ids.Add(FName(*(FString(Prefix) + Suffix)));
		}
	}
	for (const EAstraeonBodyAction Action : { EAstraeonBodyAction::Scan, EAstraeonBodyAction::Interact,
		EAstraeonBodyAction::Pickup, EAstraeonBodyAction::ToolUse, EAstraeonBodyAction::Consume,
		EAstraeonBodyAction::Present })
	{
		Ids.Add(GetClipIdForAction(Action));
	}
	return Ids;
}
