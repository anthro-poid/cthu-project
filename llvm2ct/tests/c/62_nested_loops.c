// EXPECT: 30

int main()
{
    int total = 0;

    for ( int outer = 0; outer < 3; ++ outer )
        for ( int inner = 0; inner < 4; ++ inner )
            total += outer + inner;

    return total;
}
