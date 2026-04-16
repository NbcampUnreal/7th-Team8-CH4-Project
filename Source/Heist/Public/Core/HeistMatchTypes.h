#pragma once

#include "CoreMinimal.h"
#include "HeistMatchTypes.generated.h"

UENUM(BlueprintType)
enum class EHeistTeam : uint8
{
	None,
	Thief,
	Police,
	Spector
};

UENUM(BlueprintType)
enum class EHeistMatchPhase : uint8
{
	None,
	Briefing,
	Execution,
	Result
};

UENUM(BlueprintType)
enum class EHeistBriefingDrawingTool : uint8
{
	Pen,
	Eraser
};

UENUM(BlueprintType)
enum class EHeistBriefingViewMode : uint8
{
	Thief,
	Police
};

UENUM(BlueprintType)
enum class EHeistBriefingStrokeVisibilityScope : uint8
{
	ThiefTeamShared,
	PolicePrivate
};

UENUM(BlueprintType)
enum class EHeistBriefingPointType : uint8
{
	ThiefInsertion,
	PoliceObjective
};

USTRUCT(BlueprintType)
struct FHeistSpawnPointData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D NormalizedPosition = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LayerId = NAME_None;

	/** 이 포인트가 어떤 선택 액션과 연결되는지 정의한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHeistBriefingPointType PointType = EHeistBriefingPointType::ThiefInsertion;

	/** 어느 팀 UI에서 이 포인트를 볼 수 있는지 정의한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHeistBriefingViewMode VisibleTo = EHeistBriefingViewMode::Thief;

	/** 레벨 내 해당 포인트 액터를 찾을 때 사용하는 Actor 태그. 서버만 참조. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WorldTag = NAME_None;
};

USTRUCT(BlueprintType)
struct FHeistBriefingSelectionCount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FHeistBriefingStrokePoint
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D NormalizedPosition = FVector2D::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RelativeTime = 0.f;
};

USTRUCT(BlueprintType)
struct FHeistBriefingStrokeStyle
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color = FLinearColor::Black;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Thickness = 2.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHeistBriefingDrawingTool Tool = EHeistBriefingDrawingTool::Pen;
};

USTRUCT(BlueprintType)
struct FHeistBriefingStroke
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid StrokeID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AuthorPlayerName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LayerId = TEXT("Outside");
	
	UPROPERTY(EditAnywhere, BlueprintREadWrite)
	FHeistBriefingStrokeStyle Style;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FHeistBriefingStrokePoint> Points;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ServerTimestamp = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRemoved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHeistBriefingStrokeVisibilityScope VisibilityScope = EHeistBriefingStrokeVisibilityScope::ThiefTeamShared;
};

USTRUCT(BlueprintType)
struct FHeistBriefingStrokePreviewChunk
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid StrokeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AuthorPlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LayerId = TEXT("Outside");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHeistBriefingStrokeStyle Style;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FHeistBriefingStrokePoint> ChunkPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHeistBriefingStrokeVisibilityScope VisibilityScope = EHeistBriefingStrokeVisibilityScope::ThiefTeamShared;
};

USTRUCT(BlueprintType)
struct FHeistBriefingPlanLayerDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName LayerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> PlanTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SortOrder = 0;
};
