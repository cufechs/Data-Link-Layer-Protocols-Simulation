package Project_GroupC;

@license(LGPL);

import ned.IdealChannel;

network Network
{
    parameters:
        int n = default(4);
        @display("bgb=435,283");
    submodules:
        nodes[n]: Node;
        parent: Parent {
            @display("is=l");
        }
    connections:
        for i=0..sizeof(nodes)-1, for j=i+1..sizeof(nodes)-1 {
            nodes[i].outs++ --> IdealChannel --> nodes[j].ins++;
            nodes[i].ins++ <-- IdealChannel <-- nodes[j].outs++;
        }
        for i=0..sizeof(nodes)-1 {
            nodes[i].outParent --> IdealChannel --> parent.ins++;
            nodes[i].inParent <-- IdealChannel <-- parent.outs++;
        }
}
