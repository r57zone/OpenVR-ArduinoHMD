//============ Copyright (c) Valve Corporation, All rights reserved. ============
//=============== Changed by r57zone (https://github.com/r57zone) ===============

#include <openvr_driver.h>
#include <atlstr.h> 
//#include "driverlog.h"

#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

#if defined( _WINDOWS )
#include <windows.h>
#endif

#pragma warning (disable: 4996)

using namespace vr;


#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec( dllexport )
#define HMD_DLL_IMPORT extern "C" __declspec( dllimport )
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C" 
#else
#error "Unsupported Platform."
#endif

inline HmdQuaternion_t HmdQuaternion_Init( double w, double x, double y, double z )
{
	HmdQuaternion_t quat;
	quat.w = w;
	quat.x = x;
	quat.y = y;
	quat.z = z;
	return quat;
}

inline void HmdMatrix_SetIdentity( HmdMatrix34_t *pMatrix )
{
	pMatrix->m[0][0] = 1.f;
	pMatrix->m[0][1] = 0.f;
	pMatrix->m[0][2] = 0.f;
	pMatrix->m[0][3] = 0.f;
	pMatrix->m[1][0] = 0.f;
	pMatrix->m[1][1] = 1.f;
	pMatrix->m[1][2] = 0.f;
	pMatrix->m[1][3] = 0.f;
	pMatrix->m[2][0] = 0.f;
	pMatrix->m[2][1] = 0.f;
	pMatrix->m[2][2] = 1.f;
	pMatrix->m[2][3] = 0.f;
}


// keys for use with the settings API
static const char * const k_pch_steamvr_Section = "SteamVR";
static const char * const k_pch_arduinoHMD_SerialNumber_String = "SerialNumber";
static const char * const k_pch_arduinoHMD_ModelNumber_String = "ModelNumber";
static const char * const k_pch_arduinoHMD_WindowX_Int32 = "WindowX";
static const char * const k_pch_arduinoHMD_WindowY_Int32 = "WindowY";
static const char * const k_pch_arduinoHMD_WindowWidth_Int32 = "windowWidth";
static const char * const k_pch_arduinoHMD_WindowHeight_Int32 = "windowHeight";
static const char * const k_pch_arduinoHMD_RenderWidth_Int32 = "RenderWidth";
static const char * const k_pch_arduinoHMD_RenderHeight_Int32 = "RenderHeight";
static const char * const k_pch_arduinoHMD_SecondsFromVsyncToPhotons_Float = "SecondsFromVsyncToPhotons";
static const char * const k_pch_arduinoHMD_DisplayFrequency_Float = "displayFrequency";

// own output settings
static const char * const k_pch_arduinoHMD_DistortionK1_Float = "DistortionK1";
static const char * const k_pch_arduinoHMD_DistortionK2_Float = "DistortionK2";
static const char * const k_pch_arduinoHMD_ZoomWidth_Float = "ZoomWidth";
static const char * const k_pch_arduinoHMD_ZoomHeight_Float = "ZoomHeight";
static const char * const k_pch_arduinoHMD_FOV_Float = "FOV";
static const char * const k_pch_arduinoHMD_IPD_Float = "IPD";
static const char * const k_pch_arduinoHMD_DistanceBetweenEyes_Int32 = "DistanceBetweenEyes";
static const char * const k_pch_arduinoHMD_ScreenOffsetX_Int32 = "ScreenOffsetX";
static const char * const k_pch_arduinoHMD_Stereo_Bool = "Stereo";
static const char * const k_pch_arduinoHMD_DebugMode_Bool = "DebugMode";

// arduino hmd settings
static const char * const k_pch_arduinoHMD_Section = "ArduinoHMD";
static const char * const k_pch_arduinoHMD_ArduinoRequire_Bool = "ArduinoRequire";
static const char * const k_pch_arduinoHMD_ArduinoRotationType_Bool = "RotationType";
static const char * const k_pch_arduinoHMD_COM_port_Int32 = "COMPort";
static const char * const k_pch_arduinoHMD_CenteringKey_String = "CenteringKey";
static const char * const k_pch_arduinoHMD_CrouchPressKey_String = "CrouchPressKey";
static const char * const k_pch_arduinoHMD_CrouchOffset_Float = "CrouchOffset";

HANDLE hSerial;
int32_t comPortNumber;

bool ArduinoRotationType = false; // 1 - quat, 0 - ypr
bool HMDConnected = false, HMDInitCentring = false, ArduinoNotRequire = false;
std::thread *pArduinoReadThread = NULL;

// Position
double fPos[3] = { 0, 0, 0 };

#define StepPos 0.0033;
#define StepRot 0.2;

// YPR type
float ArduinoIMU[3] = { 0, 0, 0 }, yprOffset[3] = { 0, 0, 0 }; // Yaw, Pitch, Roll
float LastArduinoIMU[3] = { 0, 0, 0 };

