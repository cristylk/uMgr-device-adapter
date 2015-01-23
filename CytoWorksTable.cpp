///////////////////////////////////////////////////////////////////////////////
// FILE:          CWTable_revised.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   CytoWorksTable device adapter.
//                
// AUTHOR:        Cristy Koebler, 10/2014
//
// LICENSE:       This library is free software; you can redistribute it and/or
//                modify it under the terms of the GNU Lesser General Public
//                License as published by the Free Software Foundation.
//                
//                You should have received a copy of the GNU Lesser General Public
//                License along with the source distribution; if not, write to
//                the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
//                Boston, MA  02111-1307  USA
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES. 
//
#ifdef WIN32
   #include <windows.h>
   #define snprintf _snprintf 
#endif

#include "CytoWorksTable.h"
#include "../../../MMDevice/ModuleInterface.h"
#include "../../../MMDevice/DeviceUtils.h"
#include "../../../MMDevice/DeviceBase.h"
#include <cstdio>
#include <math.h>
#include <string>
#include <sstream>
#include <iostream>

//constants
const char* g_Hub = "CytoTableHub";
const char* g_XYStageDeviceName = "CytoTableXYStage";
const char* g_ZStageDeviceName = "ZStage";
const char* g_Axis_Id = "SingleAxisName";
//const char* g_LEDName = "LED";

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
	RegisterDevice(g_Hub, MM::HubDevice, "CytoTableHub (required)");
	RegisterDevice(g_XYStageDeviceName, MM::XYStageDevice, "CytoTableXYStage");
	RegisterDevice(g_ZStageDeviceName, MM::StageDevice, "ZStage");
	//RegisterDevice(g_LEDName, MM::ShutterDevice, "LED");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
	if (deviceName == 0)
		return 0;
	if (strcmp(deviceName, g_Hub) == 0)
	{
		return new Hub();                           
	}
	if (strcmp(deviceName, g_XYStageDeviceName) == 0)
	{
		CytoTableXYStage* pCytoTableXYStage = new CytoTableXYStage();
		return pCytoTableXYStage;
	}
	if (strcmp(deviceName, g_ZStageDeviceName) == 0)
	{
		ZStage* pZStage = new ZStage();
		return pZStage;
	}
	return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

int clearPort(MM::Device& device, MM::Core& core, const char* port)
{
   // Clear contents of serial port
   const unsigned int bufSize = 255;
   unsigned char clear[bufSize];
   unsigned long read = bufSize;
   int ret;
   while (read == bufSize)
   {
      ret = core.ReadFromSerial(&device, port, clear, bufSize, read);
      if (ret != DEVICE_OK)
         return ret;
   }
   return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
//Hub
///////////////////////////////////////////////////////////////////////////////
Hub::Hub() :
	transmissionDelay_(10),
	initialized_(false)
{
   InitializeDefaultErrorMessages();

   // custom error messages:
   SetErrorText(ERR_NO_PORT_SET, "Hub device not found. Connect to Hub first.");
   SetErrorText(ERR_SERIAL_COMMAND_FAILED, "Unable to connect to the port. Is the device		connected?");
   SetErrorText(ERR_NO_ANSWER, "No answer from the controller.  Is it connected?");
   
   // Port:
   CPropertyAction* pAct = new CPropertyAction(this, &Hub::OnPort);
   CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);
}

Hub::~Hub()
{
   Shutdown();
}

void Hub::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_Hub);
}

bool Hub::Busy()
{
   return false;
}

MM::DeviceDetectionStatus Hub::DetectDevice(void)
{
	if (initialized_)
      return MM::CanCommunicate;
   
	// all conditions must be satisfied...
	MM::DeviceDetectionStatus result = MM::Misconfigured;

	try
	{
	   std::string portLowerCase = port_;
		   for( std::string::iterator its = portLowerCase.begin(); its != portLowerCase.end				(); ++its)
	   {
	      *its = (char)tolower(*its);
	   }
	   if( 0< portLowerCase.length() &&  0 != portLowerCase.compare("undefined")  && 0 !=				portLowerCase.compare("unknown") )
	   {
		   result = MM::CanNotCommunicate;
		   // device specific default communication parameters
		   GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_Handshaking,				"Off");
		   GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_BaudRate,					"9600");
		   GetCoreCallback()->SetDeviceProperty(port_.c_str(), MM::g_Keyword_StopBits, "1");
           GetCoreCallback()->SetDeviceProperty(port_.c_str(), "AnswerTimeout", "500.0");
           GetCoreCallback()->SetDeviceProperty(port_.c_str(), "DelayBetweenCharsMs",					"11.0");
           MM::Device* pS = GetCoreCallback()->GetDevice(this, port_.c_str());
           pS->Initialize();
           result = MM::CanCommunicate;
           pS->Shutdown();
        }
     }
     catch(...)
     {
		 LogMessage("Exception in DetectDevice!",false);
     }
    
	 return result;
}

