// EXPECT: 14

int choose( int lhs, int rhs )
{
    int selected;

    if ( lhs < rhs )
        selected = lhs + 3;
    else
        selected = rhs - 2;

    return selected * 2;
}

int main()
{
    return choose( 2, 5 ) + choose( 8, 4 );
}
