// EXPECT: 46

int nested_conditionals( int value, unsigned char upper, signed char lower )
{
    int common = value + upper + lower;

    if ( value >= 5 )
    {
        if ( upper > 5 )
        {
            int dead = common * 2;
            (void) dead;
        }
        else
        {
            int dead = common - 4;
            (void) dead;
        }
    }
    else
    {
        if ( lower < 5 )
        {
            int dead = common + 5;
            (void) dead;
        }
        else
        {
            int dead = common / 2;
            (void) dead;
        }
    }

    return common;
}

int main()
{
    int first = nested_conditionals( 6, 7, 1 );
    int second = nested_conditionals( 6, 3, 2 );
    int third = nested_conditionals( 2, 4, 3 );
    int fourth = nested_conditionals( 2, 4, 6 );
    return first + second + third + fourth;
}