int Hub::Initialize()
{
	clearPort(*this, *GetCoreCallback(), port_.c_str());

	// Name
	int ret = CreateProperty(MM::g_Keyword_Name, g_Hub, MM::String, true);
	if (DEVICE_OK != ret)
		return ret;

	// Description
	ret = CreateProperty(MM::g_Keyword_Description, "CytoWorks Hub", MM::String, true);
	if (DEVICE_OK != ret)
		return ret;

	ret = UpdateStatus();
	if (ret != DEVICE_OK)
		return ret;

	initialized_ = true;

	return DEVICE_OK;
}

int Hub::Shutdown()
{
   if (initialized_)
      initialized_ = false;

   return DEVICE_OK;
}

int Hub::DetectInstalledDevices()
{
   if (MM::CanCommunicate == DetectDevice()) 
   {
      std::vector<std::string> peripherals; 
      peripherals.clear();
      peripherals.push_back(g_XYStageDeviceName);
	  peripherals.push_back(g_ZStageDeviceName);
	  //peripherals.push_back(g_LEDName);
      for (size_t i=0; i < peripherals.size(); i++) 
      {
         MM::Device* pDev = ::CreateDevice(peripherals[i].c_str());
         if (pDev) 
         {
			 AddInstalledDevice(pDev);
         }
      }
   }

   return DEVICE_OK;
}

//////////////// Action Handlers (Hub) /////////////////
int Hub::OnPort(MM::PropertyBase* pProp, MM::ActionType pAct)
{
   if (pAct == MM::BeforeGet)
   {
      pProp->Set(port_.c_str());
   }
   else if (pAct == MM::AfterSet)
   {
      if (initialized_)
      {
         pProp->Set(port_.c_str());
         return ERR_PORT_CHANGE_FORBIDDEN;
      }
      pProp->Get(port_);
   }
   return DEVICE_OK;
}

//////////////////////////////////////////////////////////////////////////////
// XYStage
// * XYStage - two axis stage device
//////////////////////////////////////////////////////////////////////////////
CytoTableXYStage::CytoTableXYStage() :
	initialized_(false), 
	stepSizeXUm_(0.1), //Trying this out and seeing what happens
	stepSizeYUm_(0.1), //Trying this out and seeing what happens
	speed_(2500.0), //Trying this out and seeing what happens
	//maxSpeed_ (7.5),- This is from ASI - do we need it?
	originX_(0),
	originY_(0)
{
	InitializeDefaultErrorMessages();
	// create pre-initialization properties
	SetErrorText(ERR_NO_HUB, "Please add the Hub device first!");

	CreateProperty(MM::g_Keyword_Name, g_XYStageDeviceName, MM::String, true);

	//Description
	CreateProperty(MM::g_Keyword_Description, "CytoWorksTable XY stage driver adapter", MM::String, true);
}

CytoTableXYStage::~CytoTableXYStage()
{
   Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
// XYStage methods required by the API
///////////////////////////////////////////////////////////////////////////////
void CytoTableXYStage::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_XYStageDeviceName);
}

