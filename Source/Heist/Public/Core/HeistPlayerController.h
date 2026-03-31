#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HeistPlayerController.generated.h"

UCLASS()
class HEIST_API AHeistPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHeistPlayerController();

protected:
	virtual void PlayerTick(float DeltaTime) override;

private:
	void UpdateCursorRotation();
};