// Quaternion type
struct QuaternionF {
	float w, x, y, z;
};
QuaternionF ArduinoIMUQuat = { 0, 0, 0, 0 }, HMDQuatOffset = { 0, 0, 0, 0 }, LastArduinoIMUQuat = { 0, 0, 0, 0 };

double DegToRad(double f) {
	return f * (3.14159265358979323846 / 180);
}

float OffsetYPR(float f, float f2)
{
	f -= f2;
	if (f < -180)
		f += 360;
	else if (f > 180)
		f -= 360;

	return f;
}

inline vr::HmdQuaternion_t EulerAngleToQuaternion(double Yaw, double Pitch, double Roll)
{
	vr::HmdQuaternion_t q;
	// Abbreviations for the various angular functions
	double cy = cos(Yaw * 0.5);
	double sy = sin(Yaw * 0.5);
	double cp = cos(Pitch * 0.5);
	double sp = sin(Pitch * 0.5);
	double cr = cos(Roll * 0.5);
	double sr = sin(Roll * 0.5);

	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;

	return q;
}

bool CorrectAngleValue(float Value)
{
	if (Value > -180 && Value < 180)
		return true;
	else
		return false;
}

bool CorrectQuatValue(float Value)
{
	if (Value > 1 || Value < -1)
		return false;
	else
		return true;
}

void SetCentering()
{
	if (ArduinoRotationType == false) {
		yprOffset[0] = ArduinoIMU[0];
		yprOffset[1] = ArduinoIMU[1];
		yprOffset[2] = ArduinoIMU[2];
	}
	else {
		float length = std::sqrt(ArduinoIMUQuat.x * ArduinoIMUQuat.x + ArduinoIMUQuat.y * ArduinoIMUQuat.y + ArduinoIMUQuat.z * ArduinoIMUQuat.z + ArduinoIMUQuat.w * ArduinoIMUQuat.w);
		ArduinoIMUQuat.w /= length;
		ArduinoIMUQuat.x /= length;
		ArduinoIMUQuat.y /= length;
		ArduinoIMUQuat.z /= length;
		HMDQuatOffset.w = ArduinoIMUQuat.w;
		HMDQuatOffset.x = ArduinoIMUQuat.x;
		HMDQuatOffset.z = ArduinoIMUQuat.y;
		HMDQuatOffset.z = ArduinoIMUQuat.z;
	}
}

void ArduinoIMURead()
{
	DWORD bytesRead;

	while (HMDConnected) {

		// YPR
		if (ArduinoRotationType == false) {
			ReadFile(hSerial, &ArduinoIMU, sizeof(ArduinoIMU), &bytesRead, 0);

			if (CorrectAngleValue(ArduinoIMU[0]) == false || CorrectAngleValue(ArduinoIMU[1]) == false || CorrectAngleValue(ArduinoIMU[2]) == false)
			{
				// last correct values
				ArduinoIMU[0] = LastArduinoIMU[0];
				ArduinoIMU[1] = LastArduinoIMU[1];
				ArduinoIMU[2] = LastArduinoIMU[2];

				PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
			}
			else if (CorrectAngleValue(ArduinoIMU[0]) && CorrectAngleValue(ArduinoIMU[1]) && CorrectAngleValue(ArduinoIMU[2])) // save last correct values
			{
				LastArduinoIMU[0] = ArduinoIMU[0];
				LastArduinoIMU[1] = ArduinoIMU[1];
				LastArduinoIMU[2] = ArduinoIMU[2];

				if (HMDInitCentring == false)
					if (ArduinoIMU[0] != 0 || ArduinoIMU[1] != 0 || ArduinoIMU[2] != 0) {
						SetCentering();
						HMDInitCentring = true;
					}
			}

		// Quaternion
		} else {
			ReadFile(hSerial, &ArduinoIMUQuat, sizeof(ArduinoIMUQuat), &bytesRead, 0);

			if (CorrectQuatValue(ArduinoIMUQuat.w) == false || CorrectQuatValue(ArduinoIMUQuat.x) == false || CorrectQuatValue(ArduinoIMUQuat.y) == false || CorrectQuatValue(ArduinoIMUQuat.z) == false)
			{
				// last correct values
				ArduinoIMUQuat.w = LastArduinoIMUQuat.w;
				ArduinoIMUQuat.x = LastArduinoIMUQuat.x;
				ArduinoIMUQuat.y = LastArduinoIMUQuat.y;
				ArduinoIMUQuat.z = LastArduinoIMUQuat.z;

				PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
			}
			else if (CorrectQuatValue(ArduinoIMUQuat.w) && CorrectQuatValue(ArduinoIMUQuat.x) && CorrectQuatValue(ArduinoIMUQuat.y) && CorrectQuatValue(ArduinoIMUQuat.z)) // save last correct values
			{
				LastArduinoIMUQuat.w = ArduinoIMUQuat.w;
				LastArduinoIMUQuat.x = ArduinoIMUQuat.x;
				LastArduinoIMUQuat.y = ArduinoIMUQuat.y;
				LastArduinoIMUQuat.z = ArduinoIMUQuat.z;

				if (HMDInitCentring == false)
					if (ArduinoIMUQuat.w != 0 || ArduinoIMUQuat.x != 0 || ArduinoIMUQuat.y != 0 || ArduinoIMUQuat.z != 0) {
						SetCentering();
						HMDInitCentring = true;
					}
			}

		}

		if (bytesRead == 0) Sleep(1);
	}
}

