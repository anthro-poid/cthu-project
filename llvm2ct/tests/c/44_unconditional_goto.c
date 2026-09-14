// EXPECT: 42

int forward( int value )
{
    int result = value + 20;
    goto exit;

exit:
    return result + 2;
}

int main()
{
    return forward( 20 );
}
