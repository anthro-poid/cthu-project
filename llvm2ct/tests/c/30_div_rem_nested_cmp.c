int main()
{
    int a = 13;
    int b = 4;
    int c = 11;
    int d = 3;
    int q1 = a / b;
    int r1 = a % b;
    int q2 = c / d;
    int r2 = c % d;
    return ( q1 + r1 ) < ( q2 + r2 );
}
