#
# OMNeT++/OMNEST Makefile for $(LIB_PREFIX)florasat
#
# This file was generated with the command:
#  opp_makemake --make-so -f --deep -o florasat -O out -KINET4_5_2_PROJ=/home/ylnner/Documents/omnet++/omnetpp-6.3.0/samples/inet4.5_2 -DINET_IMPORT -I. -I/home/ylnner/Documents/LibTorch/libtorch/include -I/home/ylnner/Documents/LibTorch/libtorch/include/torch/csrc/api/include -I$$\(INET4_5_2_PROJ\)/src -L$$\(INET4_5_2_PROJ\)/src -ltorch -ltorch_cpu -lc10 -lINET$$\(D\)
#

# Name of target to be created (-o option)
TARGET_DIR = .
TARGET_NAME = $(LIB_PREFIX)florasat$(D)
TARGET = $(TARGET_NAME)$(SHARED_LIB_SUFFIX)
TARGET_FILES = $(TARGET_DIR)/$(TARGET)

# C++ include paths (with -I)
INCLUDE_PATH = -I. -I/home/ylnner/Documents/LibTorch/libtorch/include -I/home/ylnner/Documents/LibTorch/libtorch/include/torch/csrc/api/include -I$(INET4_5_2_PROJ)/src

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = $(LDFLAG_LIBPATH)$(INET4_5_2_PROJ)/src  -ltorch -ltorch_cpu -lc10 -lINET$(D)

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/Global/Antenna/CustomAntenna.o \
    $O/Global/Base/MLBox/Base_MLBox.o \
    $O/Global/Base/_20_Data_Link/Base_MacProtocol.o \
    $O/Global/Base/_30_Network/Base_Routing.o \
    $O/Global/Base/_30_Network/FailedLink.o \
    $O/Global/Base/_30_Network/Null_Routing.o \
    $O/Global/Base/_30_Network/Ter_Base_Routing.o \
    $O/Global/Base/_30_Network/Ter_Null_Routing.o \
    $O/Global/Base/_30_Network/packetprocessing/TagBasedPacketDelayer.o \
    $O/Global/Channel/CustomBandListening.o \
    $O/Global/Channel/CustomPHYAnalogModel.o \
    $O/Global/Channel/CustomPHYPathLoss.o \
    $O/Global/Channel/LoRaAnalogModel.o \
    $O/Global/Channel/LoRaBandListening.o \
    $O/Global/Channel/LoRaHataOkumura.o \
    $O/Global/Channel/LoRaLogNormalShadowing.o \
    $O/Global/Channel/LoRaModulation.o \
    $O/Global/Channel/LoRaPathLossOulu.o \
    $O/Global/Channel/Propagation/DtSIoTPropagation.o \
    $O/Global/Channel/Unitdisk/SatelliteUnitDiskTransmission.o \
    $O/Global/Channel/Unitdisk/SatelliteUnitDiskTransmitter.o \
    $O/Global/Constellation_Creator/ConstellationCreator.o \
    $O/Global/Energy/CustomEnergyConsumer.o \
    $O/Global/Medium/DSL/CustomMedium.o \
    $O/Global/Medium/DSL/LoRaMedium.o \
    $O/Global/Medium/DSL/LoRaMediumCache.o \
    $O/Global/Medium/DSL/LoRaNeighborCache.o \
    $O/Global/Mobility/INorad.o \
    $O/Global/Statistics/PacketRecorder.o \
    $O/Global/Statistics/TransmissionStatistics/TransmissionStatistics.o \
    $O/Global/Topology_Control/ConstellationTopologyControl.o \
    $O/Global/Topology_Control/TopologyControlBase.o \
    $O/Global/Transceiver/CustomReceiver.o \
    $O/Global/Transceiver/CustomReception.o \
    $O/Global/Transceiver/CustomTransmission.o \
    $O/Global/Transceiver/CustomTransmitter.o \
    $O/Global/Transceiver/LoRaReceiver.o \
    $O/Global/Transceiver/LoRaReception.o \
    $O/Global/Transceiver/LoRaTransmission.o \
    $O/Global/Transceiver/LoRaTransmitter.o \
    $O/Global/Transmission_Predictor/Transmission_Predictor.o \
    $O/Global/Utilities/CSVReader.o \
    $O/Global/Utilities/FloraSignals.o \
    $O/Global/Utilities/ISLDirection.o \
    $O/Global/Utilities/ISLState.o \
    $O/Global/Utilities/LayerForwarder.o \
    $O/Global/Utilities/PositionAwareBase.o \
    $O/Global/Utilities/Utils.o \
    $O/Global/Utilities/WalkerType.o \
    $O/Global/Utilities/Routing_Core/DijkstraShortestPath.o \
    $O/Global/Utilities/Routing_Core/MinHopCount.o \
    $O/Global/Utilities/libnorad/ccoord.o \
    $O/Global/Utilities/libnorad/cEcef.o \
    $O/Global/Utilities/libnorad/cEci.o \
    $O/Global/Utilities/libnorad/cJulian.o \
    $O/Global/Utilities/libnorad/cNoradBase.o \
    $O/Global/Utilities/libnorad/cNoradBaseA.o \
    $O/Global/Utilities/libnorad/cNoradSDP4.o \
    $O/Global/Utilities/libnorad/cNoradSGP4.o \
    $O/Global/Utilities/libnorad/cNoradSGP4A.o \
    $O/Global/Utilities/libnorad/cOrbit.o \
    $O/Global/Utilities/libnorad/cOrbitA.o \
    $O/Global/Utilities/libnorad/cSite.o \
    $O/Global/Utilities/libnorad/cTLE.o \
    $O/Global/Utilities/libnorad/cVector.o \
    $O/Global/Utilities/libnorad/globals.o \
    $O/Nodes/_10_Terminal/Ter.o \
    $O/Nodes/_10_Terminal/_10_Physical/Ter_Phy_CustomRadio.o \
    $O/Nodes/_10_Terminal/_10_Physical/Ter_Phy_LoRaRadio.o \
    $O/Nodes/_10_Terminal/_20_Data_Link/Ter_Dlk_KiWan.o \
    $O/Nodes/_10_Terminal/_20_Data_Link/Transmission_Predictor/Transmission_Predictor.o \
    $O/Nodes/_10_Terminal/_30_Network/GroundForwardingTable.o \
    $O/Nodes/_10_Terminal/_30_Network/Ter_Net_TerminalRouting.o \
    $O/Nodes/_10_Terminal/_30_Network/routing/ddra/Ter_Net_DDRARouting.o \
    $O/Nodes/_10_Terminal/_40_Application/Ter_App.o \
    $O/Nodes/_10_Terminal/_60_Mobility/Ter_Mob.o \
    $O/Nodes/_20_Satellite/Sat.o \
    $O/Nodes/_20_Satellite/DSL/_10_Physical/Sat_Dsl_Phy_CustomRadio.o \
    $O/Nodes/_20_Satellite/DSL/_10_Physical/Sat_Dsl_Phy_LoRaRadio.o \
    $O/Nodes/_20_Satellite/DSL/_20_Data_Link/Sat_Dsl_Dlk_kMac.o \
    $O/Nodes/_20_Satellite/ISL/_10_Physical/ISLInterface.o \
    $O/Nodes/_20_Satellite/_30_Network/ForwardingTable.o \
    $O/Nodes/_20_Satellite/_30_Network/HybridForwardingTable.o \
    $O/Nodes/_20_Satellite/_30_Network/Sat_Net_PacketHandler.o \
    $O/Nodes/_20_Satellite/_30_Network/Sat_Net_SatelliteRouting.o \
    $O/Nodes/_20_Satellite/_30_Network/Sat_Net_SdrModel.o \
    $O/Nodes/_20_Satellite/_30_Network/Sat_Net_StoreAndForward.o \
    $O/Nodes/_20_Satellite/_30_Network/Routing/ddra/Sat_Net_DDRARouting.o \
    $O/Nodes/_20_Satellite/_60_Mobility/Sat_Mob_NoradA.o \
    $O/Nodes/_20_Satellite/_60_Mobility/Sat_Mob_Orchestrator.o \
    $O/Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility.o \
    $O/Nodes/_20_Satellite/_60_Mobility/Sat_Mob_SatelliteMobility_Standalone.o \
    $O/Nodes/_30_Station/Sta.o \
    $O/Nodes/_30_Station/_10_Physical/Sta_Phy.o \
    $O/Nodes/_30_Station/_20_Data_Link/Sta_Dlk_Statistics.o \
    $O/Nodes/_30_Station/_30_Network/Sta_Net.o \
    $O/Nodes/_30_Station/_60_Mobility/Sta_Mob_StationMobility.o \
    $O/Global/Base/_30_Network/packetprocessing/DelayPacketTag_m.o \
    $O/Global/Messages/_10_Physical/LoRaPhyPreamble_m.o \
    $O/Global/Messages/_10_Physical/LoRaRadioControlInfo_m.o \
    $O/Global/Messages/_20_Data_Link/Base_MacFrame_m.o \
    $O/Global/Messages/_20_Data_Link/KiWanMacFrame_m.o \
    $O/Global/Messages/_20_Data_Link/LoRaTagInfo_m.o \
    $O/Global/Messages/_30_Network/DDRARoutingHeader_m.o \
    $O/Global/Messages/_30_Network/Net_CstRoutingTag_m.o \
    $O/Global/Messages/_30_Network/Net_DropPacketTag_m.o \
    $O/Global/Messages/_30_Network/Net_ProcessPacketTag_m.o \
    $O/Global/Messages/_30_Network/Net_QueueInsertionTimeTag_m.o \
    $O/Global/Messages/_30_Network/Net_StoreAndForward_m.o \
    $O/Global/Messages/_30_Network/SendAtTag_m.o \
    $O/Global/Messages/_30_Network/SendOnTag_m.o \
    $O/Global/Messages/_40_Application/LoRaAppPacket_m.o \
    $O/Global/Statistics/TransmissionStatistics/CstTransmissionStatisticsTag_m.o \
    $O/Global/Utilities/ISLDirection_m.o

