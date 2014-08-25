///////////////////////////////////////////////////////////////////////////////
// FILE:          CytoWorksTable.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   CytoWorksTable device adapter.
//                
// AUTHOR:        Cristy Koebler, 8/2014
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
#include "../../MMDevice/ModuleInterface.h"
#include "../../MMDevice/DeviceUtils.h"
#include "../../MMDevice/DeviceBase.h"
#include <cstdio>
#include <math.h>
#include <string>
#include <sstream>
#include <iostream>

//constants
const char* g_XYStageDeviceName = "CytoTableXYStage";
//const char* g_ZStageDeviceName = "ZStage";
//remove comment when you do Z stage
////Marzhauser - when i want to implement the light
//const char* g_LED1Name = "LED100-1";
/////////////////////////////////////////////////////////////////////////////////
////Go thru these once I have done the methods
////Ludl code
////const char* g_Ludl_Reset = "Reset";
////const char* g_Ludl_Version = "Version";
////const char* g_Ludl_TransmissionDelay = "TransmissionDelay";
////const char* g_Ludl_Axis_Id = "LudlSingleAxisName";
//////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////
MODULE_API void InitializeModuleData()
{
	RegisterDevice(g_XYStageDeviceName, MM::XYStageDevice, "CytoTableXYStage");
	//RegisterDevice(g_ZStageDeviceName, MM::StageDevice, "ZStage");
	//remove comment when you do z stage
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
	if (deviceName == 0)
      return 0;
	if (strcmp(deviceName, g_XYStageDeviceName) == 0)
	{
		CytoTableXYStage* pCytoTableXYStage = new CytoTableXYStage();
		return pCytoTableXYStage;
	}
	/*else if (strcmp(deviceName, g_ZStageDeviceName) == 0) //possibly this line should just be an if stmt
	{
		ZStage* pZStage = new ZStage();
		return pZStage;
	}*/ //remove comment when you do the Zstage stuff!!

	return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

int clearPort(MM::Device& device, MM::Core& core, const char* port)
{
	//Clear the serial port
	const unsigned int bufSize = 255;
	unsigned char clear[bufSize];
	unsigned long read = bufSize;
	int ret;
	while ((int) read == bufSize)
	{
      ret = core.ReadFromSerial(&device, port, clear, bufSize, read);
      if (ret != DEVICE_OK)
         return ret;
	}
   return DEVICE_OK;
}

int getResult(MM::Device& device, MM::Core& core, const char* )
{ 
   const int bufSize = 255;
   char rec[bufSize];
   string result;
   int ret = core.GetSerialAnswer(&device, port_.c_str(), bufSize, rec, "\n");
   if (ret != DEVICE_OK) 
      return ret;
   result = rec;
   if ( (result.length() < 1) || (result[0] != ':') )
      return ERR_NO_ANSWER;
   if (result[1] == 'A') 
      return DEVICE_OK;
   if (result[1] == 'N')
   {
      int errorNr = atoi(result.substr(2,64).c_str());
      if (errorNr > 0)
         return errorNr;
      else
         return ERR_COMMAND_FAILED;
   }
   // We should never get here
   return ERR_UNRECOGNIZED_ANSWER;
}

//MAY NEED TO REVIEW LUDL HUB CODE...

///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// XYStage
// * XYStage - two axis stage device.
// * Note that this adapter uses two coordinate systems.  There is the adapters own coordinate
// * system with the X and Y axis going the 'Micro-Manager standard' direction
// * Then, there is the Ludl native system.  All functions using 'steps' use the Ludl system
// * All functions using Um use the Micro-Manager coordinate system (THIS IS NOTE INCLUDED WITH LUDL CODE)
//////////////////////////////////////////////////////////////////////////////
CytoTableXYStage::CytoTableXYStage() :
	initialized_(false), 
	stepSizeXUm_(0.0012), //Have to find out OUR step size
	stepSizeYUm_(0.0012), //Have to find out OUR step size
	stepSizeUm_(0.1), //Have to find out OUR step size
	speed_(2500.0), //Have to find out OUR speed
	startSpeed_(500.0), //Have to find out OUR speed
	accel_(75), //Have to find out OUR acceleration
	idX_(1), //Using ludl values for now...guessing these are correct
	idY_(2), //Using ludl values for now...guessing these are correct
//The rest of the values across various machines...not sure what i really need here
//ASI
   /*maxSpeed_ (7.5),
   motorOn_(true),
   nrMoveRepetitions_(0),
   answerTimeoutMs_(1000)*/
	originX_(0),
	originY_(0)
{
	InitializeDefaultErrorMessages();
	// create pre-initialization properties

	CreateProperty(MM::g_Keyword_Name, g_XYStageDeviceName, MM::String, true);

	//Description
	CreateProperty(MM::g_Keyword_Description, "CytoWorksTable XY stage driver adapter", MM::String, true);

	//FYI, some of the adapters have a section for the Port...I'm leaving it out for now, but should be aware of it
	//Ludl doesn't have it, but Marz, Prior, & ASI do
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

//Wow...this one needs a LOT of work!!
int CytoTableXYStage::Initialize()
//FYI, this method is sort of a mess...all the DA's are all over the place here...
{
	//Not really sure where to start. Each one has a slightly different path here. I'm currently using Ludl
	// Step size
   CPropertyAction* pAct = new CPropertyAction (this, &CytoTableXYStage::OnStepSize);
   int ret = CreateProperty("StepSize", "1.0", MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Speed (in um/sec)
   pAct = new CPropertyAction (this, &CytoTableXYStage::OnSpeed);
   ret = CreateProperty("Speed", "2500.0", MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Initial Speed (in um/sec) (i.e. lowest speed)
   pAct = new CPropertyAction (this, &CytoTableXYStage::OnStartSpeed);
   ret = CreateProperty("StartSpeed", "500.0", MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Accel (Acceleration is a number between 1 and 255, lower numbers give shorter ramp time, i.e. faster acceleration
   pAct = new CPropertyAction (this, &CytoTableXYStage::OnAccel);
   ret = CreateProperty("Acceleration", "100", MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   //FYI, MARZ has backlash info when you're ready to add it
   
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

//Returns true if an axis is busy
bool CytoTableXYStage::Busy()
{
	if (AxisBusy("X") || AxisBusy("Y"))
      return true;
	return false;
}

//Returns true if any axis (X or Y) is still moving.
//Oof...this one is a doozy...not sure if it works...
bool CytoTableXYStage::AxisBusy(const char* axis)
{
    clearPort(*this, *GetCoreCallback(), port_.c_str());		
	ostringstream cmd;
	cmd << "STATUS " << axis;
 
	int ret = SendSerialCommand(port_.c_str(), cmd.str().c_str(), "\r");
	if (ret != DEVICE_OK)
	{
      ostringstream os;
      os << "SendSerialCommand failed in XYStage::Busy, error code:" << ret;
      this->LogMessage(os.str().c_str(), false);
      // return false; // can't write, continue just so that we can read an answer in case write succeeded even though we received an error
	}

   // Poll for the response to the Busy status request
	unsigned char status=0;
	unsigned long read=0;
	int numTries=0, maxTries=400;
	long pollIntervalMs = 5;
	this->LogMessage("Starting read in XY-Stage Busy", true);
	do {
      ret = ReadFromComPort(port_.c_str(), &status, 1, read);
      if (ret != DEVICE_OK)
      {
         ostringstream os;
         os << "ReadFromComPort failed in XYStage::Busy, error code:" << ret;
         this->LogMessage(os.str().c_str(), false);
         return false; // Error, let's pretend all is fine
      }
      numTries++;
      CDeviceUtils::SleepMs(pollIntervalMs);
	}
	while(read==0 && numTries < maxTries); // keep trying up to 2 sec
	ostringstream os;
	os << "Tried reading "<< numTries << " times, and finally read " << read << " chars";
	this->LogMessage(os.str().c_str(), true);

	if (status == 'B')
      return true;
	else
      return false;
}


//Safe assumption to say all the following methods are not final...i'm totally guessing from here on out
int CytoTableXYStage::SetPositionSteps(long x, long y)//This is from Ludl
{
	ostringstream command;
	command << "MOVE ";
	command << "X=" << x << " ";
	command << "Y=" << y ;

	int ret = SendSerialCommand(port_.c_str(), command.str().c_str(), "\r");
		if (ret != DEVICE_OK)
			return ret;

	string resp;
	ret = GetSerialAnswer(port_.c_str(), "\n", resp);
		if (ret != DEVICE_OK) 
			return ret;
		if (resp.length() < 1)
			return ERR_NO_ANSWER;

	//returning an answer
	istringstream is(resp);
	string outcome;
	is >> outcome;

   if (outcome.compare(":A") == 0)
      return DEVICE_OK; // success!

   // error code
   int code;
   is >> code;
   return code;
}

int CytoTableXYStage::SetRelativePositionSteps(long x, long y)
{
   ostringstream cmd;
   cmd << "MOVREL ";
   cmd << "X=" << x << " ";
   cmd << "Y=" << y ;

   int ret = SendSerialCommand(port_.c_str(), cmd.str().c_str(), "\r");
   if (ret != DEVICE_OK)
      return ret;
  
   string resp;
   ret = GetSerialAnswer(port_.c_str(), "\n", resp);
   if (ret != DEVICE_OK) 
      return ret;
   if (resp.length() < 1)
      return ERR_NO_ANSWER;

   istringstream is(resp);
   string outcome;
   is >> outcome;

   if (outcome.compare(":A") == 0)
      return DEVICE_OK; // success!

   // figure out the error code
   int code;
   is >> code;
   return code;
}

// Returns current position in steps.
int CytoTableXYStage::GetPositionSteps(long& x, long& y)
{
   PurgeComPort(port_.c_str());
   
   const char* cmd = "WHERE X Y";

   int ret = SendSerialCommand(port_.c_str(), cmd, "\r");
   if (ret != DEVICE_OK)
      return ret;
  
   string resp;
   ret = GetSerialAnswer(port_.c_str(), "\n", resp);
   if (ret != DEVICE_OK) 
      return ret;
   if (resp.length() < 1)
      return ERR_NO_ANSWER;

   istringstream is(resp);
   string outcome;
   is >> outcome;

   if (outcome.compare(":A") == 0)
   {
      is >> x;
      is >> y;
      return DEVICE_OK; // success!
   }

   // figure out the error code
   int code;
   is >> code;
   return code;
}

// Defines current position as origin (0,0) coordinate
int CytoTableXYStage::SetOrigin()
{
   PurgeComPort(port_.c_str());

   const char* cmd = "HERE X=0 Y=0";

   int ret = SendSerialCommand(port_.c_str(), cmd, "\r");
   if (ret != DEVICE_OK)
      return ret;

   string resp;
   ret = GetSerialAnswer(port_.c_str(), "\n", resp);

   return SetAdapterOrigin();
}

/**
 * Defines current position as origin (0,0) coordinate of our coordinate system
 * Get the current (stage-native) XY position
 * This is going to be the origin in our coordinate system
 * The Ludl stage X axis is the same orientation as out coordinate system, the Y axis is reversed
 */
int CytoTableXYStage::SetAdapterOrigin()
{
   long xStep, yStep;
   int ret = GetPositionSteps(xStep, yStep);
   if (ret != DEVICE_OK)
      return ret;
   originX_ = xStep * stepSizeUm_;
   originY_ = yStep * stepSizeUm_;

   return DEVICE_OK;
}

int CytoTableXYStage::Home()
{
   PurgeComPort(port_.c_str());

   const char* cmd = "HOME X Y";

   int ret = SendSerialCommand(port_.c_str(), cmd, "\r");
   if (ret != DEVICE_OK)
      return ret;
  
   //set a separate time-out since it can take a long time for the home command to complete
   unsigned char status=0;
   unsigned long read=0;
   int numTries=0, maxTries=200;
   long pollIntervalMs = 50;
   this->LogMessage("Starting read in XY-Stage HOME\n", true);
   do {
      ret = ReadFromComPort(port_.c_str(), &status, 1, read);
      if (ret != DEVICE_OK)
      {
         ostringstream os;
         os << "ReadFromComPort failed in XYStage::Busy, error code:" << ret;
         this->LogMessage(os.str().c_str(), false);
         return false; // Error, let's pretend all is fine
      }
      numTries++;
      CDeviceUtils::SleepMs(pollIntervalMs);
   }
   while(read==0 && numTries < maxTries); // keep trying up to 2 sec
   ostringstream os;
   os << "Tried reading "<< numTries << " times, and finally read " << read << " chars";
   this->LogMessage(os.str().c_str());

   string resp;
   ret = GetSerialAnswer(port_.c_str(), "\n", resp);
   
   return DEVICE_OK;
}

int CytoTableXYStage::Stop()
{
   PurgeComPort(port_.c_str());

   const char* cmd = "HALT";

   int ret = SendSerialCommand(port_.c_str(), cmd, "\r");
   if (ret != DEVICE_OK)
      return ret;
  
   string resp;
   ret = GetSerialAnswer(port_.c_str(), "\n", resp);
   
   return DEVICE_OK;
}

//Returns the stage position limits in um.
int CytoTableXYStage::GetLimitsUm(double& /*xMin*/, double& /*xMax*/, double& /*yMin*/, double& /*yMax*/)
{
   return DEVICE_UNSUPPORTED_COMMAND;
}

int CytoTableXYStage::GetStepLimits(long& /*xMin*/, long& /*xMax*/, long& /*yMin*/, long& /*yMax*/)
{
   return DEVICE_UNSUPPORTED_COMMAND;
}

///////////////////////////////////////////////////////////////////////////////
// Internal, helper method
///////////////////////////////////////////////////////////////////////////////
// Sends a specified command to the controller

int CytoTableXYStage::ExecuteCommand(const string& cmd, string& response)
{
   clearPort(*this, *GetCoreCallback(), port_.c_str());
   if (DEVICE_OK != SendSerialCommand(port_.c_str(), cmd.c_str(), "\r"))
      return DEVICE_SERIAL_COMMAND_FAILED;

   int ret = GetSerialAnswer(port_.c_str(), "\n", response);
   if (ret != DEVICE_OK) 
      return ret;
   if (response.length() < 1)
      return ERR_NO_ANSWER;
   
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
// Handle changes and updates to property values.
///////////////////////////////////////////////////////////////////////////////
int CytoTableXYStage::OnStepSize(MM::PropertyBase* pProp, MM::ActionType eAct)
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

/*
 * Speed as returned by device is in pulses per second
 * We convert that to um per second using the factor stepSizeUm)
 */
int CytoTableXYStage::OnSpeed(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      string resp;
      int ret = ExecuteCommand("SPEED X Y", resp);
      if (ret != DEVICE_OK)
         return ret;
      if (resp.substr(0,2) != ":A")
         return ERR_COMMAND_FAILED;
      // tokenize on space:
      stringstream ss(resp);
      string buf;
      vector<string> tokens;
      while (ss >> buf)
         tokens.push_back(buf);
      long speedX = atol(tokens.at(1).c_str());
      // long speedY = atol(tokens.at(2).c_str());
      // TODO, deal with situation when these two are not the same
      speed_ = speedX * stepSizeUm_;
      pProp->Set(speed_);
   }
   else if (eAct == MM::AfterSet)
   {
      double uiSpeed; // Start Speed in um/sec
      long speed; // Speed in rotations per sec
      pProp->Get(uiSpeed);
      speed = (long)(uiSpeed / stepSizeUm_);
      if (speed < 85 ) speed = 85;
      if (speed > 2764800) speed = 276480;
      string resp;
      ostringstream os;
      os << "SPEED X=" << speed << " Y=" << speed;
      int ret = ExecuteCommand(os.str().c_str(), resp);
      if (ret != DEVICE_OK)
         return ret;
      if (resp.substr(0,2) != ":A")
         return ERR_COMMAND_FAILED;
      speed_ = speed * stepSizeUm_;
   }
   return DEVICE_OK;
}

int CytoTableXYStage::OnStartSpeed(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      string resp;
      int ret = ExecuteCommand("STSPEED X Y", resp);
      if (ret != DEVICE_OK)
         return ret;
      if (resp.substr(0,2) != ":A")
         return ERR_COMMAND_FAILED;
      // tokenize on space:
      stringstream ss(resp);
      string buf;
      vector<string> tokens;
      while (ss >> buf)
         tokens.push_back(buf);
      long speedX = atol(tokens.at(1).c_str());
      // long speedY = atol(tokens.at(2).c_str());
      // TODO, deal with situation when these two are not the same
      startSpeed_ = speedX * stepSizeUm_;
      pProp->Set(startSpeed_);
   }
   else if (eAct == MM::AfterSet)
   {
      double uiStartSpeed; // this is the Start Speed in um/sec
      long startSpeed; // startSpeed in rotations per sec
      pProp->Get(uiStartSpeed);
      startSpeed = (long)(uiStartSpeed / stepSizeUm_);
      if (startSpeed < 1000 ) startSpeed = 1000;
      if (startSpeed > 2764800) startSpeed = 276480;
      string resp;
      ostringstream os;
      os << "STSPEED X=" << startSpeed << " Y=" << startSpeed;
      int ret = ExecuteCommand(os.str().c_str(), resp);
      if (ret != DEVICE_OK)
         return ret;
      if (resp.substr(0,2) != ":A")
         return ERR_COMMAND_FAILED;
      startSpeed_ = startSpeed * stepSizeUm_;
   }
   return DEVICE_OK;
}

int CytoTableXYStage::OnAccel(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      string resp;
      int ret = ExecuteCommand("ACCEL X Y", resp);
      if (ret != DEVICE_OK)
         return ret;
      if (resp.substr(0,2) != ":A")
         return ERR_COMMAND_FAILED;
      // tokenize on space:
      stringstream ss(resp);
      string buf;
      vector<string> tokens;
      while (ss >> buf)
         tokens.push_back(buf);
      long accelX = atol(tokens.at(1).c_str());
      // long accelY = atol(tokens.at(2).c_str());
      // TODO, deal with situation when these two are not the same
      accel_ = accelX;
      pProp->Set(accel_);
   }
   else if (eAct == MM::AfterSet)
   {
      long accel;
      pProp->Get(accel);
      if (accel < 1 ) accel = 1;
      if (accel > 255) accel = 255;
      string resp;
      ostringstream os;
      os << "ACCEL X=" << accel << " Y=" << accel;
      int ret = ExecuteCommand(os.str().c_str(), resp);
      if (ret != DEVICE_OK)
         return ret;
      if (resp.substr(0,2) != ":A")
         return ERR_COMMAND_FAILED;
      accel_ = accel;
   }
   return DEVICE_OK;
}

int CytoTableXYStage::OnIDX(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set((long)idX_);
   }
   else if (eAct == MM::AfterSet)
   {
      long id;
      pProp->Get(id);
      idX_ = (unsigned) id;
   }

   return DEVICE_OK;
}

int CytoTableXYStage::OnIDY(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set((long)idY_);
   }
   else if (eAct == MM::AfterSet)
   {
      long id;
      pProp->Get(id);
      idY_ = (unsigned) id;
   }

   return DEVICE_OK;
}