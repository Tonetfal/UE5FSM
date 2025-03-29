// Author: Antonio Sidenko (Tonetfal). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "NativeGameplayTags.h"

UE5FSM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateMachine_Label_Test);

class FUE5FSMModule
    : public IModuleInterface
{
public:
    //~IModuleInterface Interface
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    //~End of IModuleInterface Interface
};
