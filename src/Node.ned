package Project_GroupC;


simple Node
{
    parameters:
        double Modification_prob = default(30.0);
        double Loss_prob = default(30.0);
        double Duplication_prob = default(30.0);
        double Delays_prob = default(30.0);
        int n = default(4);
    gates:
        input ins[];
        output outs[];
        
        input inParent;
        output outParent;
}

