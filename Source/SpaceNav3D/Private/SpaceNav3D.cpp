// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SpaceNav3DPrivatePCH.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpaceNav3DController, Log, All);

#if PLATFORM_WINDOWS

class FSpaceNav3DController;

class FSpaceNav3DMessageHandler
	: public IWindowsMessageHandler
{
public:
	FSpaceNav3DController* controller;
	virtual bool ProcessMessage(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam, int32& OutResult) override;
};

class FSpaceNav3DController : public IInputDevice
{
public:

	FSpaceNav3DController(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
		: m_DevHdl(NULL), bNewEvent(false), MessageHandler(InMessageHandler)
	{
		UE_LOG(LogSpaceNav3DController, Display, TEXT("Input Device creation"));
		// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
		SiOpenData oData;                        //OS Independent data to open ball  

		if (SiInitialize() == SPW_DLL_LOAD_ERROR)   //init the 3DxWare input library
		{
			UE_LOG(LogSpaceNav3DController, Error, TEXT("Could not load SiAppDll dll files"));
			return;
		}

		m_hwnd = FWindowsPlatformMisc::GetTopLevelWindowHandle(GetCurrentProcessId());
		if (m_hwnd == NULL)
		{
			SiTerminate();  //called to shut down the 3DxWare input library 
			UE_LOG(LogSpaceNav3DController, Error, TEXT("Could not find UE4's HWND handle"));
			return;
		}
		UE_LOG(LogSpaceNav3DController, Display, TEXT("UE4 hwnd = %x"), m_hwnd);

		SiOpenWinInit(&oData, m_hwnd);          //init Win. platform specific data  
		SiSetUiMode(&m_DevHdl, SI_UI_ALL_CONTROLS); //Config SoftButton Win Display 

		//open data, which will check for device type and return the device handle
		// to be used by this function  
		if ((m_DevHdl = SiOpen("FSpaceNav3DModule", SI_ANY_DEVICE, SI_NO_MASK,
			SI_EVENT, &oData)) == NULL)
		{
			SiTerminate();  //called to shut down the 3DxWare input library
			UE_LOG(LogSpaceNav3DController, Error, TEXT("Could not open the Space Navigator"));
			return;
		}

		FMemory::Memzero(&ControllerState, sizeof(ControllerState));
		FMemory::Memzero(&PrevControllerState, sizeof(PrevControllerState));

		mouse_handler.controller = this;
		TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
		FWindowsApplication* WindowsApplication = (FWindowsApplication*)GenericApplication.Get();
		WindowsApplication->AddMessageHandler(mouse_handler);

	}
#if 0 // seems to crash for some reason
	~FSpaceNav3DController()
	{
		if (m_DevHdl)
		{
			TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
			FWindowsApplication* WindowsApplication = (FWindowsApplication*)GenericApplication.Get();
			WindowsApplication->RemoveMessageHandler(mouse_handler);
			SiTerminate();  //called to shut down the 3DxWare input library 
		}
	}
#endif
	virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		MessageHandler = InMessageHandler;
	}
	virtual void Tick(float DeltaTime) override
	{
	}

	float AdjustedControllerValue(const float controllerValueInitial, const float SMDeadZone, const float linearPercent, const float multiplication)
	{
		float DeadZone = 0.2f - SMDeadZone; //UE4 deadzone minus the desired deadzone
		float controllerValue;
		controllerValue = (((controllerValueInitial > 0) ? 1.0f : -1.0f) * controllerValueInitial * controllerValueInitial * (1.0f - linearPercent) + controllerValueInitial * linearPercent) * multiplication / 2.0f;
		if (controllerValueInitial > SMDeadZone)
			controllerValue = controllerValue + DeadZone;
		else if (controllerValueInitial < -SMDeadZone)
			controllerValue = controllerValue - DeadZone;
		return controllerValue;
	}

	virtual void SendControllerEvents() override
	{
		if (bNewEvent) {
			bNewEvent = false;
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftAnalogX, 0, AdjustedControllerValue(ControllerState.LeftAnalogX, 0.05f, 0.3f, 1.0f));
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftAnalogY, 0, AdjustedControllerValue(ControllerState.LeftAnalogY, 0.05f, 0.3f, 1.0f));
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::RightAnalogX, 0, AdjustedControllerValue(ControllerState.RightAnalogX, 0.05f, 0.6f, 1.5f)); //I prefer it more linear
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::RightAnalogY, 0, AdjustedControllerValue(ControllerState.RightAnalogY, 0.05f, 0.3f, 1.0f));
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftTriggerAnalog, 0, AdjustedControllerValue(ControllerState.LeftTriggerAnalog, 0.05f, 0.3f, 1.0f));
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::RightTriggerAnalog, 0, AdjustedControllerValue(ControllerState.RightTriggerAnalog, 0.05f, 0.3f, 1.0f));
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::MotionController_Left_Thumbstick_X, 0, ControllerState.RollAnalog); // roll

			// buttons
