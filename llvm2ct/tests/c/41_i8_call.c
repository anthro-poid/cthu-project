// EXPECT: 44

unsigned char add_chars( unsigned char a, unsigned char b )
{
    return a + b;
}

int main( void )
{
    return add_chars( 200, 100 );
}
