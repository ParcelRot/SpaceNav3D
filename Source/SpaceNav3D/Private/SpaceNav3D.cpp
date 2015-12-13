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
		: MessageHandler(InMessageHandler), bNewEvent(false), m_DevHdl(NULL)
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

	virtual void SendControllerEvents() override
	{
		if (bNewEvent) {
			bNewEvent = false;

			MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftAnalogX, 0, ControllerState.LeftAnalogX); // pan left/right
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftAnalogY, 0, ControllerState.LeftAnalogY); // pan forward/backward
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::RightAnalogX, 0, ControllerState.RightAnalogX); // rotate left/right
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::RightAnalogY, 0, ControllerState.RightAnalogY);  // rotate up/down
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::LeftTriggerAnalog, 0, ControllerState.LeftTriggerAnalog); // Pan vertical
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::RightTriggerAnalog, 0, ControllerState.RightTriggerAnalog); // Pan vertical
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
	} ControllerState, PrevControllerState;

	FSpaceNav3DMessageHandler mouse_handler; // 3D mouse events sent to UE window

	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;
};

static float ShortToNormalizedFloat(SHORT AxisVal)
{
	// normalize [-32768..32767] -> [-1..1]
	const float Norm = (AxisVal <= 0 ? 2048.f : 2047.f);
	return float(AxisVal) / Norm;
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
				controller->ControllerState.LeftAnalogX = ShortToNormalizedFloat(Event.u.spwData.mData[SI_TX]); // pan left/right
				controller->ControllerState.LeftAnalogY = ShortToNormalizedFloat(Event.u.spwData.mData[SI_TZ]); // pan forward/backward
				controller->ControllerState.RightAnalogX = -ShortToNormalizedFloat(Event.u.spwData.mData[SI_RY]); // rotate left/right
				controller->ControllerState.RightAnalogY = ShortToNormalizedFloat(Event.u.spwData.mData[SI_RX]);  // rotate up/down

				if (Event.u.spwData.mData[SI_TZ] >= 0) {
					controller->ControllerState.LeftTriggerAnalog = -ShortToNormalizedFloat(Event.u.spwData.mData[SI_TY]);
					controller->ControllerState.RightTriggerAnalog = 0.0;
				}
				else if (Event.u.spwData.mData[SI_TZ] < 0) {
					controller->ControllerState.RightTriggerAnalog = ShortToNormalizedFloat(Event.u.spwData.mData[SI_TY]);
					controller->ControllerState.LeftTriggerAnalog = 0.0;
				}

				controller->bNewEvent = true;
				break;

			case SI_ZERO_EVENT:
				//SiSetLEDs(controller->m_DevHdl, show = !show);  // toggle LEDS on any button
				FMemory::Memzero(&controller->ControllerState, sizeof(controller->ControllerState));
				controller->bNewEvent = true;
				break;

			case  SI_BUTTON_EVENT:
				if ((num = SiButtonPressed(&Event)) != SI_NO_BUTTON)
				{
					// do something when a button is pressed
					//SiSetLEDs(controller->m_DevHdl, show = !show);  // toggle LEDS on any button
				}
				if ((num = SiButtonReleased(&Event)) != SI_NO_BUTTON)
				{
					 // do something when a button is released
				}
				controller->bNewEvent = true;
				break;
			} /* end switch */
		} /* end SiGetEvent */
	}
	return false; // we MUST return false here otherwise weird things like Tooltips not rendering right in UE4 Editor!
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
