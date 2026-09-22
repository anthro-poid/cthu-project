// EXPECT: 450

int conditional_mixed_widths( unsigned char byte, signed char delta, int base )
{
    int common = base + byte - delta;

    if ( byte > 127 )
    {
        unsigned char dead = byte ^ 90;
        (void) dead;
    }
    else
    {
        signed char dead = delta - 7;
        (void) dead;
    }

    return common * 2;
}

int main()
{
    int large = conditional_mixed_widths( 200, -5, 10 );
    int small = conditional_mixed_widths( 20, 7, -3 );
    return large + small;
}
