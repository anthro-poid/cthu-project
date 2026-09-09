int main()
{
    unsigned int a = 4000000000u;
    unsigned int b = 65537u;
    unsigned int quotient = a / b;
    unsigned int remainder = a % b;
    return ( quotient >> 3 ) + remainder;
}
