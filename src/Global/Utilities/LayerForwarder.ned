package Global.Utilities;

simple LayerForwarder
{
    parameters:
        string layerName @optional;
        @class(florasat::LayerForwarder);
        @display("i=block/process");
    gates:
        input upperIn @optional;
        output upperOut @optional;
        input lowerIn @optional;
        output lowerOut @optional;
}
