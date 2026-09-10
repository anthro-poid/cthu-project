// EXPECT: 1
int main()
{
    int a = 7;
    int b = 2;
    int c = 9;
    int d = 3;
    int e = 6;
    int f = 4;
    int g = 8;
    int h = 5;
    int left  = ( a + b ) * ( c - d );
    int right = ( e - f ) * ( g + h );
    return left != right;
}
