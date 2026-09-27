// EXPECT: 31

unsigned char advance( unsigned char count )
{
    unsigned char value = 1;

    for ( unsigned char i = 0; i < count; ++ i )
        value = value * 2 + 1;

    return value;
}

int main()
{
    return advance( 4 );
}
