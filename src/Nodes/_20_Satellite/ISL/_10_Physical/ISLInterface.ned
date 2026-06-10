package Nodes._20_Satellite.ISL._10_Physical;

import inet.queueing.base.PacketSinkBase;
import inet.queueing.contract.IActivePacketSink;


simple ISLInterface extends PacketSinkBase like IActivePacketSink
{
    parameters:
        displayStringTextFormat = default("processed %p pk (%l) dropped %d");
        @class("satellite::ISLInterface");
        //@display("i=block/app");

        @signal[packetSent](type=inet::Packet);
        @signal[packetReceived](type=inet::Packet);

    gates:
        // ISL gates
        input islIn;
        output islOut;

        input in @labels(pull);
        output out;
}
