package Project_GroupC;


simple Parent
{
     parameters:
        double refreshGap_TimeStep = default(30.0);
        double ProbabiltyToNotAssign = default(50.0);
        int n = default(4);
    gates:
        input ins[];
        output outs[];
}
