// EXPECT: 30

int mixed_pipeline( unsigned char byte, signed char signed_byte,
                    int lhs, int rhs )
{
    _Bool less = lhs < rhs;
    unsigned char adjusted_byte = byte + 17;
    signed char adjusted_signed = signed_byte - 3;
    int base = lhs * 2 + rhs;
    goto first;

first: ;
    int partial = base + adjusted_byte;
    unsigned char carried = adjusted_byte ^ 90;
    goto second;

second: ;
    int combined = partial + adjusted_signed + less;
    goto exit;

exit: ;
    return combined + carried;
}

int main()
{
    return mixed_pipeline( 250, -100, 10, 20 );
}
