// EXPECT: 609

int conditional_bool_chain( int lhs, int rhs, unsigned char tag )
{
    _Bool less = lhs < rhs;
    _Bool high = tag > 127;
    int common = lhs + rhs + tag;

    if ( less )
    {
        if ( high )
        {
            int dead = common << 1;
            (void) dead;
        }
        else
        {
            int dead = common ^ lhs;
            (void) dead;
        }
    }
    else
    {
        if ( high )
        {
            int dead = common - rhs;
            (void) dead;
        }
        else
        {
            int dead = common + lhs;
            (void) dead;
        }
    }

    return common + less + high;
}

int main()
{
    int first = conditional_bool_chain( 10, 20, 200 );
    int second = conditional_bool_chain( 30, 4, 10 );
    int third = conditional_bool_chain( 1, 8, 20 );
    int fourth = conditional_bool_chain( 50, 2, 250 );
    return first + second + third + fourth;
}
