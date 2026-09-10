// EXPECT: 0
int main()
{
    int a = 12;
    int b = 10;
    int c = 6;
    int d = 3;
    int x = ( a & b ) | ( c ^ d );
    int y = ( a | c ) & ( b ^ d );
    return x == y;
}
