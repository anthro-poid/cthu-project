// EXPECT: 8500

int mixed_widths( unsigned char byte, int word )
{
    int adjusted_word = word + 1000;
    unsigned char adjusted_byte = byte + 50;
    goto combine;

combine: ;
    int result = adjusted_word + adjusted_byte;
    goto exit;

exit: ;
    return result * 2;
}

int main()
{
    return mixed_widths( 200, 3000 );
}
