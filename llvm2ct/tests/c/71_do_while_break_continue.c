// EXPECT: 13

int main()
{
    int sum = 0;
    int i = 0;

    do
    {
        ++ i;

        if ( i == 2 )
            continue;

        if ( i == 6 )
            break;

        sum += i;
    }
    while ( i < 10 );

    return sum;
}
