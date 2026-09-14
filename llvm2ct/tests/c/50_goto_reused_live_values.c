// EXPECT: 48

int reused_live_values( unsigned char byte, int word )
{
    int base = word + byte;
    goto square;

square: ;
    int squared = base * base;
    unsigned char incremented = byte + 1;
    goto exit;

exit: ;
    return squared + base + incremented;
}

int main()
{
    return reused_live_values( 5, 1 );
}
