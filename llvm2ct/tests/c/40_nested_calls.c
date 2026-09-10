// EXPECT: 42

int increment( int value )
{
    return value + 1;
}

int increment_twice( int value )
{
    return increment( increment( value ) );
}

int main( void )
{
    return increment_twice( 40 );
}
