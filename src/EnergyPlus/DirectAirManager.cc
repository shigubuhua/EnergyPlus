// EnergyPlus, Copyright (c) 1996-2019, The Board of Trustees of the University of Illinois,
// The Regents of the University of California, through Lawrence Berkeley National Laboratory
// (subject to receipt of any required approvals from the U.S. Dept. of Energy), Oak Ridge
// National Laboratory, managed by UT-Battelle, Alliance for Sustainable Energy, LLC, and other
// contributors. All rights reserved.
//
// NOTICE: This Software was developed under funding from the U.S. Department of Energy and the
// U.S. Government consequently retains certain rights. As such, the U.S. Government has been
// granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable,
// worldwide license in the Software to reproduce, distribute copies to the public, prepare
// derivative works, and perform publicly and display publicly, and to permit others to do so.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this list of
//     conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice, this list of
//     conditions and the following disclaimer in the documentation and/or other materials
//     provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National Laboratory,
//     the University of Illinois, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without specific prior
//     written permission.
//
// (4) Use of EnergyPlus(TM) Name. If Licensee (i) distributes the software in stand-alone form
//     without changes from the version obtained under this License, or (ii) Licensee makes a
//     reference solely to the software portion of its product, Licensee must refer to the
//     software as "EnergyPlus version X" software, where "X" is the version number Licensee
//     obtained under this License and may not use a different name for the software. Except as
//     specifically required in this Section (4), Licensee shall not use in a company name, a
//     product name, in advertising, publicity, or other promotional activities any name, trade
//     name, trademark, logo, or other designation of "EnergyPlus", "E+", "e+" or confusingly
//     similar designation, without the U.S. Department of Energy's prior written consent.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// C++ Headers
#include <cmath>

// ObjexxFCL Headers
#include <ObjexxFCL/Fmath.hh>

// EnergyPlus Headers
#include <DataAirLoop.hh>
#include <AirflowNetwork/Elements.hpp>
#include <DataEnvironment.hh>
#include <DataHVACGlobals.hh>
#include <DataIPShortCuts.hh>
#include <DataLoopNode.hh>
#include <DataPrecisionGlobals.hh>
#include <DataSizing.hh>
#include <DataZoneEquipment.hh>
#include <DirectAirManager.hh>
#include <EMSManager.hh>
#include <General.hh>
#include <GeneralRoutines.hh>
#include <InputProcessing/InputProcessor.hh>
#include <NodeInputManager.hh>
#include <OutputProcessor.hh>
#include <Psychrometrics.hh>
#include <ReportSizingManager.hh>
#include <ScheduleManager.hh>
#include <SplitterComponent.hh>
#include <UtilityRoutines.hh>

namespace EnergyPlus {

namespace DirectAirManager {

    // Module containing the routines dealing with the DIRECT AIR
    // component.

    // MODULE INFORMATION:
    //       AUTHOR         Fred Buhl
    //       DATE WRITTEN   October 1999
    //       MODIFIED       Brent Griffith, May 2009 added EMS override control of flow rate
    //       RE-ENGINEERED  na

    // PURPOSE OF THIS MODULE:
    // To encapsulate the data and algorithms required to
    // simulate the direct air component  Direct air
    // is the component used to pass supply air directly
    // into a zone without any thermostatic control.

    // METHODOLOGY EMPLOYED:

    // REFERENCES: none

    // OTHER NOTES: none

    // USE STATEMENTS:
    // Use statements for data only modules
    // Using/Aliasing
    using namespace DataPrecisionGlobals;
    using namespace DataGlobals;
    using namespace ScheduleManager;
    using DataEnvironment::StdBaroPress;
    using DataEnvironment::StdRhoAir;
    using DataHVACGlobals::SmallAirVolFlow;
    using DataLoopNode::Node;

    // Data
    // MODULE PARAMETER DEFINITIONS:

    // Type declarations in DirectAir module

    // MODULE VARIABLE DECLARATIONS:
    int NumDirectAir(0);
    Array1D_bool CheckEquipName;

    namespace {
        // These were static variables within different functions. They were pulled out into the namespace
        // to facilitate easier unit testing of those functions.
        // These are purposefully not in the header file as an extern variable. No one outside of this should
        // use these. They are cleared by clear_state() for use by unit tests, but normal simulations should be unaffected.
        // This is purposefully in an anonymous namespace so nothing outside this implementation file can use it.
        bool GetDirectAirInputFlag(true);
        static bool MyOneTimeFlag(true);
        static bool ZoneEquipmentListChecked(false); // True after the Zone Equipment List has been checked for items
        static Array1D_bool MyEnvrnFlag;
        static Array1D_bool MySizeFlag;
        static Array1D_bool MyDirectAirInitFlag;
    } // namespace

    // SUBROUTINE SPECIFICATIONS FOR MODULE AirLoopSplitter