#if WITH_EDITOR
			UEditorEngine *EEngine = Cast<UEditorEngine>(GEngine);
			FEditorViewportClient* ViewportClient = NULL;
			int32 ViewIndex;
			for (ViewIndex = 0; ViewIndex < EEngine->GetAllViewportClients().Num(); ++ViewIndex)
			{
				ViewportClient = EEngine->GetAllViewportClients()[ViewIndex];
				if (ViewportClient && ViewportClient->Viewport->HasFocus()) {
					//UE_LOG(LogSpaceNav3DController, Display, TEXT("Viewport %d has focus"), ViewIndex);
					break;
				}
			}
					
			if ( (ViewIndex < EEngine->GetAllViewportClients().Num()) && ViewportClient) {
				const FUIAction* Action = NULL;
				switch (ControllerState.cmd) {
				case V3DCMD_VIEW_FIT:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().FocusViewportToSelection);
					break;
				case V3DCMD_VIEW_FRONT:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Front);
					break;
				case V3DCMD_VIEW_BACK:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Back);
					break;
				case V3DCMD_VIEW_TOP:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Top);
					break;
				case V3DCMD_VIEW_LEFT:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Left);
					break;
				case V3DCMD_VIEW_RIGHT:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Right);
					break;
				case V3DCMD_VIEW_BOTTOM:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Bottom);
					break;
				case V3DCMD_VIEW_ISO1:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Perspective);
					break;
				case V3DCMD_VIEW_ISO2:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Perspective);
					if (Action)
						Action->Execute();
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().LitMode);
					//Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Next);
				case V3DCMD_SCALE_PLUS:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Perspective);
					break;
				case V3DCMD_SCALE_MINUS:
					Action = ViewportClient->GetEditorViewportWidget()->GetCommandList()->GetActionForCommand(FEditorViewportCommands::Get().Perspective);
					break;
				default:
					break;
				}
				if (Action)
					Action->Execute();

				// reset the button states
				ControllerState.cmd = 0;
				ControllerState.button_pressed = 0;
				ControllerState.button_released = 0;
			}
#endif // WITH_EDITOR
		}
	}


	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override { return false; }

	/**
	* IForceFeedbackSystem pass through functions
	*/
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override {}
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues &values) override {}

public:
	HWND m_hwnd;		// UE main window
	SiHdl m_DevHdl;		// Handle to 3D mouse Device
	bool bNewEvent;

	struct F3DState {
		float LeftAnalogX;
		float LeftAnalogY;
		float RightAnalogX;
		float RightAnalogY;
		float LeftTriggerAnalog;
		float RightTriggerAnalog;
		float RollAnalog;
		int cmd;
		// 3D connexion SDK buttons, 0 = none
		int   button_pressed;
		int   button_released;
	} ControllerState, PrevControllerState;

	FSpaceNav3DMessageHandler mouse_handler; // 3D mouse events sent to UE window

	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;
};

static float LongToNormalizedFloat(long AxisVal)
{
	// normalize [-32768..32767] -> [-1..1]
	const float Norm = (AxisVal <= 0 ? 2100.f : 2100.f);
	float prenorm = float(AxisVal) / Norm;

	if (prenorm == 0.0)
		return 0.0;

	prenorm *= 0.8;
#if WITH_EDITOR
	UWorld* world = GEditor->GetEditorWorldContext().World();
	if (GIsEditor && world && !world->HasBegunPlay())
	{
		// scale (0,1] into [0.2,1] range to get around hardcoded deadzone in editor
		return prenorm + (AxisVal <= 0 ? -0.2f : 0.2f);
	}
#endif
	return 3.0*prenorm + (AxisVal <= 0 ? -0.2f : 0.2f);
}

