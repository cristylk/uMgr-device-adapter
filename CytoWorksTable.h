///////////////////////////////////////////////////////////////////////////////
// FILE:          CytoWorksTable.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Implementation of the CytoWorksTable device adapter.              
//                
// AUTHOR:        Cristy Koebler, 8/19/14.
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
//
#ifndef _CYTOWORKSTABLE_H_
#define _CYTOWORKSTABLE_H_

#include "../../../MMDevice/MMDevice.h"
#include "../../../MMDevice/DeviceBase.h"

#include <string>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//Will have to finalize these after all code is completed
//////////////////////////////////////////////////////////////////////////////
#define ERR_INVALID_SPEED             10003
#define ERR_PORT_CHANGE_FORBIDDEN     10004	 //Used in Hub:OnPort
#define ERR_SET_POSITION_FAILED       10005
#define ERR_INVALID_STEP_SIZE         10006  //Used in CytoTableXYStage()
#define ERR_LOW_LEVEL_MODE_FAILED     10007
#define ERR_INVALID_MODE              10008
#define ERR_INVALID_ID                10009
#define ERR_UNRECOGNIZED_ANSWER       10010
#define ERR_INVALID_COMMAND_LEVEL     10011
#define ERR_MODULE_NOT_FOUND          10012
#define ERR_NO_ANSWER                 10013 //Used in Hub, XY
#define ERR_COMMAND_FAILED            10014
#define ERR_INVALID_DEVICE_NUMBER     10015
#define ERR_DEVICE_CHANGE_NOT_ALLOWED 10016
#define ERR_NO_HUB		              10017 //Used in CytoTableXYStage(), ZStage()
#define ERR_UNSPECIFIED_ERROR         10018
#define ERR_HOME_REQUIRED             10019
#define ERR_INVALID_PACKET_LENGTH     10020
#define ERR_RESPONSE_TIMEOUT          10021
#define ERR_BUSY                      10022
#define ERR_STEPS_OUT_OF_RANGE        10023
#define ERR_STAGE_NOT_ZEROED          10024
#define ERR_NOT_LOCKED                10025
#define ERR_NOT_CALIBRATED            10026
#define ERR_OFFSET                    10100
#define ERR_SERIAL_COMMAND_FAILED     10101 //Used in Hub
#define ERR_NO_PORT_SET				  10102 //Used in Hub


// MMCore name of serial port
std::string port_ = "";

int clearPort(MM::Device& device, MM::Core& core, const char* port);

//It's possible that I will need these - not sure yet 11.10.14
//int getResult(MM::Device& device, MM::Core& core, const char* port);

class Hub : public HubBase<Hub>
{
   public:
	   Hub();
      ~Hub();

      // Device API
      // ---------
	  int Initialize();
      int Shutdown();
      void GetName(char* pszName) const;
      bool Busy();

      // device discovery
	  MM::DeviceDetectionStatus DetectDevice(void);
	  int DetectInstalledDevices();      
	  
	  // action interface
      int OnPort (MM::PropertyBase* pProp, MM::ActionType eAct);

   private:
      // Command exchange with MMCore
      std::string command_;
      bool initialized_;
	  int transmissionDelay_;
};

class CytoTableXYStage : public CXYStageBase<CytoTableXYStage>
{
public: 
	CytoTableXYStage();
	~CytoTableXYStage();

		// Device API
		int Initialize();
		int Shutdown();
		
		void GetName(char* pszName) const;
		bool Busy();

		// XYStage API
		int SetPositionSteps(long x, long y);
		int SetRelativePositionSteps(long x, long y);
		int GetPositionSteps(long& x, long& y);
		int SetOrigin();
		int Home();
		int Stop();
		int GetStepLimits(long& xMin, long& xMax, long& yMin, long& yMax);
		int GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax);
		double GetStepSizeXUm() {return stepSizeXUm_;}
		double GetStepSizeYUm() {return stepSizeYUm_;}
		int IsXYStageSequenceable(bool& isSequenceable) const {isSequenceable =	false; return			DEVICE_OK;}

		// action interface
		int OnStepSizeX		(MM::PropertyBase* pProp, MM::ActionType eAct);
		int OnStepSizeY		(MM::PropertyBase* pProp, MM::ActionType eAct);
		int OnSpeed			(MM::PropertyBase* pProp, MM::ActionType eAct);


private:
	bool initialized_;
	double stepSizeXUm_;
	double stepSizeYUm_;
	double speed_; 
	//bool AxisBusy(const char* axis);
	//double stepSizeUm_; Don't use this - always use separate x and y

	//These ones are up in the air for now - use if you need them
	//double startSpeed_; - only need this if you use OnStartSpeed
	//long accel_; - only need this if you use OnAccel
	double originX_; //- only need this if you use SetAdapterOrigin
	double originY_; //- only need this if you use SetAdapterOrigin
	//unsigned idX_; - only need this if you use OnIDX
	//unsigned idY_; - only need this if you use OnIDY
};

class ZStage : public CStageBase<ZStage>
{
	public:
		ZStage();
		~ZStage();
 
// Device API
	int Initialize();
	int Shutdown();
  
	void GetName(char* pszName) const;
	bool Busy();

   // Stage API
	int SetPositionUm(double pos);
	int GetPositionUm(double& pos);
	int SetPositionSteps(long steps);
	int GetPositionSteps(long& steps);
	int SetOrigin();
	int GetLimits(double& min, double& max);

	bool IsContinuousFocusDrive() const {return false;} 

   // action interface
	int OnID(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnStepSize	(MM::PropertyBase* pProp, MM::ActionType eAct);
	int IsStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}

	//This one i'm not sure - comes from ASI
	//int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct); //When you see OnID from Ludl, that's what this is--same function

private:
	bool initialized_;
	double stepSizeUm_;
	std::string id_;
   
};

#endif //_CYTOWORKSTABLE_H_
