// EXPECT: 15

unsigned char choose_byte( _Bool condition,
                           unsigned char first, unsigned char second )
{
    unsigned char selected;

    if ( condition )
        selected = first + 1;
    else
        selected = second ^ 3;

    return selected;
}

int main()
{
    int first = choose_byte( 1, 4, 9 );
    int second = choose_byte( 0, 4, 9 );
    return first + second;
}