bool FSpaceNav3DMessageHandler::ProcessMessage(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam, int32& OutResult)
{
	static uint32 spwid = 0;

	if (spwid == 0) {
		spwid = RegisterWindowMessage(_T("SpaceWareMessage00"));
		UE_LOG(LogSpaceNav3DController, Display, TEXT("SpaceWare port = %d, hwnd = %x"), spwid, hwnd);
	}

	if (msg == spwid)
	{
		int            num;      /* number of button returned */
		SiSpwEvent     Event;    /* 3DxWare Event */
		SiGetEventData EData;    /* 3DxWare Event Data */

		/* init Window platform specific data for a call to SiGetEvent */
		SiGetEventWinInit(&EData, spwid, wParam, lParam);

		/* check whether msg was a 3D mouse event and process it */
		if (SiGetEvent(controller->m_DevHdl, 0, &EData, &Event) == SI_IS_EVENT)
		{
			static bool show = true;

			switch (Event.type)
			{
			case SI_MOTION_EVENT:
				//SiSetLEDs(controller->m_DevHdl, show = !show);  // toggle LEDS on any button
				controller->ControllerState.LeftAnalogX = LongToNormalizedFloat(Event.u.spwData.mData[SI_TX]); // pan left/right
				controller->ControllerState.LeftAnalogY = LongToNormalizedFloat(Event.u.spwData.mData[SI_TZ]); // pan forward/backward
				controller->ControllerState.RightAnalogX = -LongToNormalizedFloat(Event.u.spwData.mData[SI_RY]); // rotate left/right
				controller->ControllerState.RightAnalogY = LongToNormalizedFloat(Event.u.spwData.mData[SI_RX]);  // rotate up/down

				if (!GIsEditor) // really weird to allow roll in editor, so we won't!
					controller->ControllerState.RollAnalog = LongToNormalizedFloat(Event.u.spwData.mData[SI_RZ]);  // rotate up/down

				if (Event.u.spwData.mData[SI_TZ] >= 0) {
					controller->ControllerState.LeftTriggerAnalog = -LongToNormalizedFloat(Event.u.spwData.mData[SI_TY]);
					controller->ControllerState.RightTriggerAnalog = 0.0;
				}
				else if (Event.u.spwData.mData[SI_TZ] < 0) {
					controller->ControllerState.RightTriggerAnalog = LongToNormalizedFloat(Event.u.spwData.mData[SI_TY]);
					controller->ControllerState.LeftTriggerAnalog = 0.0;
				}

				controller->bNewEvent = true;
				break;

			case SI_ZERO_EVENT:
				//SiSetLEDs(controller->m_DevHdl, show = !show);  // toggle LEDS on any button
				FMemory::Memzero(&controller->ControllerState, sizeof(controller->ControllerState));
				controller->bNewEvent = true;
				break;

			case SI_CMD_EVENT:
				if (Event.u.cmdEventData.pressed)
				{
					SPWuint32 v3dcmd = Event.u.cmdEventData.functionNumber;
					UE_LOG(LogSpaceNav3DController, Display, TEXT("Si Command Event %d"), v3dcmd);
					controller->ControllerState.cmd = v3dcmd;
				}
				controller->bNewEvent = true;
				break;
#if 1
			case  SI_BUTTON_EVENT:
				// We latch the button presses - in other words, if a user pushes and releases the button in between engine ticks, it will still get input to the engine eventually
				if ((num = SiButtonPressed(&Event)) != SI_NO_BUTTON)
				{
					// do something when a button is pressed
					//SiSetLEDs(controller->m_DevHdl, show = !show);  // toggle LEDS on any button
					controller->ControllerState.button_pressed = num;
					SPWuint32 v3dkey;
					SiGetButtonV3DK(controller->m_DevHdl, num, &v3dkey);
					UE_LOG(LogSpaceNav3DController, Display, TEXT("SiButtonPressed %d, (%d)"), num, v3dkey);
				}
				if ((num = SiButtonReleased(&Event)) != SI_NO_BUTTON)
				{
					 // do something when a button is released
					controller->ControllerState.button_released = num;
					UE_LOG(LogSpaceNav3DController, Display, TEXT("SiButtonReleased %d"), num);
				}
				controller->bNewEvent = true;
				break;
#endif
			} /* end switch */
		} /* end SiGetEvent */

		return true;
	}
	return false; // nope, wasn't us
}

class FSpaceNav3DModule : public IInputDeviceModule
{
	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		return TSharedPtr< class IInputDevice >(new FSpaceNav3DController(InMessageHandler));
	}
};

IMPLEMENT_MODULE(FSpaceNav3DModule, SpaceNav3D)


#endif