# Message files
MSGFILES = \
    Global/Base/_30_Network/packetprocessing/DelayPacketTag.msg \
    Global/Messages/_10_Physical/LoRaPhyPreamble.msg \
    Global/Messages/_10_Physical/LoRaRadioControlInfo.msg \
    Global/Messages/_20_Data_Link/Base_MacFrame.msg \
    Global/Messages/_20_Data_Link/KiWanMacFrame.msg \
    Global/Messages/_20_Data_Link/LoRaTagInfo.msg \
    Global/Messages/_30_Network/DDRARoutingHeader.msg \
    Global/Messages/_30_Network/Net_CstRoutingTag.msg \
    Global/Messages/_30_Network/Net_DropPacketTag.msg \
    Global/Messages/_30_Network/Net_ProcessPacketTag.msg \
    Global/Messages/_30_Network/Net_QueueInsertionTimeTag.msg \
    Global/Messages/_30_Network/Net_StoreAndForward.msg \
    Global/Messages/_30_Network/SendAtTag.msg \
    Global/Messages/_30_Network/SendOnTag.msg \
    Global/Messages/_40_Application/LoRaAppPacket.msg \
    Global/Statistics/TransmissionStatistics/CstTransmissionStatisticsTag.msg \
    Global/Utilities/ISLDirection.msg

