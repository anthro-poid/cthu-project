// EXPECT: 219

int transform( int value, unsigned char delta, _Bool add )
{
    for ( int i = 0; i < 4; ++ i )
    {
        if ( i == 1 && !add )
        {
            value ^= delta;
            continue;
        }

        if ( add )
            value += delta + i;
        else
            value -= delta - i;

        if ( value > 40 )
            break;
    }

    return value;
}

int accumulate( int base, unsigned char limit )
{
    int total = base;

    for ( int outer = 0; outer < 3; ++ outer )
    {
        int inner = 0;

        do
        {
            ++ inner;

            if ( inner == 2 && outer == 0 )
                continue;

            if ( outer == 1 && inner == 3 )
                break;

            total += outer + inner;
        }
        while ( inner < limit );

        if ( ( total & 1 ) == 0 )
            total = total / 2 + outer;
        else
            total += 3;
    }

    return total;
}

unsigned char scramble( unsigned char value, unsigned char mask )
{
    unsigned char result = ( value + 3 ) ^ mask;

    if ( result > 20 )
        result = result - 7;
    else
        result = result << 1;

    return result;
}

_Bool accept( int value, unsigned char threshold )
{
    return value >= threshold && value != 0;
}

int combine( int base, unsigned char first, unsigned char second,
             _Bool reverse )
{
    unsigned char left = scramble( first, second );
    unsigned char right = scramble( second, first );
    int result = base;

    if ( reverse )
        result += right - left;
    else
        result += left + right;

    return accept( result, 5 ) ? result : 5;
}

int main()
{
    int first_transform = transform( 5, 8, 1 );
    int second_transform = transform( 30, 4, 0 );
    unsigned char narrowed = first_transform + second_transform;
    int adjustment = first_transform < second_transform ? 7 : 3;
    int control_flow = ( narrowed << 1 ) + adjustment;

    int nested = accumulate( 4, 4 );
    int first_combination = combine( 10, 6, 3, 0 );
    int second_combination = combine( first_combination, 25, 2, 1 );

    return control_flow + nested + first_combination + second_combination;
}
