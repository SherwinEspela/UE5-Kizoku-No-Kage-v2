#include "AnimStitchControlShapes.h"
#include "AnimStitchManualEdits.h"

#define LOCTEXT_NAMESPACE "AnimStitch"

namespace
{
	TArray<FVector> Polygon(int32 Sides, double Rotation = 0.0)
	{
		TArray<FVector> Loop;
		for (int32 i = 0; i < Sides; ++i)
		{
			const double A = Rotation + 2.0 * PI * i / Sides;
			Loop.Add(FVector(0.0, FMath::Cos(A), FMath::Sin(A)));
		}
		return Loop;
	}

	TArray<TArray<FVector>> MakeLoops(EAnimStitchControlShape Shape)
	{
		TArray<TArray<FVector>> Loops;
		switch (Shape)
		{
		case EAnimStitchControlShape::Square:
			Loops.Add({ FVector(0, -1, -1), FVector(0, 1, -1), FVector(0, 1, 1), FVector(0, -1, 1) });
			break;
		case EAnimStitchControlShape::Diamond:
			Loops.Add({ FVector(0, 0, -1), FVector(0, 1, 0), FVector(0, 0, 1), FVector(0, -1, 0) });
			break;
		case EAnimStitchControlShape::Triangle:
			Loops.Add(Polygon(3, PI * 0.5));
			break;
		case EAnimStitchControlShape::Hexagon:
			Loops.Add(Polygon(6));
			break;
		case EAnimStitchControlShape::Cross:
		{
			const double W = 0.32;
			Loops.Add({ FVector(0, -W, -1), FVector(0, W, -1), FVector(0, W, -W), FVector(0, 1, -W), FVector(0, 1, W), FVector(0, W, W),
				FVector(0, W, 1), FVector(0, -W, 1), FVector(0, -W, W), FVector(0, -1, W), FVector(0, -1, -W), FVector(0, -W, -W) });
			break;
		}
		case EAnimStitchControlShape::Circle:
		default:
			Loops.Add(Polygon(32));
			break;
		}
		return Loops;
	}
}

namespace AnimStitchControlShapes
{
	const TArray<TArray<FVector>>& GetLoops(EAnimStitchControlShape Shape)
	{
		static TArray<TArray<TArray<FVector>>> Cache;
		if (Cache.Num() == 0)
		{
			for (int32 i = 0; i <= (int32)EAnimStitchControlShape::Cross; ++i) { Cache.Add(MakeLoops((EAnimStitchControlShape)i)); }
		}
		const int32 Index = (int32)Shape;
		if (Shape == EAnimStitchControlShape::Auto || Shape == EAnimStitchControlShape::Box || Shape == EAnimStitchControlShape::Sphere || !Cache.IsValidIndex(Index))
		{
			return Cache[(int32)EAnimStitchControlShape::Circle];
		}
		return Cache[Index];
	}

	EAnimStitchControlShape DefaultFor(FName Bone, EAnimStitchTrackKind Kind)
	{
		switch (Kind)
		{
		case EAnimStitchTrackKind::IKPole: return EAnimStitchControlShape::Diamond;
		case EAnimStitchTrackKind::IKLimb: return EAnimStitchControlShape::Square;
		case EAnimStitchTrackKind::RootOffset: return EAnimStitchControlShape::Cross;
		default: break;
		}
		if (FAnimStitchManualEdits::IsPelvisBone(Bone)) { return EAnimStitchControlShape::Square; }
		const FString Lower = Bone.ToString().ToLower();
		if (Lower.Contains(TEXT("hand")) || Lower.Contains(TEXT("foot")) || Lower.Contains(TEXT("ankle"))) { return EAnimStitchControlShape::Hexagon; }
		return EAnimStitchControlShape::Circle;
	}

	EAnimStitchControlShape Resolve(const FAnimStitchControlStyle& Style, FName Bone, EAnimStitchTrackKind Kind)
	{
		if (Style.Shape == EAnimStitchControlShape::Auto || Style.Shape == EAnimStitchControlShape::Box || Style.Shape == EAnimStitchControlShape::Sphere)
		{
			return DefaultFor(Bone, Kind);
		}
		return Style.Shape;
	}

	FText GetDisplayName(EAnimStitchControlShape Shape)
	{
		switch (Shape)
		{
		case EAnimStitchControlShape::Circle: return LOCTEXT("ShapeCircle", "Circle");
		case EAnimStitchControlShape::Square: return LOCTEXT("ShapeSquare", "Square");
		case EAnimStitchControlShape::Diamond: return LOCTEXT("ShapeDiamond", "Diamond");
		case EAnimStitchControlShape::Triangle: return LOCTEXT("ShapeTriangle", "Triangle");
		case EAnimStitchControlShape::Hexagon: return LOCTEXT("ShapeHexagon", "Hexagon");
		case EAnimStitchControlShape::Cross: return LOCTEXT("ShapeCross", "Cross");
		default: return LOCTEXT("ShapeAuto", "Auto (by role)");
		}
	}
}

#undef LOCTEXT_NAMESPACE
