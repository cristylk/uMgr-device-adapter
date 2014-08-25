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

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include <string>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// Error codes
//////////////////////////////////////////////////////////////////////////////
#define ERR_UNKNOWN_POSITION          10002
#define ERR_INVALID_SPEED             10003
#define ERR_PORT_CHANGE_FORBIDDEN     10004
#define ERR_SET_POSITION_FAILED       10005
#define ERR_INVALID_STEP_SIZE         10006
#define ERR_LOW_LEVEL_MODE_FAILED     10007
#define ERR_INVALID_MODE              10008
#define ERR_INVALID_ID                10009
#define ERR_UNRECOGNIZED_ANSWER       10010
#define ERR_INVALID_COMMAND_LEVEL     10013
#define ERR_MODULE_NOT_FOUND          10014
#define ERR_NO_ANSWER                 10017
#define ERR_COMMAND_FAILED            10021
#define ERR_INVALID_DEVICE_NUMBER     10023
#define ERR_DEVICE_CHANGE_NOT_ALLOWED 10024
#define ERR_NO_CONTROLLER             10027
#define ERR_UNSPECIFIED_ERROR         10010
#define ERR_HOME_REQUIRED             10011
#define ERR_INVALID_PACKET_LENGTH     10012
#define ERR_RESPONSE_TIMEOUT          10013
#define ERR_BUSY                      10014
#define ERR_STEPS_OUT_OF_RANGE        10015
#define ERR_STAGE_NOT_ZEROED          10016
#define ERR_NOT_LOCKED                10011
#define ERR_NOT_CALIBRATED            10012
#define ERR_OFFSET                    10100


// MMCore name of serial port
std::string port_ = "";

int clearPort(MM::Device& device, MM::Core& core, const char* port);

////////THIS WHOLE SECTION SHOULD BE DOUBLE CHECKED!! Going to uncomment for now, see if it works...
int getResult(MM::Device& device, MM::Core& core, const char* port);

//THERE ARE A FEW OTHER VARS AND METHODS I HAVEN'T MENTIONED HERE IN THE VARIOUS 
//DEVICE ADAPTERS. ONLY GO BACK AND ADD THEM IF THIS FILE DOESN'T WORK.
//Specifically there's stuff in Thorlabs that wasn't added.

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
		//These lines shared across adapters - delete this comment line later
		int SetPositionSteps(long x, long y);
		int GetPositionSteps(long& x, long& y);
		int SetRelativePositionSteps(long x, long y);
		int Home();
		int Stop();
		int SetOrigin();
		int SetAdapterOrigin();//THIS IS NOT IN ASI (keep it because it's in Ludl and Marz)
		int GetStepLimits(long& xMin, long& xMax, long& yMin, long& yMax);
		int GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax);
		double GetStepSizeXUm() {return stepSizeXUm_;}
		double GetStepSizeYUm() {return stepSizeYUm_;}
		int IsXYStageSequenceable(bool& isSequenceable) const {isSequenceable =	false; return DEVICE_OK;}
		
		//Following additional lines from Marzhauser - WILL NEED TO CHECK THESE OUT BEFORE ADDING
		/*int SetPositionUm(double x, double y);
		int SetRelativePositionUm(double dx, double dy);
		int SetAdapterOriginUm(double x, double y);
		int GetPositionUm(double& x, double& y);
		int Move(double vx, double vy); */
		//Following additional lines from ASI
		/*int Calibrate();
		int Calibrate1();*/
		//Previous additional lines - WILL NEED TO CHECK THESE OUT BEFORE ADDING

		// action interface
		int OnStepSize(MM::PropertyBase* pProp, MM::ActionType eAct);
		int OnStepSizeX		(MM::PropertyBase* pProp, MM::ActionType eAct);
		int OnStepSizeY		(MM::PropertyBase* pProp, MM::ActionType eAct);
		int OnSpeed			(MM::PropertyBase* pProp, MM::ActionType eAct);
		int OnStartSpeed	(MM::PropertyBase* pProp, MM::ActionType eAct);
		int OnAccel			(MM::PropertyBase* pProp, MM::ActionType eAct);
		//adding these two in for now, but not sure what they do...
		int OnIDX		(MM::PropertyBase* pProp, MM::ActionType eAct);
		int OnIDY		(MM::PropertyBase* pProp, MM::ActionType eAct);
		//Need to cobble together what i need from below - NONE OF THIS IS FINAL!!
////////////////////////////////////////////////////////////////////////////// 
////From Marzhauser
 //  int OnPort		(MM::PropertyBase* pProp, MM::ActionType eAct);
 //  int OnBacklashX	(MM::PropertyBase* pProp, MM::ActionType eAct);
 //  int OnBacklashY	(MM::PropertyBase* pProp, MM::ActionType eAct);
