// EXPECT: 395

int transitive_live_values( unsigned char byte, int word )
{
    unsigned char transformed_byte = byte ^ 170;
    int transformed_word = word * 3;
    goto middle;

middle: ;
    int local = transformed_word + 5;
    goto exit;

exit: ;
    return local + transformed_byte;
}

int main()
{
    return transitive_live_values( 240, 100 );
}
