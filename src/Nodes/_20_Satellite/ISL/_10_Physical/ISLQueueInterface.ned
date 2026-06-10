package Nodes._20_Satellite.ISL._10_Physical;

import inet.queueing.queue.DropTailQueue;

module ISLQueueInterface
{
    gates:
        input in;
        output out;

    submodules:
        queue: DropTailQueue {
            //@display("p=900,100");
        }
        islIf: ISLInterface;
        
    connections allowunconnected:
        in --> queue.in;
        queue.out --> islIf.in;
        islIf.islOut --> out;
}
