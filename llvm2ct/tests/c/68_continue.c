// EXPECT: 9

int main()
{
    int sum = 0;

    for ( int i = 0; i < 6; ++ i )
    {
        if ( i == 0 || i == 2 || i == 4 )
            continue;

        sum += i;
    }

    return sum;
}
