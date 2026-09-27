// EXPECT: 15

int main()
{
    int sum = 0;
    int i = 0;

    for ( ; i < 10; ++ i )
    {
        if ( i == 5 )
            break;

        sum += i;
    }

    return sum + i;
}