    // Object Data
    Array1D<DirectAirProps> DirectAir;

    void clear_state()
    {
        NumDirectAir = 0;
        CheckEquipName.deallocate();
        DirectAir.deallocate();
        GetDirectAirInputFlag = true;
        MyOneTimeFlag = true;
        ZoneEquipmentListChecked = false;
        MyEnvrnFlag.deallocate();
        MySizeFlag.deallocate();
        MyDirectAirInitFlag.deallocate();
    }

    // Functions

    void SimDirectAir(std::string const &EquipName,
                      int const ControlledZoneNum,
                      bool const FirstHVACIteration,
                      Real64 &SensOutputProvided,
                      Real64 &LatOutputProvided, // Latent output provided (kg/s), dehumidification = negative
                      int &CompIndex)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Fred Buhl
        //       DATE WRITTEN   Oct 1999
        //       MODIFIED       Don Shirey, Aug 2009, LatOutputProvided
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE
        // This subroutine manages Direct Air component simulation.
        // It is called from SimZoneEquipment.

        // METHODOLOGY EMPLOYED:

        // Using/Aliasing
        using General::TrimSigDigits;

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        int DirectAirNum;

        if (GetDirectAirInputFlag) { // First time subroutine has been entered
            GetDirectAirInput();
            GetDirectAirInputFlag = false;
        }

        // Find the correct Direct Air Equipment
        if (CompIndex == 0) {
            DirectAirNum = UtilityRoutines::FindItemInList(EquipName, DirectAir, &DirectAirProps::EquipID);
            if (DirectAirNum == 0) {
                ShowFatalError("SimDirectAir: Unit not found=" + EquipName);
            }
            CompIndex = DirectAirNum;
        } else {
            DirectAirNum = CompIndex;
            if (DirectAirNum > NumDirectAir || DirectAirNum < 1) {
                ShowFatalError("SimDirectAir:  Invalid CompIndex passed=" + TrimSigDigits(DirectAirNum) +
                               ", Number of Units=" + TrimSigDigits(NumDirectAir) + ", Entered Unit name=" + EquipName);
            }
            if (CheckEquipName(DirectAirNum)) {
                if (EquipName != DirectAir(DirectAirNum).EquipID) {
                    ShowFatalError("SimDirectAir: Invalid CompIndex passed=" + TrimSigDigits(DirectAirNum) + ", Unit name=" + EquipName +
                                   ", stored Unit Name for that index=" + DirectAir(DirectAirNum).EquipID);
                }
                CheckEquipName(DirectAirNum) = false;
            }
        }
        if (DirectAirNum <= 0) {
            ShowFatalError("SimDirectAir: Unit not found=" + EquipName);
        }

        // With the correct DirectAirNum to Initialize the system
        DataSizing::CurTermUnitSizingNum = DirectAir(DirectAirNum).TermUnitSizingNum;
        InitDirectAir(DirectAirNum, ControlledZoneNum, FirstHVACIteration);

        CalcDirectAir(DirectAirNum, ControlledZoneNum, SensOutputProvided, LatOutputProvided);

        // No Update

        // ReportDirectAir( DirectAirNum );
    }

    void GetDirectAirInput()
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Fred Buhl
        //       DATE WRITTEN   Mar 1999
        //       MODIFIED       na
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE
        // Input the AirLoopSplitter data and store it in the AirLoopSplitter array.

        // METHODOLOGY EMPLOYED:
        // Use the Get routines from the InputProcessor module.

        // Using/Aliasing
        using DataGlobals::AnyEnergyManagementSystemInModel;
        using DataGlobals::ScheduleAlwaysOn;
        using DataZoneEquipment::ZoneEquipConfig;
        using NodeInputManager::GetOnlySingleNode;
        using SplitterComponent::NumSplitters;
        using SplitterComponent::SplitterCond;
        using namespace DataLoopNode;
        using namespace DataIPShortCuts;

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:    INTEGER :: BaseboardNum
        int NumNums;   // Number of REAL(r64) numbers returned by GetObjectItem
        int NumAlphas; // Number of alphanumerics returned by GetObjectItem
        int DirectAirNum;
        int IOStat;
        static std::string const RoutineName("GetDirectAirInput: "); // include trailing blank space
        static bool ErrorsFound(false);
        int Loop;     // Do Loop Index
        int CtrlZone; // controlled zome do loop index
        int SupAirIn; // controlled zone supply air inlet index
        int NodeNum;
        int SplitNum;

        cCurrentModuleObject = "AirTerminal:SingleDuct:Uncontrolled";

        NumDirectAir = inputProcessor->getNumObjectsFound(cCurrentModuleObject);

