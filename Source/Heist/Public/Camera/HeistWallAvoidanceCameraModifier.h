#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "HeistWallAvoidanceCameraModifier.generated.h"

/**
 * 카메라~캐릭터 사이에 벽이 감지되면 캐릭터와의 거리를 유지하면서 카메라를 수직 방향으로 올린다.
 * AHeistPlayerController::BeginPlay에서 AddNewCameraModifier로 등록한다.
 */
UCLASS(Blueprintable)
class HEIST_API UHeistWallAvoidanceCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
	// 벽 미감지 시 기본 피치
	UPROPERTY(EditDefaultsOnly, Category = "Camera|WallAvoidance")
	float DefaultPitch = -65.0f;

	// 벽에 완전히 막혔을 때 최대 피치 (수직 하향)
	UPROPERTY(EditDefaultsOnly, Category = "Camera|WallAvoidance")
	float MaxPitch = -90.0f;

	// 라인트레이스 충돌 채널
	UPROPERTY(EditDefaultsOnly, Category = "Camera|WallAvoidance")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Camera;
};