////From ASI
 //  int OnPort		(MM::PropertyBase* pProp, MM::ActionType eAct);

private: //FYI, most of these are from Ludl
	bool initialized_;
	double stepSizeUm_;
	double stepSizeXUm_;
	double stepSizeYUm_;
	double speed_; 
	double startSpeed_;
	long accel_;
	bool AxisBusy(const char* axis);
	double originX_;
	double originY_;
	//don't know what these two do, but getting errors if I don't have them
	unsigned idX_;
	unsigned idY_;
	//Check the previous two out to figure what they do
	int ExecuteCommand(const std::string& cmd, std::string& response);
//Need to cobble together what i need from below - NONE OF THIS IS FINAL!!
//////////////////////////////////////////////////////////////////////////////
//From Marz
   //bool range_measured_;
//////////////////////////////////////////////////////////////////////////////
};

//COMMENTING ALL Z STAGE STUFF OUT UNTIL I FINISH IT!!
//
//class ZStage : public CStageBase<ZStage>
//{
//public:
//	ZStage();
//	~ZStage();
//  
//   // Device API
//	int Initialize();
//	int Shutdown();
//  
//	void GetName(char* pszName) const;
//	bool Busy();
//
//   // Stage API
//	int SetPositionUm(double pos);
//	int GetPositionUm(double& pos);
//	int SetPositionSteps(long steps);
//	int GetPositionSteps(long& steps);
//	int SetOrigin();
//	int GetLimits(double& min, double& max);
//
//	bool IsContinuousFocusDrive() const {return false;} //This one's missing from Marz, not worried about that
//	
////////////////////////////////////////////////////////////////////////////////
////Following additional lines WILL NEED TO CHECK THESE OUT BEFORE ADDING
//  //From Marzhauser
//   /*int SetRelativePositionUm(double d);
//   int Move(double velocity);
//   int SetAdapterOriginUm(double d);
//   int SetAdapterOrigin();
//   int Stop();*/
//   //From ASI
//  /*int Calibrate();*/
////////////////////////////////////////////////////////////////////////////////  
//
//
//   // action interface
//	int IsStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}
//	int OnPort(MM::PropertyBase* pProp, MM::ActionType eAct); //When you see OnID from Ludl, that's what this is--same function
////Need to cobble together what i need from below - NONE OF THIS IS FINAL!! These are part of action interface
////////////////////////////////////////////////////////////////////////////////
// //  //From Ludl
// //  int OnStepSize	(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  int OnAutofocus	(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  //From Marzhauser
// //  int OnStepSize	(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  int OnSpeed		(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  int OnAccel		(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  int OnBacklash	(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  int OnSequence	(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  //From Prior
// //  int OnMaxSpeed	(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  int OnAcceleration(MM::PropertyBase* pProp, MM::ActionType eAct);
// //  int OnSCurve		(MM::PropertyBase* pProp, MM::ActionType eAct);
////From ASI
//// Sequence functions
//  /* int GetStageSequenceMaxLength(long& nrEvents) const  {nrEvents = nrEvents_; return DEVICE_OK;}
//   int StartStageSequence();
//   int StopStageSequence();
//   int ClearStageSequence();
//   int AddToStageSequence(double position);
//   int SendStageSequence();*/
////From Marz
//// Sequence functions
// /*  int GetStageSequenceMaxLength(long& nrEvents) const {nrEvents = nrEvents_; return DEVICE_OK;}
//   int StartStageSequence();
//   int StopStageSequence();
//   int ClearStageSequence();
//   int AddToStageSequence(double position);
//   int SendStageSequence();*/
////////////////////////////////////////////////////////////////////////////////
//
//private:
//   double stepSizeUm_;
//   int ExecuteCommand(const std::string& cmd, std::string& response);
//   int Autofocus(long param);
//   double answerTimeoutMs_;
//   bool initialized_;
////Need to cobble together what i need from below - NONE OF THIS IS FINAL!! These are part of private methods
////////////////////////////////////////////////////////////////////////////////
//	//Ludl
//   /*std::string id_;*/
//	//Marzhauser
//   /*bool range_measured_;
//   double speedZ_;
//   double accelZ_;
//   double originZ_;
//   bool sequenceable_;
//   long nrEvents_;
//   std::vector<double> sequence_;*/
//   //ASI
//   /*bool HasRingBuffer();
//   std::vector<double> sequence_;
//   std::string axis_;
//   unsigned int axisNr_;
//   bool sequenceable_;
//   bool hasRingBuffer_;
//   long nrEvents_;
//   long curSteps_;*/
//	//Prior
//   /*int GetResolution(double& res);
//   bool initialized_;
//   std::string port_;
//   long curSteps_;
//   bool HasCommand(std::string command);*/
//
//};

#endif //_CYTOWORKSTABLE_H_