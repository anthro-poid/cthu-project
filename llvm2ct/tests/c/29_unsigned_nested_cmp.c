int main()
{
    unsigned int a = 10;
    unsigned int b = 3;
    unsigned int c = 7;
    unsigned int d = 2;
    unsigned int x = ( a / b ) + ( c % d );
    unsigned int y = ( a * d ) - ( b + c );
    return x >= y;
}
