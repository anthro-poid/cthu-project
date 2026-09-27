// EXPECT: 13

int fib_rec( int i, int f1, int f2 )
{
    if ( i == 0 )
        return f1;
    return fib_rec( i - 1, f2, f1 + f2 );
}

int main()
{
    return fib_rec( 7, 0, 1 );
}
