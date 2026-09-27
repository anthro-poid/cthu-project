// EXPECT: 13

int fibonacci( int count )
{
    int previous = 0;
    int current = 1;

    for ( int i = 0; i < count; ++ i )
    {
        int next = previous + current;
        previous = current;
        current = next;
    }

    return previous;
}

int main()
{
    return fibonacci( 7 );
}
