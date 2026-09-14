// EXPECT: 1018

int bool_and_words( int lhs, int rhs )
{
    _Bool equal = lhs == rhs;
    unsigned char byte = lhs + 20;
    goto combine;

combine: ;
    int result = lhs + byte + equal;
    goto exit;

exit: ;
    return result * 2;
}

int main()
{
    return bool_and_words( 500, 500 );
}
