// EXPECT: 0
int main()
{
    int a = 2;
    int b = 3;
    int c = 4;
    int d = 5;
    int x = ( a + b ) * ( c - d );
    int y = ( a * c ) + ( b * d );
    return x > y;
}