int CytoTableXYStage::Initialize()
{
	//const char* cmd = "/1?9R";
	//int response = SendSerialCommand(port_.c_str(), cmd, "\r");
	/*//Clear the x port, set x current, set x resolution, set top speed, set hold current, set	   acceleration factor, reverse x positive to negative
	This is what jason did - I've included Ludl's code too
	int ret = SetCommand("/1?9R");//this one isn't right - need to set a separate method for this!!
	int ret = SetCommand("/1m40R");
	int ret = SetCommand("/1j256R");
	int ret = SetCommand("/1V64000R");
	int ret = SetCommand("/1h50R");
	int ret = SetCommand("/1L1000R");
	int ret = SetCommand("/1F1R");*/

	/*string command;
	command = "/1?R";
	command = "/1m40R";
	command = "/1j256R";
	command = "/1V20320R";
	command = "/1h50R";
	command = "/1L333R";
	command = "/1F1R";
	command = "/2?9R";
	command = "/2m40R";
	command = "/2j256R";
	command = "/2V20320R";
	command = "/2h50R";
	command = "/2L333R";
	command = "/2F1R";
	command = "/1z2500000R";
	command = "/1A0R";*/

	
	// Step size - need to set actual step size #
	CPropertyAction* pAct = new CPropertyAction (this, &CytoTableXYStage::OnStepSizeX);
	int ret = CreateProperty("StepSize-X", "0.1", MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// Step size - need to set actual step size #
	pAct = new CPropertyAction (this, &CytoTableXYStage::OnStepSizeY);
	ret = CreateProperty("StepSize-Y", "0.1", MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	// Speed (in um/sec) - need to set actual speed
	pAct = new CPropertyAction (this, &CytoTableXYStage::OnSpeed);
	ret = CreateProperty("Speed", "2500.0", MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		 return ret;

	ret = UpdateStatus();
	if (ret != DEVICE_OK)
		 return ret;

	initialized_ = true;
	return DEVICE_OK;
}

int CytoTableXYStage::Shutdown()
{
   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

bool CytoTableXYStage::Busy() 
{//this does not work at all :(
	//Need to check if either X or Y is busy - If yes, return true, if no, return false
   //if (AxisBusy("1") || AxisBusy("2"))
     // return true;
   return false;
}

int CytoTableXYStage::SetPositionSteps(long x, long y)
{
	ostringstream cmdX, cmdY;
	cmdX << "/1P" << x << "R";//will need to change to A eventually
	cmdY << "/2P" << y << "R";
	
	//check if we are busy - X first
	int retX = SendSerialCommand(port_.c_str(), cmdX.str().c_str(), "\r");
	if (retX != DEVICE_OK)
      return retX;

	string responseX;
	retX = GetSerialAnswer(port_.c_str(), "\r", responseX);
	if (retX != DEVICE_OK) 
      return retX;
	if (responseX.length() < 1)
      return ERR_NO_ANSWER;

	istringstream is(responseX);
	string outcomeX;
	is >> outcomeX;

	//now, check if Y is busy
	int retY = SendSerialCommand(port_.c_str(), cmdY.str().c_str(), "\r");
	if (retY != DEVICE_OK)
      return retY;
  
	string responseY;
	retY = GetSerialAnswer(port_.c_str(), "\n", responseY);
	if (retY != DEVICE_OK) 
      return retY;
	if (responseY.length() < 1)
      return ERR_NO_ANSWER;

	istringstream isY(responseY);
	string outcomeY;
	isY >> outcomeY;

	if (outcomeX.compare("/0@`") == 0) 
	{
		if (outcomeY.compare("/1@`")==0)
		{
			return DEVICE_OK; // success!
		}
	}
      //Should device ok be out here?

   // figure out the error code
	int code;
	is >> code;
	return code;

	int codeY;
	isY >> codeY;
	return codeY;
}

int CytoTableXYStage::SetRelativePositionSteps(long x, long y)
{
	ostringstream cmdX, cmdY;
	cmdX << "/1P" << x << "R";
	cmdY << "/2P" << y << "R";

	//check if we are busy - X first
	int retX = SendSerialCommand(port_.c_str(), cmdX.str().c_str(), "\r");
	if (retX != DEVICE_OK)
      return retX;

	string responseX;
	retX = GetSerialAnswer(port_.c_str(), "\r", responseX);
	if (retX != DEVICE_OK) 
      return retX;
	if (responseX.length() < 1)
      return ERR_NO_ANSWER;

	istringstream is(responseX);
	string outcomeX;
	is >> outcomeX;

	//now, check if Y is busy
	int retY = SendSerialCommand(port_.c_str(), cmdY.str().c_str(), "\r");
	if (retY != DEVICE_OK)
      return retY;
  
	string responseY;
	retY = GetSerialAnswer(port_.c_str(), "\n", responseY);
	if (retY != DEVICE_OK) 
      return retY;
	if (responseY.length() < 1)
      return ERR_NO_ANSWER;

	istringstream isY(responseY);
	string outcomeY;
	isY >> outcomeY;

	if (outcomeX.compare("/0@`") == 0) 
	{
		if (outcomeY.compare("/1@`")==0)
		{
			return DEVICE_OK; // success!
		}
	}
      //Should device ok be out here?

   // figure out the error code
	int code;
	is >> code;
	return code;

	int codeY;
	isY >> codeY;
	return codeY;
}

int CytoTableXYStage::GetPositionSteps(long& x, long& y)
{
	PurgeComPort(port_.c_str());
	
	//NEED TO COMPLETELY REDO THIS!!

	const char* cmdX = "/1?0R";
	//const char* cmdY = "/2?0";

	//check if we are busy - X first
	int retX = SendSerialCommand(port_.c_str(), cmdX, "\r");//or is term "R"?
	if (retX != DEVICE_OK)
		return retX;
  
	//string responseX;
	//retX = GetSerialAnswer(port_.c_str(), "\r", responseX);
	//if (retX != DEVICE_OK) 
 //     return retX;
	//if (responseX.length() < 1)
 //     return ERR_NO_ANSWER;

	//istringstream is(responseX);
	//string outcomeX;
	//is >> outcomeX;

	//if (outcomeX.compare("/0@`") == 0) 
	//{
	//	//if (outcomeY.compare("/1@`")==0)
	//	//{
	//		is >> x;
			return DEVICE_OK; // success!
	//	//}
	//}
 //     //Should device ok be out here?
	//y;

 //  // figure out the error code
	//int code;
	//is >> code;
	//return code;

}

int CytoTableXYStage::SetOrigin()
{
	PurgeComPort(port_.c_str());
	//Defines current position as origin (0,0) coordinate of the controller
	const char* cmdX = "/1z0R";
	const char* cmdY = "/2z0R";
	//check if we are busy - X first
	int retX = SendSerialCommand(port_.c_str(), cmdX, "\r");//or is term "R"?
	if (retX != DEVICE_OK)
		return retX;
  
	string responseX;
	retX = GetSerialAnswer(port_.c_str(), "\r", responseX);

	//check if we are busy - now Y
	int retY = SendSerialCommand(port_.c_str(), cmdY, "\r");//maybe wrong term?
	if (retY != DEVICE_OK)
		return retY;

	string responseY;
	retY = GetSerialAnswer(port_.c_str(), "\r", responseY);

	//return the answer
	long xStep, yStep;
	int answer = GetPositionSteps(xStep, yStep);
	if (answer != DEVICE_OK)
		return answer;
	originX_ = xStep * stepSizeXUm_;
	originY_ = yStep * stepSizeYUm_;
	
	return DEVICE_OK;
	
}

int CytoTableXYStage::Home()
{
	PurgeComPort(port_.c_str());

	
//some other stuff goes in here
return DEVICE_OK;
}

int CytoTableXYStage::Stop()
{
	PurgeComPort(port_.c_str());

	//give the command
	const char* cmdX = "/1TR";
	const char* cmdY = "/2TR";
	
	//check to make sure it worked - check that X stopped first
	int retX = SendSerialCommand(port_.c_str(), cmdX, "\r"); //is R the correct terminating character? or is it \r?
	if (retX != DEVICE_OK)
		return retX;

	string responseX;
	retX = GetSerialAnswer(port_.c_str(), "\r", responseX); //correct terminating char?

   //check that Y stopped
	int retY = SendSerialCommand(port_.c_str(), cmdY, "\r"); //correct terminating character? 
	if (retY != DEVICE_OK)
		return retY;

	string responseY;
	retY = GetSerialAnswer(port_.c_str(), "\r", responseY); //correct terminating char?
	
	return DEVICE_OK;
}

int CytoTableXYStage::GetStepLimits(long& /*xMin*/, long& /*xMax*/, long& /*yMin*/, long& /*yMax*/)
{
	return DEVICE_UNSUPPORTED_COMMAND;
}

int CytoTableXYStage::GetLimitsUm(double& /*xMin*/, double& /*xMax*/, double& /*yMin*/, double& /*yMax*/)
{
	return DEVICE_UNSUPPORTED_COMMAND;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
int CytoTableXYStage::OnStepSizeX(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
      pProp->Set(stepSizeXUm_);
	}
	else if (eAct == MM::AfterSet)
	{
      double stepSize;
      pProp->Get(stepSize);
      if (stepSize <=0.0)
      {
         pProp->Set(stepSizeXUm_);
         return ERR_INVALID_STEP_SIZE;
      }
      stepSizeXUm_ = stepSize;
	}

	return DEVICE_OK;
}

int CytoTableXYStage::OnStepSizeY(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(stepSizeYUm_);
   }
   else if (eAct == MM::AfterSet)
   {
      double stepSize;
      pProp->Get(stepSize);
      if (stepSize <=0.0)
      {
         pProp->Set(stepSizeYUm_);
         return ERR_INVALID_STEP_SIZE;
      }
      stepSizeYUm_ = stepSize;
   }

   return DEVICE_OK;
}


int CytoTableXYStage::OnSpeed(MM::PropertyBase* pProp, MM::ActionType eAct)
{
//
	return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
//Z Stage
///////////////////////////////////////////////////////////////////////////////
ZStage::ZStage() :
   initialized_(false),
   stepSizeUm_(0.1)
{
	InitializeDefaultErrorMessages();
	SetErrorText(ERR_NO_HUB, "Please add the Hub device first!");

   // Name
   CreateProperty(MM::g_Keyword_Name, g_ZStageDeviceName, MM::String, true);

   // Description
   CreateProperty(MM::g_Keyword_Description, "Z stage driver adapter", MM::String, true);

   // Axis ID
   id_ = "Z";
   CPropertyAction* pAct = new CPropertyAction(this, &ZStage::OnID);
   CreateProperty(g_Axis_Id, id_.c_str(), MM::String, false, pAct, true); 
   //Need to figure out this for Cyto
   /*AddAllowedValue(g_Axis_Id, "X");
   AddAllowedValue(g_Axis_Id, "Y");
   AddAllowedValue(g_Axis_Id, "Z");
   AddAllowedValue(g_Axis_Id, "R");
   AddAllowedValue(g_Axis_Id, "T");
   AddAllowedValue(g_Axis_Id, "F");
   AddAllowedValue(g_Axis_Id, "A");
   AddAllowedValue(g_Axis_Id, "B");
   AddAllowedValue(g_Axis_Id, "C");*/
}

ZStage::~ZStage()
{
   Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
// Stage methods required by the API
///////////////////////////////////////////////////////////////////////////////

void ZStage::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_ZStageDeviceName);
}


int ZStage::Initialize()
{
	// Position
	CPropertyAction* pAct = new CPropertyAction (this, &ZStage::OnStepSize);
	int ret = CreateProperty("StepSize", "1.0", MM::Float, false, pAct);
	if (ret != DEVICE_OK)
		return ret;

	ret = UpdateStatus();
	if (ret != DEVICE_OK)
		return ret;

	initialized_ = true;
	return DEVICE_OK;
}

int ZStage::Shutdown()
{
   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

bool ZStage::Busy()
{
//
	return false;
}

int ZStage::SetPositionUm(double pos)
{
	pos;
	return DEVICE_OK;
}

int ZStage::GetPositionUm(double& pos)
{
	pos;
	return DEVICE_OK;
}

int ZStage::SetPositionSteps(long steps)
{
	steps;
	return DEVICE_OK;
}

int ZStage::GetPositionSteps(long& steps)
{
	steps;
	return DEVICE_OK;
}

int ZStage::SetOrigin()
{
	return DEVICE_UNSUPPORTED_COMMAND;
}

int ZStage::GetLimits(double& /*min*/, double& /*max*/)
{
   return DEVICE_UNSUPPORTED_COMMAND;
}

int ZStage::OnStepSize(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
      pProp->Set(stepSizeUm_);
	}
   else if (eAct == MM::AfterSet)
	{
      double stepSize;
      pProp->Get(stepSize);
      if (stepSize <=0.0)
      {
         pProp->Set(stepSizeUm_);
         return ERR_INVALID_STEP_SIZE;
      }
      stepSizeUm_ = stepSize;
	}

   return DEVICE_OK;
}

int ZStage::OnID(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
      pProp->Set(id_.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
      string id;
      pProp->Get(id_);
      // Only allow axis that we know:
      if (id == "X" || id == "Y" || id == "Z")
         id_ = id;
	}

   return DEVICE_OK;
}