void ArduinoIMUStart() {
	TCHAR PortName[32] = { 0 };
	_stprintf(PortName, TEXT("\\\\.\\COM%d"), comPortNumber);
	//CString sPortName;
	//sPortName.Format(_T("COM%d"), IniFile.ReadInteger("Main", "ComPort", 2));

	hSerial = ::CreateFile(PortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hSerial != INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND) {

		DCB dcbSerialParams = { 0 };
		dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

		if (GetCommState(hSerial, &dcbSerialParams))
		{
			dcbSerialParams.BaudRate = CBR_115200;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;

			if (SetCommState(hSerial, &dcbSerialParams))
			{
				HMDConnected = true;
				PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
				pArduinoReadThread = new std::thread(ArduinoIMURead);
			}
		}
	}
}

int KeyNameToKeyCode(std::string KeyName) {
	std::transform(KeyName.begin(), KeyName.end(), KeyName.begin(), ::toupper);

	if (KeyName == "NONE") return 0;

	else if (KeyName == "MOUSE-LEFT-BTN") return VK_LBUTTON;
	else if (KeyName == "MOUSE-RIGHT-BTN") return VK_RBUTTON;
	else if (KeyName == "MOUSE-MIDDLE-BTN") return VK_MBUTTON;
	else if (KeyName == "MOUSE-SIDE1-BTN") return VK_XBUTTON1;
	else if (KeyName == "MOUSE-SIDE2-BTN") return VK_XBUTTON2;

	else if (KeyName == "ESCAPE") return VK_ESCAPE;
	else if (KeyName == "F1") return VK_F1;
	else if (KeyName == "F2") return VK_F2;
	else if (KeyName == "F3") return VK_F3;
	else if (KeyName == "F4") return VK_F4;
	else if (KeyName == "F5") return VK_F5;
	else if (KeyName == "F6") return VK_F6;
	else if (KeyName == "F7") return VK_F7;
	else if (KeyName == "F8") return VK_F8;
	else if (KeyName == "F9") return VK_F9;
	else if (KeyName == "F10") return VK_F10;
	else if (KeyName == "F11") return VK_F11;
	else if (KeyName == "F12") return VK_F12;

	else if (KeyName == "~") return 192;
	else if (KeyName == "1") return '1';
	else if (KeyName == "2") return '2';
	else if (KeyName == "3") return '3';
	else if (KeyName == "4") return '4';
	else if (KeyName == "5") return '5';
	else if (KeyName == "6") return '6';
	else if (KeyName == "7") return '7';
	else if (KeyName == "8") return '8';
	else if (KeyName == "9") return '9';
	else if (KeyName == "0") return '0';
	else if (KeyName == "-") return 189;
	else if (KeyName == "=") return 187;

	else if (KeyName == "TAB") return VK_TAB;
	else if (KeyName == "CAPS-LOCK") return VK_CAPITAL;
	else if (KeyName == "SHIFT") return VK_SHIFT;
	else if (KeyName == "CTRL") return VK_CONTROL;
	else if (KeyName == "WIN") return VK_LWIN;
	else if (KeyName == "ALT") return VK_MENU;
	else if (KeyName == "SPACE") return VK_SPACE;
	else if (KeyName == "ENTER") return VK_RETURN;
	else if (KeyName == "BACKSPACE") return VK_BACK;

	else if (KeyName == "Q") return 'Q';
	else if (KeyName == "W") return 'W';
	else if (KeyName == "E") return 'E';
	else if (KeyName == "R") return 'R';
	else if (KeyName == "T") return 'T';
	else if (KeyName == "Y") return 'Y';
	else if (KeyName == "U") return 'U';
	else if (KeyName == "I") return 'I';
	else if (KeyName == "O") return 'O';
	else if (KeyName == "P") return 'P';
	else if (KeyName == "[") return 219;
	else if (KeyName == "]") return 221;
	else if (KeyName == "A") return 'A';
	else if (KeyName == "S") return 'S';
	else if (KeyName == "D") return 'D';
	else if (KeyName == "F") return 'F';
	else if (KeyName == "G") return 'G';
	else if (KeyName == "H") return 'H';
	else if (KeyName == "J") return 'J';
	else if (KeyName == "K") return 'K';
	else if (KeyName == "L") return 'L';
	else if (KeyName == ":") return 186;
	else if (KeyName == "APOSTROPHE") return 222;
	else if (KeyName == "\\") return 220;
	else if (KeyName == "Z") return 'Z';
	else if (KeyName == "X") return 'X';
	else if (KeyName == "C") return 'C';
	else if (KeyName == "V") return 'V';
	else if (KeyName == "B") return 'B';
	else if (KeyName == "N") return 'N';
	else if (KeyName == "M") return 'M';
	else if (KeyName == "<") return 188;
	else if (KeyName == ">") return 190;
	else if (KeyName == "?") return 191;

	else if (KeyName == "PRINTSCREEN") return VK_SNAPSHOT;
	else if (KeyName == "SCROLL-LOCK") return VK_SCROLL;
	else if (KeyName == "PAUSE") return VK_PAUSE;
	else if (KeyName == "INSERT") return VK_INSERT;
	else if (KeyName == "HOME") return VK_HOME;
	else if (KeyName == "PAGE-UP") return VK_NEXT;
	else if (KeyName == "DELETE") return VK_DELETE;
	else if (KeyName == "END") return VK_END;
	else if (KeyName == "PAGE-DOWN") return VK_PRIOR;

	else if (KeyName == "UP") return VK_UP;
	else if (KeyName == "DOWN") return VK_DOWN;
	else if (KeyName == "LEFT") return VK_LEFT;
	else if (KeyName == "RIGHT") return VK_RIGHT;

	else if (KeyName == "NUM-LOCK") return VK_NUMLOCK;
	else if (KeyName == "NUMPAD0") return VK_NUMPAD0;
	else if (KeyName == "NUMPAD1") return VK_NUMPAD1;
	else if (KeyName == "NUMPAD2") return VK_NUMPAD2;
	else if (KeyName == "NUMPAD3") return VK_NUMPAD3;
	else if (KeyName == "NUMPAD4") return VK_NUMPAD4;
	else if (KeyName == "NUMPAD5") return VK_NUMPAD5;
	else if (KeyName == "NUMPAD6") return VK_NUMPAD6;
	else if (KeyName == "NUMPAD7") return VK_NUMPAD7;
	else if (KeyName == "NUMPAD8") return VK_NUMPAD8;
	else if (KeyName == "NUMPAD9") return VK_NUMPAD9;

	else if (KeyName == "NUMPAD-DIVIDE") return VK_DIVIDE;
	else if (KeyName == "NUMPAD-MULTIPLY") return VK_MULTIPLY;
	else if (KeyName == "NUMPAD-MINUS") return VK_SUBTRACT;
	else if (KeyName == "NUMPAD-PLUS") return VK_ADD;
	else if (KeyName == "NUMPAD-DEL") return VK_DECIMAL;

	else return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CDeviceDriver : public vr::ITrackedDeviceServerDriver, public vr::IVRDisplayComponent
{
public:
	CDeviceDriver(  )
	{
		m_hmdId = vr::k_unTrackedDeviceIndexInvalid;
		m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

		//DriverLog( "Using settings values\n" );
		m_flIPD = vr::VRSettings()->GetFloat(k_pch_steamvr_Section, k_pch_arduinoHMD_IPD_Float);

		char buf[1024];
		vr::VRSettings()->GetString(k_pch_steamvr_Section, k_pch_arduinoHMD_SerialNumber_String, buf, sizeof( buf ) );
		m_sSerialNumber = buf;

		vr::VRSettings()->GetString(k_pch_steamvr_Section, k_pch_arduinoHMD_ModelNumber_String, buf, sizeof( buf ) );
		m_sModelNumber = buf;

		m_nWindowX = vr::VRSettings()->GetInt32(k_pch_steamvr_Section, k_pch_arduinoHMD_WindowX_Int32 );
		m_nWindowY = vr::VRSettings()->GetInt32(k_pch_steamvr_Section, k_pch_arduinoHMD_WindowY_Int32 );
		m_nWindowWidth = vr::VRSettings()->GetInt32(k_pch_steamvr_Section, k_pch_arduinoHMD_WindowWidth_Int32 );
		m_nWindowHeight = vr::VRSettings()->GetInt32(k_pch_steamvr_Section, k_pch_arduinoHMD_WindowHeight_Int32 );
		m_nRenderWidth = vr::VRSettings()->GetInt32(k_pch_steamvr_Section, k_pch_arduinoHMD_RenderWidth_Int32 );
		m_nRenderHeight = vr::VRSettings()->GetInt32(k_pch_steamvr_Section, k_pch_arduinoHMD_RenderHeight_Int32 );
		m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(k_pch_steamvr_Section, k_pch_arduinoHMD_SecondsFromVsyncToPhotons_Float );
		m_flDisplayFrequency = vr::VRSettings()->GetFloat(k_pch_steamvr_Section, k_pch_arduinoHMD_DisplayFrequency_Float );

		m_fDistortionK1 = vr::VRSettings()->GetFloat(k_pch_steamvr_Section, k_pch_arduinoHMD_DistortionK1_Float);
		m_fDistortionK2 = vr::VRSettings()->GetFloat(k_pch_steamvr_Section, k_pch_arduinoHMD_DistortionK2_Float);
		m_fZoomWidth = vr::VRSettings()->GetFloat(k_pch_steamvr_Section, k_pch_arduinoHMD_ZoomWidth_Float);
		m_fZoomHeight = vr::VRSettings()->GetFloat(k_pch_steamvr_Section, k_pch_arduinoHMD_ZoomHeight_Float);
		m_fFOV = (vr::VRSettings()->GetFloat(k_pch_steamvr_Section, k_pch_arduinoHMD_FOV_Float) * 3.14159265358979323846 / 180); //radians
		m_nDistanceBetweenEyes = vr::VRSettings()->GetInt32(k_pch_steamvr_Section, k_pch_arduinoHMD_DistanceBetweenEyes_Int32);
		m_nScreenOffsetX = vr::VRSettings()->GetInt32(k_pch_steamvr_Section, k_pch_arduinoHMD_ScreenOffsetX_Int32);
		m_bStereoMode = vr::VRSettings()->GetBool(k_pch_steamvr_Section, k_pch_arduinoHMD_Stereo_Bool);
		m_bDebugMode = vr::VRSettings()->GetBool(k_pch_steamvr_Section, k_pch_arduinoHMD_DebugMode_Bool);

		vr::VRSettings()->GetString(k_pch_arduinoHMD_Section, k_pch_arduinoHMD_CenteringKey_String, buf, sizeof(buf));
		m_centeringKey = KeyNameToKeyCode(buf);

		vr::VRSettings()->GetString(k_pch_arduinoHMD_Section, k_pch_arduinoHMD_CrouchPressKey_String, buf, sizeof(buf));
		m_crouchPressKey = KeyNameToKeyCode(buf);

		m_crouchOffset = vr::VRSettings()->GetFloat(k_pch_arduinoHMD_Section, k_pch_arduinoHMD_CrouchOffset_Float);

		vr::VRSettings()->GetString(k_pch_arduinoHMD_Section, "HMDUpKey", buf, sizeof(buf));
		m_hmdUpKey = KeyNameToKeyCode(buf);

		vr::VRSettings()->GetString(k_pch_arduinoHMD_Section, "HMDDownKey", buf, sizeof(buf));
		m_hmdDownKey = KeyNameToKeyCode(buf);

		vr::VRSettings()->GetString(k_pch_arduinoHMD_Section, "HMDResetKey", buf, sizeof(buf));
		m_hmdResetKey = KeyNameToKeyCode(buf);

		//DriverLog( "driver_arduinohmd: Serial Number: %s\n", m_sSerialNumber.c_str() );
		//DriverLog( "driver_arduinohmd: Model Number: %s\n", m_sModelNumber.c_str() );
		//DriverLog( "driver_arduinohmd: Window: %d %d %d %d\n", m_nWindowX, m_nWindowY, m_nWindowWidth, m_nWindowHeight );
		//DriverLog( "driver_arduinohmd: Render Target: %d %d\n", m_nRenderWidth, m_nRenderHeight );
		//DriverLog( "driver_arduinohmd: Seconds from Vsync to Photons: %f\n", m_flSecondsFromVsyncToPhotons );
		//DriverLog( "driver_arduinohmd: Display Frequency: %f\n", m_flDisplayFrequency );
		//DriverLog( "driver_arduinohmd: IPD: %f\n", m_flIPD );
	}

	virtual ~CDeviceDriver()
	{
	}


	virtual EVRInitError Activate( vr::TrackedDeviceIndex_t unObjectId ) 
	{
		m_hmdId = unObjectId;
		m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_hmdId);

		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str() );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str() );

		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_SerialNumber_String, m_sSerialNumber.c_str());
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_WillDriftInYaw_Bool, false);
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ManufacturerName_String, "OpenVR-ArduinoHMD");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_TrackingFirmwareVersion_String, "1.0");
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_HardwareRevision_String, "1.0");
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DeviceIsWireless_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DeviceIsCharging_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_CanUnifyCoordinateSystemWithHmd_Bool, true);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_ContainsProximitySensor_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_DeviceCanPowerOff_Bool, false);
		vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_DeviceClass_Int32, vr::TrackedDeviceClass_HMD);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_HasCamera_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_ForceUpdateRequired_Bool, false);
		vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_RegisteredDeviceType_String, "HMD");
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_NeverTracked_Bool, false);
		vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Identifiable_Bool, false);

		vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_UserIpdMeters_Float, m_flIPD );
		vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.f );
		vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_DisplayFrequency_Float, m_flDisplayFrequency );
		vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, m_flSecondsFromVsyncToPhotons );

		// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
		vr::VRProperties()->SetUint64Property( m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2 );

		// avoid "not fullscreen" warnings from vrmonitor
		vr::VRProperties()->SetBoolProperty( m_ulPropertyContainer, Prop_IsOnDesktop_Bool, false );

		// debug mode activate Windowed Mode (borderless fullscreen), lock to 30 FPS 
		vr::VRProperties()->SetBoolProperty( m_ulPropertyContainer, Prop_DisplayDebugMode_Bool, m_bDebugMode );

		// Icons can be configured in code or automatically configured by an external file "drivername\resources\driver.vrresources".
		// Icon properties NOT configured in code (post Activate) are then auto-configured by the optional presence of a driver's "drivername\resources\driver.vrresources".
		// In this manner a driver can configure their icons in a flexible data driven fashion by using an external file.
		//
		// The structure of the driver.vrresources file allows a driver to specialize their icons based on their HW.
		// Keys matching the value in "Prop_ModelNumber_String" are considered first, since the driver may have model specific icons.
		// An absence of a matching "Prop_ModelNumber_String" then considers the ETrackedDeviceClass ("HMD", "Controller", "GenericTracker", "TrackingReference")
		// since the driver may have specialized icons based on those device class names.
		//
		// An absence of either then falls back to the "system.vrresources" where generic device class icons are then supplied.
		//
		// Please refer to "bin\drivers\sample\resources\driver.vrresources" which contains this sample configuration.
		//
		// "Alias" is a reserved key and specifies chaining to another json block.
		//
		// In this sample configuration file (overly complex FOR EXAMPLE PURPOSES ONLY)....
		//
		// "Model-v2.0" chains through the alias to "Model-v1.0" which chains through the alias to "Model-v Defaults".
		//
		// Keys NOT found in "Model-v2.0" would then chase through the "Alias" to be resolved in "Model-v1.0" and either resolve their or continue through the alias.
		// Thus "Prop_NamedIconPathDeviceAlertLow_String" in each model's block represent a specialization specific for that "model".
		// Keys in "Model-v Defaults" are an example of mapping to the same states, and here all map to "Prop_NamedIconPathDeviceOff_String".
		//
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceOff_String, "{indexhmd}/icons/headset_status_off.png" );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearching_String, "{indexhmd}/icons/headset_status_searching.gif" );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{indexhmd}/icons/headset_status_searching_alert.gif" );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReady_String, "{indexhmd}/icons/headset_status_ready.png" );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{indexhmd}/icons/headset_status_ready_alert.png" );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceNotReady_String, "{indexhmd}/icons/headset_status_error.png" );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceStandby_String, "{indexhmd}/icons/headset_status_standby.png" );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceAlertLow_String, "{indexhmd}/icons/headset_status_standby.png" ); //headset_status_ready_low.png

		return VRInitError_None;
	}

	virtual void Deactivate() 
	{
		m_hmdId = vr::k_unTrackedDeviceIndexInvalid;
	}

	virtual void EnterStandby()
	{
	}

	void *GetComponent( const char *pchComponentNameAndVersion )
	{
		if ( !_stricmp( pchComponentNameAndVersion, vr::IVRDisplayComponent_Version ) )
		{
			return (vr::IVRDisplayComponent*)this;
		}

		// override this to add a component to a driver
		return NULL;
	}

	virtual void PowerOff() 
	{
	}

	/** debug request from a client */
	virtual void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize ) 
	{
		if( unResponseBufferSize >= 1 )
			pchResponseBuffer[0] = 0;
	}

	virtual void GetWindowBounds( int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight ) 
	{
		*pnX = m_nWindowX;
		*pnY = m_nWindowY;
		*pnWidth = m_nWindowWidth;
		*pnHeight = m_nWindowHeight;
	}

	virtual bool IsDisplayOnDesktop() 
	{
		return true;
	}

	virtual bool IsDisplayRealDisplay()
	{
		if (m_nWindowX == 0 && m_nWindowY == 0)
			return false;
		else
			return true; //Support working on extended display
	}

	virtual void GetRecommendedRenderTargetSize( uint32_t *pnWidth, uint32_t *pnHeight ) 
	{
		*pnWidth = m_nRenderWidth;
		*pnHeight = m_nRenderHeight;
	}

	virtual void GetEyeOutputViewport( EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight ) 
	{
		if (m_bStereoMode) {

			*pnY = m_nScreenOffsetX;
			*pnWidth = m_nWindowWidth / 2;
			*pnHeight = m_nWindowHeight;

			if (eEye == Eye_Left)
				*pnX = m_nDistanceBetweenEyes;
			else
				*pnX = (m_nWindowWidth / 2) - m_nDistanceBetweenEyes;
		}
		else 
		{ //Mono mode
			pnY = 0;
			*pnWidth = m_nRenderWidth;
			*pnHeight = m_nRenderHeight;

			if (eEye == Eye_Left)
				*pnX = (m_nWindowWidth - m_nRenderWidth) / 2;
			else
				*pnX = m_nWindowWidth;
		}
	}

	virtual void GetProjectionRaw( EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom ) 
	{
		/*pfLeft = -1.0;
		*pfRight = 1.0;
		*pfTop = -1.0;
		*pfBottom = 1.0;*/	

		if (m_bStereoMode) {
			*pfLeft = -m_fFOV;
			*pfRight = m_fFOV;
			*pfTop = -m_fFOV;
			*pfBottom = m_fFOV;
		}
		else { //Mono
			*pfLeft = (float)m_nRenderWidth / m_nRenderHeight * -1;
			*pfRight = (float)m_nRenderWidth / m_nRenderHeight;
			*pfTop = -1.0;
			*pfBottom = 1.0;
		}
	}

	virtual DistortionCoordinates_t ComputeDistortion( EVREye eEye, float fU, float fV ) 
	{
		DistortionCoordinates_t coordinates;

		//distortion for lens from https://github.com/HelenXR/openvr_survivor/blob/master/src/head_mount_display_device.cc
		float hX;
		float hY;
		double rr;
		double r2;
		double theta;

		rr = sqrt((fU - 0.5f)*(fU - 0.5f) + (fV - 0.5f)*(fV - 0.5f));
		r2 = rr * (1 + m_fDistortionK1 * (rr*rr) + m_fDistortionK2 * (rr*rr*rr*rr));
		theta = atan2(fU - 0.5f, fV - 0.5f);
		hX = sin(theta)*r2*m_fZoomWidth;
		hY = cos(theta)*r2*m_fZoomHeight;

		coordinates.rfBlue[0] = hX + 0.5f;
		coordinates.rfBlue[1] = hY + 0.5f;
		coordinates.rfGreen[0] = hX + 0.5f;
		coordinates.rfGreen[1] = hY + 0.5f;
		coordinates.rfRed[0] = hX + 0.5f;
		coordinates.rfRed[1] = hY + 0.5f;

		return coordinates;
	}

	virtual DriverPose_t GetPose() 
	{
		DriverPose_t pose = { 0 };

		if (HMDConnected || ArduinoNotRequire) {
			pose.poseIsValid = true;
			pose.result = TrackingResult_Running_OK;
			pose.deviceIsConnected = true;
		} else {
			pose.poseIsValid = false;
			pose.result = TrackingResult_Uninitialized;
			pose.deviceIsConnected = false;
		}

		pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
		pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
		pose.shouldApplyHeadModel = true;
		pose.poseTimeOffset = 0;
		pose.willDriftInYaw = false;

		if (HMDConnected || ArduinoNotRequire) {

			// Pos
			if ((GetAsyncKeyState(VK_NUMPAD8) & 0x8000) != 0) fPos[2] -= StepPos;
			if ((GetAsyncKeyState(VK_NUMPAD2) & 0x8000) != 0) fPos[2] += StepPos;

			if ((GetAsyncKeyState(VK_NUMPAD4) & 0x8000) != 0) fPos[0] -= StepPos;
			if ((GetAsyncKeyState(VK_NUMPAD6) & 0x8000) != 0) fPos[0] += StepPos;

			if ((GetAsyncKeyState(m_hmdDownKey) & 0x8000) != 0) fPos[1] += StepPos;
			if ((GetAsyncKeyState(m_hmdUpKey) & 0x8000) != 0) fPos[1] -= StepPos;

			// Yaw fixing
			if ((GetAsyncKeyState(VK_NUMPAD1) & 0x8000) != 0 && yprOffset[0] < 180) yprOffset[0] += StepRot;
			if ((GetAsyncKeyState(VK_NUMPAD3) & 0x8000) != 0 && yprOffset[0] > -180) yprOffset[0] -= StepRot;

			// Roll fixing
			if ((GetAsyncKeyState(VK_NUMPAD7) & 0x8000) != 0 && yprOffset[2] < 180) yprOffset[2] += StepRot;
			if ((GetAsyncKeyState(VK_NUMPAD9) & 0x8000) != 0 && yprOffset[2] > -180) yprOffset[2] -= StepRot;

			if ((GetAsyncKeyState(m_hmdResetKey) & 0x8000) != 0)
			{
				fPos[0] = 0;
				fPos[1] = 0;
				fPos[2] = 0;
			}

			// Centering
			if ( (GetAsyncKeyState(m_centeringKey) & 0x8000) != 0 || ( (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 && (GetAsyncKeyState(VK_MENU) & 0x8000) != 0 && (GetAsyncKeyState('R') & 0x8000) != 0) )
				SetCentering();

			// Set head tracking rotation
			if (ArduinoRotationType == false) { // YPR
				pose.qRotation = EulerAngleToQuaternion(DegToRad( OffsetYPR(ArduinoIMU[2], yprOffset[2]) ),
														DegToRad( OffsetYPR(ArduinoIMU[0], yprOffset[0]) * -1 ),
														DegToRad( OffsetYPR(ArduinoIMU[1], yprOffset[1]) * -1 ));
			}  else { // Quaternion
				// Centered
				pose.qRotation.w = ArduinoIMUQuat.w * LastArduinoIMUQuat.w - ArduinoIMUQuat.x * LastArduinoIMUQuat.x - ArduinoIMUQuat.y * LastArduinoIMUQuat.y - ArduinoIMUQuat.z * LastArduinoIMUQuat.z;
				pose.qRotation.x = ArduinoIMUQuat.w * LastArduinoIMUQuat.x + ArduinoIMUQuat.x * LastArduinoIMUQuat.w + ArduinoIMUQuat.y * LastArduinoIMUQuat.z - ArduinoIMUQuat.z * LastArduinoIMUQuat.y;
				pose.qRotation.y = ArduinoIMUQuat.w * LastArduinoIMUQuat.y - ArduinoIMUQuat.x * LastArduinoIMUQuat.z + ArduinoIMUQuat.y * LastArduinoIMUQuat.w + ArduinoIMUQuat.z * LastArduinoIMUQuat.x;
				pose.qRotation.z = ArduinoIMUQuat.w * LastArduinoIMUQuat.z + ArduinoIMUQuat.x * LastArduinoIMUQuat.y - ArduinoIMUQuat.y * LastArduinoIMUQuat.x + ArduinoIMUQuat.z * LastArduinoIMUQuat.w;
			}

			//Set head position tracking
			pose.vecPosition[0] = fPos[0]; // X
			pose.vecPosition[1] = fPos[1]; // Z

			if ((GetAsyncKeyState(m_crouchPressKey) & 0x8000) != 0)
				pose.vecPosition[1] -= m_crouchOffset;
			
			pose.vecPosition[2] = fPos[2]; // Y
		}

		return pose;
	}
	

	void RunFrame()
	{
		// In a real driver, this should happen from some pose tracking thread.
		// The RunFrame interval is unspecified and can be very irregular if some other
		// driver blocks it for some periodic task.
		if (m_hmdId != vr::k_unTrackedDeviceIndexInvalid )
		{
			vr::VRServerDriverHost()->TrackedDevicePoseUpdated( m_hmdId, GetPose(), sizeof( DriverPose_t ) );
		}
	}

	std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
	vr::TrackedDeviceIndex_t m_hmdId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;

	int32_t m_nWindowX;
	int32_t m_nWindowY;
	int32_t m_nWindowWidth;
	int32_t m_nWindowHeight;
	int32_t m_nRenderWidth;
	int32_t m_nRenderHeight;
	float m_flSecondsFromVsyncToPhotons;
	float m_flDisplayFrequency;
	float m_flIPD;

	float m_fDistortionK1;
	float m_fDistortionK2;
	float m_fZoomWidth;
	float m_fZoomHeight;
	float m_fFOV;
	int32_t m_nDistanceBetweenEyes;
	int32_t m_nScreenOffsetX;
	bool m_bStereoMode = true;
	bool m_bDebugMode;

	int32_t m_crouchPressKey;
	float m_crouchOffset;
	int32_t m_centeringKey;
	int32_t m_hmdUpKey;
	int32_t m_hmdDownKey;
	int32_t m_hmdResetKey;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CServerDriver: public IServerTrackedDeviceProvider
{
public:
	virtual EVRInitError Init( vr::IVRDriverContext *pDriverContext ) ;
	virtual void Cleanup() ;
	virtual const char * const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame() ;
	virtual bool ShouldBlockStandbyMode()  { return false; }
	virtual void EnterStandby()  {}
	virtual void LeaveStandby()  {}


private:
	CDeviceDriver *m_pNullHmdLatest = nullptr;
};

CServerDriver g_serverDriverNull;


EVRInitError CServerDriver::Init( vr::IVRDriverContext *pDriverContext )
{
	VR_INIT_SERVER_DRIVER_CONTEXT( pDriverContext );
	//InitDriverLog( vr::VRDriverLog() );

	ArduinoRotationType = vr::VRSettings()->GetInt32(k_pch_arduinoHMD_Section, k_pch_arduinoHMD_ArduinoRotationType_Bool);
	ArduinoNotRequire = !vr::VRSettings()->GetInt32(k_pch_arduinoHMD_Section, k_pch_arduinoHMD_ArduinoRequire_Bool);
	comPortNumber = vr::VRSettings()->GetInt32(k_pch_arduinoHMD_Section, k_pch_arduinoHMD_COM_port_Int32);
	
	ArduinoIMUStart();

	if (HMDConnected || ArduinoNotRequire)
	{
		m_pNullHmdLatest = new CDeviceDriver();
		vr::VRServerDriverHost()->TrackedDeviceAdded(m_pNullHmdLatest->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_pNullHmdLatest);
	}

	return VRInitError_None;
}

void CServerDriver::Cleanup() 
{
	//CleanupDriverLog();

	if (HMDConnected) {
		HMDConnected = false;
		pArduinoReadThread->join();
		delete pArduinoReadThread;
		pArduinoReadThread = nullptr;
		CloseHandle(hSerial);
	}

	if (HMDConnected || ArduinoNotRequire) {
		delete m_pNullHmdLatest;
		m_pNullHmdLatest = NULL;
	}
}


void CServerDriver::RunFrame()
{
	if ( m_pNullHmdLatest )
	{
		m_pNullHmdLatest->RunFrame();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void *HmdDriverFactory( const char *pInterfaceName, int *pReturnCode )
{
	if( 0 == strcmp( IServerTrackedDeviceProvider_Version, pInterfaceName ) )
	{
		return &g_serverDriverNull;
	}

	if( pReturnCode )
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}
