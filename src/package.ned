package Project_GroupC;

@license(LGPL);

import ned.IdealChannel;

network Mesh
{
    parameters:
        int n = default(5);
    submodules:
        nodes[n]: Node;
    connections: 
       for i=0..sizeof(nodes)-1, for j=i+1..sizeof(nodes)-1
       {
           nodes[i].outs++ --> IdealChannel --> nodes[j].ins++;
           nodes[i].ins++ <-- IdealChannel <-- nodes[j].outs++;
       }  
}
