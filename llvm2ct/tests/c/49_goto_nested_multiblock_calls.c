// EXPECT: 134

int affine( unsigned char byte, int word )
{
    int scaled = word * 3;
    goto adjust;

adjust: ;
    int result = scaled + byte;
    goto exit;

exit: ;
    return result - 7;
}

int nested_calls( unsigned char first_byte,
                  unsigned char second_byte, int base )
{
    int first = affine( first_byte, base );
    goto second;

second: ;
    int result = affine( second_byte, first );
    goto exit;

exit: ;
    return result * 2;
}

int main()
{
    return nested_calls( 10, 20, 5 );
}