# SM files
SMFILES =

# Other makefile variables (-K)
INET4_5_2_PROJ=/home/ylnner/Documents/omnet++/omnetpp-6.3.0/samples/inet4.5_2

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
CONFIGFILE = $(shell opp_configfilepath)
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIBS = -loppenvir$D $(KERNEL_LIBS) $(SYS_LIBS)
ifneq ($(PLATFORM),win32)
LIBS += -Wl,-rpath,$(abspath $(INET4_5_2_PROJ)/src)
endif

COPTS = $(CFLAGS) $(IMPORT_DEFINES) -DINET_IMPORT $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE (if COPTS has changed since last build)
# and make the object files depend on it
COPTS_FILE = $O/.last-copts
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
  $(shell $(MKPATH) "$O")
  $(file >$(COPTS_FILE),$(COPTS))
endif

# On Windows, the target has additional file(s). An import lib and an optional debug symbol file is created too.
ifeq ($(PLATFORM),win32)
  TARGET_IMPLIB = $(TARGET_NAME)$(IMPLIB_SUFFIX)
  TARGET_IMPDEF = $(TARGET_NAME)$(IMPDEF_SUFFIX)
  TARGET_FILES+= $(TARGET_DIR)/$(TARGET_IMPLIB)
  LDFLAGS+= $(LDFLAG_IMPLIB)$O/$(TARGET_IMPLIB)
  OUT_TARGET_EXTRA_FILES = $O/$(TARGET_IMPLIB)
  ifeq ($(TOOLCHAIN_NAME),clang-msabi)
    ifeq ($(MODE),debug)
      TARGET_FILES+=$(TARGET_DIR)/$(TARGET_NAME).pdb
    endif
  else
    TARGET_FILES+= $(TARGET_DIR)/$(TARGET_IMPDEF)
    LDFLAGS+= $(LDFLAG_IMPDEF)$O/$(TARGET_IMPDEF)
    OUT_TARGET_EXTRA_FILES+= $O/$(TARGET_IMPDEF)
  endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