        if (NumDirectAir > 0) {

            DirectAir.allocate(NumDirectAir);
            CheckEquipName.dimension(NumDirectAir, true);

            for (DirectAirNum = 1; DirectAirNum <= NumDirectAir; ++DirectAirNum) {
                DirectAir(DirectAirNum).cObjectName = cCurrentModuleObject; // push Object Name into data array
                inputProcessor->getObjectItem(cCurrentModuleObject,
                                              DirectAirNum,
                                              cAlphaArgs,
                                              NumAlphas,
                                              rNumericArgs,
                                              NumNums,
                                              IOStat,
                                              lNumericFieldBlanks,
                                              lAlphaFieldBlanks,
                                              cAlphaFieldNames,
                                              cNumericFieldNames);
                UtilityRoutines::IsNameEmpty(cAlphaArgs(1), cCurrentModuleObject, ErrorsFound);
                DirectAir(DirectAirNum).EquipID = cAlphaArgs(1);
                DirectAir(DirectAirNum).Schedule = cAlphaArgs(2);
                if (lAlphaFieldBlanks(2)) {
                    DirectAir(DirectAirNum).SchedPtr = ScheduleAlwaysOn;
                } else {
                    DirectAir(DirectAirNum).SchedPtr = GetScheduleIndex(cAlphaArgs(2));
                    if (DirectAir(DirectAirNum).SchedPtr == 0) {
                        ShowSevereError(RoutineName + cCurrentModuleObject + "=\"" + cAlphaArgs(1) + "\", invalid data.");
                        ShowContinueError("..invalid " + cAlphaFieldNames(2) + "=\"" + cAlphaArgs(2) + "\".");
                        ErrorsFound = true;
                    }
                }
                // Direct air is a problem for node connections since it only has a single node
                // make this an outlet
                DirectAir(DirectAirNum).ZoneSupplyAirNode = GetOnlySingleNode(cAlphaArgs(3),
                                                                              ErrorsFound,
                                                                              cCurrentModuleObject,
                                                                              cAlphaArgs(1),
                                                                              NodeType_Air,
                                                                              NodeConnectionType_Outlet,
                                                                              1,
                                                                              ObjectIsNotParent,
                                                                              cAlphaFieldNames(3));
                // Load the maximum volume flow rate
                DirectAir(DirectAirNum).MaxAirVolFlowRate = rNumericArgs(1);

                if (lAlphaFieldBlanks(4)) {
                    DirectAir(DirectAirNum).NoOAFlowInputFromUser = true;
                } else {
                    DirectAir(DirectAirNum).OARequirementsPtr = UtilityRoutines::FindItemInList(cAlphaArgs(4), DataSizing::OARequirements);
                    if (DirectAir(DirectAirNum).OARequirementsPtr == 0) {
                        ShowSevereError(RoutineName + cCurrentModuleObject + "=\"" + cAlphaArgs(1) + "\", invalid data.");
                        ShowContinueError("..invalid " + cAlphaFieldNames(4) + "=\"" + cAlphaArgs(4) + "\".");
                        ErrorsFound = true;
                    } else {
                        DirectAir(DirectAirNum).NoOAFlowInputFromUser = false;
                    }
                }

                if (lAlphaFieldBlanks(5)) {
                    DirectAir(DirectAirNum).OAPerPersonMode = DataZoneEquipment::PerPersonDCVByCurrentLevel;
                } else {
                    if (cAlphaArgs(5) == "CURRENTOCCUPANCY") {
                        DirectAir(DirectAirNum).OAPerPersonMode = DataZoneEquipment::PerPersonDCVByCurrentLevel;
                    } else if (cAlphaArgs(5) == "DESIGNOCCUPANCY") {
                        DirectAir(DirectAirNum).OAPerPersonMode = DataZoneEquipment::PerPersonByDesignLevel;
                    } else {
                        DirectAir(DirectAirNum).OAPerPersonMode = DataZoneEquipment::PerPersonDCVByCurrentLevel;
                        ShowWarningError(RoutineName + cCurrentModuleObject + "=\"" + cAlphaArgs(1) + "\", invalid data.");
                        ShowContinueError("..invalid " + cAlphaFieldNames(5) + "=\"" + cAlphaArgs(5) +
                                          "\". The default input of CurrentOccupancy is assigned");
                    }
                }

                // DesignSpecification:AirTerminal:Sizing name
                DirectAir(DirectAirNum).AirTerminalSizingSpecIndex = 0;
                if (!lAlphaFieldBlanks(6)) {
                    DirectAir(DirectAirNum).AirTerminalSizingSpecIndex =
                        UtilityRoutines::FindItemInList(cAlphaArgs(6), DataSizing::AirTerminalSizingSpec);
                    if (DirectAir(DirectAirNum).AirTerminalSizingSpecIndex == 0) {
                        ShowSevereError(cAlphaFieldNames(6) + " = " + cAlphaArgs(6) + " not found.");
                        ShowContinueError("Occurs in " + cCurrentModuleObject + " = " + DirectAir(DirectAirNum).cObjectName);
                        ErrorsFound = true;
                    }
                }

                // Fill the Zone Equipment data with the supply air inlet node number of this unit.
                for (CtrlZone = 1; CtrlZone <= NumOfZones; ++CtrlZone) {
                    {
                        auto &thisZoneEqConfig(ZoneEquipConfig(CtrlZone));
                        if (!thisZoneEqConfig.IsControlled) continue;
                        for (SupAirIn = 1; SupAirIn <= ZoneEquipConfig(CtrlZone).NumInletNodes; ++SupAirIn) {
                            if (DirectAir(DirectAirNum).ZoneSupplyAirNode == ZoneEquipConfig(CtrlZone).InletNode(SupAirIn)) {
                                thisZoneEqConfig.AirDistUnitCool(SupAirIn).InNode = DirectAir(DirectAirNum).ZoneSupplyAirNode;
                                thisZoneEqConfig.AirDistUnitCool(SupAirIn).OutNode = DirectAir(DirectAirNum).ZoneSupplyAirNode;
                                thisZoneEqConfig.InletNodeSDUNum(SupAirIn) = DirectAirNum;
                                DirectAir(DirectAirNum).TermUnitSizingNum = thisZoneEqConfig.AirDistUnitCool(SupAirIn).TermUnitSizingIndex;
                                DirectAir(DirectAirNum).ZoneEqNum = CtrlZone;
                                DirectAir(DirectAirNum).CtrlZoneInNodeIndex = SupAirIn;
                                {
                                    auto &thisTermUnitSizingData(DataSizing::TermUnitSizing(DirectAir(DirectAirNum).TermUnitSizingNum));
                                    thisTermUnitSizingData.ADUName = DirectAir(DirectAirNum).EquipID;
                                    // Fill TermUnitSizing with specs from DesignSpecification:AirTerminal:Sizing if there is one attached to this
                                    // terminal unit
                                    if (DirectAir(DirectAirNum).AirTerminalSizingSpecIndex > 0) {
                                        {
                                            auto const &thisAirTermSizingSpec(
                                                DataSizing::AirTerminalSizingSpec(DirectAir(DirectAirNum).AirTerminalSizingSpecIndex));
                                            thisTermUnitSizingData.SpecDesCoolSATRatio = thisAirTermSizingSpec.DesCoolSATRatio;
                                            thisTermUnitSizingData.SpecDesHeatSATRatio = thisAirTermSizingSpec.DesHeatSATRatio;
                                            thisTermUnitSizingData.SpecDesSensCoolingFrac = thisAirTermSizingSpec.DesSensCoolingFrac;
                                            thisTermUnitSizingData.SpecDesSensHeatingFrac = thisAirTermSizingSpec.DesSensHeatingFrac;
                                            thisTermUnitSizingData.SpecMinOAFrac = thisAirTermSizingSpec.MinOAFrac;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Find the Zone Equipment Inlet Node from the Supply Air Path Splitter
                for (SplitNum = 1; SplitNum <= NumSplitters; ++SplitNum) {
                    for (NodeNum = 1; NodeNum <= SplitterCond(SplitNum).NumOutletNodes; ++NodeNum) {
                        if (DirectAir(DirectAirNum).ZoneSupplyAirNode == SplitterCond(SplitNum).OutletNode(NodeNum)) {
                            DirectAir(DirectAirNum).ZoneEquipAirInletNode = SplitterCond(SplitNum).InletNode;
                            break;
                        }
                    }
                }

                // If no splitter, set Zone Equipment Inlet Node to the Zone Supply Air Node
                if (NumSplitters == 0) {
                    DirectAir(DirectAirNum).ZoneEquipAirInletNode = DirectAir(DirectAirNum).ZoneSupplyAirNode;
                }
            }
        }

        if (ErrorsFound) {
            ShowFatalError(RoutineName + "Errors found in input.  Program terminates.");
        }

        // Setup output for the Direct Air Units.  This allows a comparison with
        for (Loop = 1; Loop <= NumDirectAir; ++Loop) {
            SetupOutputVariable("Zone Air Terminal Sensible Heating Energy",
                                OutputProcessor::Unit::J,
                                DirectAir(Loop).HeatEnergy,
                                "System",
                                "Sum",
                                DirectAir(Loop).EquipID);
            SetupOutputVariable("Zone Air Terminal Sensible Cooling Energy",
                                OutputProcessor::Unit::J,
                                DirectAir(Loop).CoolEnergy,
                                "System",
                                "Sum",
                                DirectAir(Loop).EquipID);
            SetupOutputVariable("Zone Air Terminal Sensible Heating Rate",
                                OutputProcessor::Unit::W,
                                DirectAir(Loop).HeatRate,
                                "System",
                                "Average",
                                DirectAir(Loop).EquipID);
            SetupOutputVariable("Zone Air Terminal Sensible Cooling Rate",
                                OutputProcessor::Unit::W,
                                DirectAir(Loop).CoolRate,
                                "System",
                                "Average",
                                DirectAir(Loop).EquipID);

            if (AnyEnergyManagementSystemInModel) {
                SetupEMSActuator("AirTerminal:SingleDuct:Uncontrolled",
                                 DirectAir(Loop).EquipID,
                                 "Mass Flow Rate",
                                 "[kg/s]",
                                 DirectAir(Loop).EMSOverrideAirFlow,
                                 DirectAir(Loop).EMSMassFlowRateValue);
                SetupEMSInternalVariable("AirTerminal:SingleDuct:Uncontrolled Maximum Mass Flow Rate",
                                         DirectAir(Loop).EquipID,
                                         "[kg/s]",
                                         DirectAir(Loop).AirMassFlowRateMax);
            }
        }
    }

    // Beginning Initialization Section of the Module
    //******************************************************************************

    void InitDirectAir(int const DirectAirNum, int const ControlledZoneNum, bool const FirstHVACIteration)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Richard J. Liesen
        //       DATE WRITTEN   January 2001

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine is for  initializations of the Direct Air Components.

        // METHODOLOGY EMPLOYED:
        // Uses the status flags to trigger events.

        using DataZoneEquipment::CheckZoneEquipmentList;
        using DataZoneEquipment::ZoneEquipConfig;
        using DataZoneEquipment::ZoneEquipInputsFilled;
        using Psychrometrics::PsyRhoAirFnPbTdbW;

        int ZoneNode;
        int Loop;

        // Do the Begin Simulation initializations
        if (MyOneTimeFlag) {

            MyEnvrnFlag.allocate(NumDirectAir);
            MySizeFlag.allocate(NumDirectAir);
            MyDirectAirInitFlag.allocate(NumDirectAir);
            MyEnvrnFlag = true;
            MySizeFlag = true;
            MyDirectAirInitFlag = true;
            MyOneTimeFlag = false;
        }

        // need to check all direct air units to see if they are on Zone Equipment List or issue warning
        if (!ZoneEquipmentListChecked && ZoneEquipInputsFilled) {
            ZoneEquipmentListChecked = true;
            for (Loop = 1; Loop <= NumDirectAir; ++Loop) {
                if (CheckZoneEquipmentList(DirectAir(DirectAirNum).cObjectName, DirectAir(Loop).EquipID)) continue;
                ShowWarningError("InitDirectAir: [" + DirectAir(DirectAirNum).cObjectName + " = " + DirectAir(Loop).EquipID +
                                 "] is not on any ZoneHVAC:EquipmentList.  It will not be simulated.");
            }
        }
        if (MyDirectAirInitFlag(DirectAirNum)) {
            if (DirectAir(DirectAirNum).AirLoopNum == 0) {
                if ((ControlledZoneNum > 0) && (DirectAir(DirectAirNum).CtrlZoneInNodeIndex > 0)) {
                    DirectAir(DirectAirNum).AirLoopNum = DataZoneEquipment::ZoneEquipConfig(DirectAir(DirectAirNum).ZoneEqNum)
                                                             .InletNodeAirLoopNum(DirectAir(DirectAirNum).CtrlZoneInNodeIndex);
                }
            } else {
                MyDirectAirInitFlag(DirectAirNum) = false;
            }
        }

        if (!SysSizingCalc && MySizeFlag(DirectAirNum)) {

            SizeDirectAir(DirectAirNum);

            DirectAir(DirectAirNum).ZoneEqNum = ControlledZoneNum;
            DirectAir(DirectAirNum).ZoneNum = ZoneEquipConfig(ControlledZoneNum).ActualZoneNum;
            MySizeFlag(DirectAirNum) = false;
        }
        // Do the Begin Environment initializations
        if (BeginEnvrnFlag && MyEnvrnFlag(DirectAirNum)) {

            // Calculate the Max Mass flow rate using the air density at Standard Conditions
            DirectAir(DirectAirNum).AirMassFlowRateMax = DirectAir(DirectAirNum).MaxAirVolFlowRate * StdRhoAir;

            Node(DirectAir(DirectAirNum).ZoneSupplyAirNode).MassFlowRateMax = DirectAir(DirectAirNum).AirMassFlowRateMax;
            Node(DirectAir(DirectAirNum).ZoneSupplyAirNode).MassFlowRateMin = 0.0;

            MyEnvrnFlag(DirectAirNum) = false;
        }

        if (!BeginEnvrnFlag) {
            MyEnvrnFlag(DirectAirNum) = true;
        }

        // Set the ZoneNode number
        ZoneNode = DirectAir(DirectAirNum).ZoneSupplyAirNode;

        Real64 mDotFromOARequirement(0.0);
        if (!DirectAir(DirectAirNum).NoOAFlowInputFromUser) {
            mDotFromOARequirement = DirectAir(DirectAirNum).AirMassFlowRateMax;
            int airLoopNum(0);
            Real64 airLoopOAFrac(0.0);
            airLoopNum = DirectAir(DirectAirNum).AirLoopNum;
            if (airLoopNum > 0) {
                airLoopOAFrac = DataAirLoop::AirLoopFlow(airLoopNum).OAFrac;
                bool UseOccSchFlag = false;
                if (DirectAir(DirectAirNum).OAPerPersonMode == DataZoneEquipment::PerPersonDCVByCurrentLevel) UseOccSchFlag = true;
                if (airLoopOAFrac > 0.0) {
                    Real64 vDotOAReq = DataZoneEquipment::CalcDesignSpecificationOutdoorAir(
                        DirectAir(DirectAirNum).OARequirementsPtr, DirectAir(DirectAirNum).ZoneNum, UseOccSchFlag, true);
                    mDotFromOARequirement = vDotOAReq * DataEnvironment::StdRhoAir / airLoopOAFrac;
                    mDotFromOARequirement = min(mDotFromOARequirement, DirectAir(DirectAirNum).AirMassFlowRateMax);
                } else {
                    mDotFromOARequirement = DirectAir(DirectAirNum).AirMassFlowRateMax;
                }
            }
        }

        if (FirstHVACIteration) {
            // The first time through set the mass flow rate to the Max
            if ((Node(ZoneNode).MassFlowRateMaxAvail > 0.0) && (GetCurrentScheduleValue(DirectAir(DirectAirNum).SchedPtr) > 0.0)) {
                if (!(AirflowNetwork::SimulateAirflowNetwork > AirflowNetwork::AirflowNetworkControlMultizone &&
                      AirflowNetwork::AirflowNetworkFanActivated)) {
                    if (DirectAir(DirectAirNum).NoOAFlowInputFromUser) {
                        Node(ZoneNode).MassFlowRate = DirectAir(DirectAirNum).AirMassFlowRateMax;
                        Node(ZoneNode).MassFlowRateMaxAvail = DirectAir(DirectAirNum).AirMassFlowRateMax;
                    } else {
                        Node(ZoneNode).MassFlowRate = mDotFromOARequirement;
                        Node(ZoneNode).MassFlowRateMaxAvail = mDotFromOARequirement;
                    }

                    if (DirectAir(DirectAirNum).EMSOverrideAirFlow) Node(ZoneNode).MassFlowRate = DirectAir(DirectAirNum).EMSMassFlowRateValue;
                }
                Node(ZoneNode).MassFlowRateMinAvail = 0.0;
            } else {
                Node(ZoneNode).MassFlowRate = 0.0;
                Node(ZoneNode).MassFlowRateMaxAvail = 0.0;
            }
        } else {
            // When not FirstHCAVIteration
            if (!DirectAir(DirectAirNum).EMSOverrideAirFlow) {
                if ((Node(ZoneNode).MassFlowRateMaxAvail > 0.0) && (GetCurrentScheduleValue(DirectAir(DirectAirNum).SchedPtr) > 0.0)) {
                    if (DirectAir(DirectAirNum).NoOAFlowInputFromUser) {
                        if (Node(ZoneNode).MassFlowRateMaxAvail < Node(ZoneNode).MassFlowRateMax) {
                            Node(ZoneNode).MassFlowRate = Node(ZoneNode).MassFlowRateMaxAvail;
                        } else if (Node(ZoneNode).MassFlowRateMinAvail > Node(ZoneNode).MassFlowRateMin) {
                            Node(ZoneNode).MassFlowRate = Node(ZoneNode).MassFlowRateMinAvail;
                        } else {
                            Node(ZoneNode).MassFlowRate = Node(ZoneNode).MassFlowRateMaxAvail;
                        }
                    } else {
                        Node(ZoneNode).MassFlowRate = mDotFromOARequirement;
                        // but also apply constraints
                        Node(ZoneNode).MassFlowRate = min(Node(ZoneNode).MassFlowRate, Node(ZoneNode).MassFlowRateMaxAvail);
                        Node(ZoneNode).MassFlowRate = min(Node(ZoneNode).MassFlowRate, Node(ZoneNode).MassFlowRateMax);
                        Node(ZoneNode).MassFlowRate = max(Node(ZoneNode).MassFlowRate, Node(ZoneNode).MassFlowRateMinAvail);
                        Node(ZoneNode).MassFlowRate = max(Node(ZoneNode).MassFlowRate, Node(ZoneNode).MassFlowRateMin);
                    }
                } else {
                    Node(ZoneNode).MassFlowRate = 0.0;
                    Node(ZoneNode).MassFlowRateMaxAvail = 0.0;
                    Node(ZoneNode).MassFlowRateMinAvail = 0.0;
                }
            } else { // EMS override on
                Node(ZoneNode).MassFlowRate = DirectAir(DirectAirNum).EMSMassFlowRateValue;
                // but also apply constraints
                Node(ZoneNode).MassFlowRate = min(Node(ZoneNode).MassFlowRate, Node(ZoneNode).MassFlowRateMaxAvail);
                Node(ZoneNode).MassFlowRate = min(Node(ZoneNode).MassFlowRate, Node(ZoneNode).MassFlowRateMax);
                Node(ZoneNode).MassFlowRate = max(Node(ZoneNode).MassFlowRate, Node(ZoneNode).MassFlowRateMinAvail);
                Node(ZoneNode).MassFlowRate = max(Node(ZoneNode).MassFlowRate, Node(ZoneNode).MassFlowRateMin);
            }
        }

        // Set reporting varialbes to zero for the Direct Air Output
        DirectAir(DirectAirNum).HeatRate = 0.0;
        DirectAir(DirectAirNum).CoolRate = 0.0;
        DirectAir(DirectAirNum).HeatEnergy = 0.0;
        DirectAir(DirectAirNum).CoolEnergy = 0.0;
    }

    void SizeDirectAir(int const DirectAirNum)
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Fred Buhl
        //       DATE WRITTEN   January 2002
        //       MODIFIED       August 2013 Daeho Kang, add component sizing table entries
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE:
        // This subroutine is for sizing Direct Air Components for which flow rates have not been
        // specified in the input.

        // METHODOLOGY EMPLOYED:
        // Obtains flow rates from the zone arrays.

        // Using/Aliasing
        using namespace DataSizing;
        using General::RoundSigDigits;
        using ReportSizingManager::ReportSizingOutput;

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        Real64 MaxAirVolFlowRateDes;  // Design maximum air volume flow rate for reporting
        Real64 MaxAirVolFlowRateUser; // User hard-sized maximum air volume flow rate for reporting
        bool IsAutoSize;              // Indicator to autosizing max air flow rate
        bool SizingDesRunThisZone;

        IsAutoSize = false;
        MaxAirVolFlowRateDes = 0.0;
        MaxAirVolFlowRateUser = 0.0;
        SizingDesRunThisZone = false;

        if ((CurZoneEqNum > 0) && (CurTermUnitSizingNum > 0)) {

            if (DirectAir(DirectAirNum).MaxAirVolFlowRate == AutoSize) {
                IsAutoSize = true;
            }
            CheckThisZoneForSizing(CurZoneEqNum, SizingDesRunThisZone);

            // Check if all are hard-sized
            if (!IsAutoSize && !SizingDesRunThisZone) { // simulation should continue
                if (DirectAir(DirectAirNum).MaxAirVolFlowRate > 0.0) {
                    ReportSizingOutput(DirectAir(DirectAirNum).cObjectName,
                                       DirectAir(DirectAirNum).EquipID,
                                       "User-Specified Maximum Air Flow Rate [m3/s]",
                                       DirectAir(DirectAirNum).MaxAirVolFlowRate);
                }
            } else { // AutoSize or hard-size with design run
                CheckZoneSizing(DirectAir(DirectAirNum).cObjectName, DirectAir(DirectAirNum).EquipID);
                MaxAirVolFlowRateDes =
                    max(TermUnitFinalZoneSizing(CurTermUnitSizingNum).DesCoolVolFlow, TermUnitFinalZoneSizing(CurTermUnitSizingNum).DesHeatVolFlow);
                if (MaxAirVolFlowRateDes < SmallAirVolFlow) {
                    MaxAirVolFlowRateDes = 0.0;
                }
                if (IsAutoSize) {
                    DirectAir(DirectAirNum).MaxAirVolFlowRate = MaxAirVolFlowRateDes;
                    ReportSizingOutput(DirectAir(DirectAirNum).cObjectName,
                                       DirectAir(DirectAirNum).EquipID,
                                       "Design Size Maximum Air Flow Rate [m3/s]",
                                       MaxAirVolFlowRateDes);
                } else { // Hard-size with sizing data
                    if (DirectAir(DirectAirNum).MaxAirVolFlowRate > 0.0 && MaxAirVolFlowRateDes > 0.0 && SizingDesRunThisZone) {
                        MaxAirVolFlowRateUser = DirectAir(DirectAirNum).MaxAirVolFlowRate;
                        ReportSizingOutput(DirectAir(DirectAirNum).cObjectName,
                                           DirectAir(DirectAirNum).EquipID,
                                           "Design Size Maximum Air Flow Rate [m3/s]",
                                           MaxAirVolFlowRateDes,
                                           "User-Specified Maximum Air Flow Rate [m3/s]",
                                           MaxAirVolFlowRateUser);
                        if (DisplayExtraWarnings) {
                            if ((std::abs(MaxAirVolFlowRateDes - MaxAirVolFlowRateUser) / MaxAirVolFlowRateUser) > AutoVsHardSizingThreshold) {
                                ShowMessage("SizeDirectAir: Potential issue with equipment sizing for AirTerminal:SingleDuct:Uncontrolled=\"" +
                                            DirectAir(DirectAirNum).EquipID + "\".");
                                ShowContinueError("User-Specified Maximum Air Flow Rate of " + RoundSigDigits(MaxAirVolFlowRateUser, 5) + " [m3/s]");
                                ShowContinueError("differs from Design Size Maximum Air Flow Rate of " + RoundSigDigits(MaxAirVolFlowRateDes, 5) +
                                                  " [m3/s]");
                                ShowContinueError("This may, or may not, indicate mismatched component sizes.");
                                ShowContinueError("Verify that the value entered is intended and is consistent with other components.");
                            }
                        }
                    }
                }
            }
        }
    }

    // End Initialization Section of the Module
    //******************************************************************************

    void CalcDirectAir(int const DirectAirNum,
                       int const ControlledZoneNum,
                       Real64 &SensOutputProvided,
                       Real64 &LatOutputProvided // Latent output provided, kg/s, dehumidification = negative
    )
    {

        // SUBROUTINE INFORMATION:
        //       AUTHOR         Fred Buhl
        //       DATE WRITTEN   Oct 1999
        //       MODIFIED       Shirey, Aug 2009 (LatOutputProvided)
        //       RE-ENGINEERED  na

        // PURPOSE OF THIS SUBROUTINE
        // Calculate the system SensOutputProvided and LatOutputProvided by the direct supply air connection

        // METHODOLOGY EMPLOYED:
        // Enthalpy balance

        // REFERENCES:

        // Using/Aliasing
        using DataHVACGlobals::SmallMassFlow;
        using DataZoneEquipment::ZoneEquipConfig;
        using Psychrometrics::PsyHFnTdbW;

        // Locals
        // SUBROUTINE ARGUMENT DEFINITIONS

        // SUBROUTINE PARAMETER DEFINITIONS:

        // INTERFACE BLOCK SPECIFICATIONS
        // na

        // DERIVED TYPE DEFINITIONS
        // na

        // SUBROUTINE LOCAL VARIABLE DECLARATIONS:
        Real64 MassFlowRate; // Air mass flow rate in kg/s
        Real64 SpecHumOut;   // Specific humidity ratio of outlet air (kg moisture / kg moist air)
        Real64 SpecHumIn;    // Specific humidity ratio of inlet [zone] air (kg moisture / kg moist air)

        // Sign convention: SysSensOutputProvided <0 Zone is cooled
        //                  SysSensOutputProvided >0 Zone is heated
        //                  SysLatOutputProvided <0 Zone is dehumidified
        //                  SysLatOutputProvided >0 Zone is humidified

        MassFlowRate = Node(DirectAir(DirectAirNum).ZoneSupplyAirNode).MassFlowRate;

        if (GetCurrentScheduleValue(DirectAir(DirectAirNum).SchedPtr) > 0.0 && MassFlowRate > SmallMassFlow) {

            //  Change this later ... should be using minimum humidity ratio in the calculation of enthalpy
            //    MinHumRat = MIN(Node(ZoneEquipConfig(ControlledZoneNum)%ZoneNode)%HumRat,   &
            //                         Node(DirectAir(DirectAirNum)%ZoneSupplyAirNode)%HumRat)

            SensOutputProvided =
                MassFlowRate *
                (PsyHFnTdbW(Node(DirectAir(DirectAirNum).ZoneSupplyAirNode).Temp, Node(ZoneEquipConfig(ControlledZoneNum).ZoneNode).HumRat) -
                 PsyHFnTdbW(Node(ZoneEquipConfig(ControlledZoneNum).ZoneNode).Temp, Node(ZoneEquipConfig(ControlledZoneNum).ZoneNode).HumRat));

            //   CR9155 Remove specific humidity calculations
            SpecHumOut = Node(DirectAir(DirectAirNum).ZoneSupplyAirNode).HumRat;
            SpecHumIn = Node(ZoneEquipConfig(ControlledZoneNum).ZoneNode).HumRat;

            LatOutputProvided = MassFlowRate * (SpecHumOut - SpecHumIn); // Latent rate, kg/s

        } else {
            SensOutputProvided = 0.0;
            LatOutputProvided = 0.0;
        }

        DirectAir(DirectAirNum).SensOutputProvided = SensOutputProvided;
    }

} // namespace DirectAirManager

} // namespace EnergyPlus
