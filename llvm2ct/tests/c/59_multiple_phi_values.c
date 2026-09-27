// EXPECT: 20

int combine( _Bool condition, int first, int second )
{
    int left;
    int right;

    if ( condition )
    {
        left = first + 1;
        right = second + 2;
    }
    else
    {
        left = first - 1;
        right = second - 2;
    }

    return left * 3 + right;
}

int main()
{
    int when_true = combine( 1, 2, 4 );
    int when_false = combine( 0, 2, 4 );
    return when_true + when_false;
}
