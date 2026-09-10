// EXPECT: 250

int widen( unsigned char value )
{
    return value;
}

int main()
{
    return widen( 250 );
}
