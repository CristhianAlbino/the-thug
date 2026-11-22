#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TTKitRefs.generated.h"

/**
 * Hard references to the third-party art the game loads by path (Scripts/build_kit.py fills
 * /Game/Kit/DA_TTKitRefs). Cooking /Game/Kit follows these, so only the used pieces are packaged.
 */
UCLASS()
class THETHUG_API UTTKitRefs : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Kit")
	TArray<TObjectPtr<UObject>> Assets;
};
