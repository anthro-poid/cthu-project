// EXPECT: 110

unsigned char add_chars( unsigned char a, unsigned char b,
                         unsigned char c, unsigned char d,
                         unsigned char e, unsigned char f,
                         unsigned char g )
{
    return a + b + c + d + e + f + g;
}

int main( void )
{
    return add_chars( 200, 100, 30, 20, 10, 5, 1 );
}
