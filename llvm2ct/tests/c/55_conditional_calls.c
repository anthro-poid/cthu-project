// EXPECT: 89

int increase( int value, unsigned char byte )
{
    return value + byte + 1;
}

int decrease( int value, unsigned char byte )
{
    return value - byte - 1;
}

int conditional_calls( _Bool outer, _Bool inner,
                       int value, unsigned char byte )
{
    int common = value * 2 + byte;

    if ( outer )
    {
        int dead = increase( common, byte );

        if ( inner )
        {
            int nested_dead = decrease( dead, byte );
            (void) nested_dead;
        }
        else
        {
            int nested_dead = increase( dead, byte );
            (void) nested_dead;
        }
    }
    else
    {
        int dead = decrease( common, byte );

        if ( inner )
        {
            int nested_dead = increase( dead, byte );
            (void) nested_dead;
        }
        else
        {
            int nested_dead = decrease( dead, byte );
            (void) nested_dead;
        }
    }

    return common;
}

int main()
{
    int first = conditional_calls( 1, 1, 10, 5 );
    int second = conditional_calls( 1, 0, 20, 6 );
    int third = conditional_calls( 0, 1, -4, 9 );
    int fourth = conditional_calls( 0, 0, 7, 3 );
    return first + second + third + fourth;
}