-include makefrag

#------------------------------------------------------------------------------

# Main target
all: $(TARGET_FILES)

$(TARGET_DIR)/% :: $O/%
	@mkdir -p $(TARGET_DIR)
	$(Q)$(LN) $< $@
ifeq ($(TOOLCHAIN_NAME),clang-msabi)
	-$(Q)-$(LN) $(<:%.dll=%.lib) $(@:%.dll=%.lib) 2>/dev/null

$O/$(TARGET_NAME).pdb: $O/$(TARGET)
endif

$O/$(TARGET) $(OUT_TARGET_EXTRA_FILES) &: $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile $(CONFIGFILE)
	@$(MKPATH) $O
	@echo Creating shared library: $@
	$(Q)$(SHLIB_LD) -o $O/$(TARGET) $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS) $(LDFLAGS)
	$(Q)$(SHLIB_POSTPROCESS) $O/$(TARGET)
ifeq ($(PLATFORM),win32)
	$(Q)llvm-ar d $O/$(TARGET_IMPLIB) $(TARGET) # WORKAROUND: throw away the first file from the archive to make the LLD generated import lib valid
endif

.PHONY: all clean cleanall depend msgheaders smheaders

# disabling all implicit rules
.SUFFIXES :
.PRECIOUS : %_m.h %_m.cc

$O/%.o: %.cc $(COPTS_FILE) | msgheaders smheaders
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc -MD -MP -MF $O/$(basename $<)_m.h.d $(MSGCOPTS) $<

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $<

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning $(TARGET)
	$(Q)-rm -rf $O
	$(Q)-rm -f $(TARGET_FILES)
	$(Q)-rm -f $(call opp_rwildcard, . , *_m.cc *_m.h *_sm.cc *_sm.h)

cleanall:
	$(Q)$(CLEANALL_COMMAND)
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

help:
	@echo "$$HELP_SYNOPSYS"
	@echo "$$HELP_TARGETS"
	@echo "$$HELP_VARIABLES"
	@echo "$$HELP_EXAMPLES"

# include all dependencies
-include $(OBJS:%=%.d) $(MSGFILES:%.msg=$O/%_m.h.d)
