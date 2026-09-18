// EXPECT: 9

void conditional_work( int value )
{
    if ( value > 10 )
    {
        int dead = value + 20;
        (void) dead;
    }
    else
    {
        int dead = value - 3;
        (void) dead;
    }
}

int main()
{
    conditional_work( 20 );
    conditional_work( 5 );
    return 9;
}
